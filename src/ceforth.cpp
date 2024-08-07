///
/// @file
/// @brief eForth implemented in 100% C/C++ for portability and education
///
#include <string.h>      // strcmp
#include <strings.h>     // strcasecmp
#include "ceforth.h"
///====================================================================
///
///> Global memory blocks
///
/// Note:
///   1.By separating pmem from dictionary,
///   * it makes dictionary uniform size which eliminates the need for link field
///   * however, it requires array size tuning manually
///   2.Using 16-bit xt offset in parameter field (instead of full 32 or 64 bits),
///   * it unified xt/pfa parameter storage and use the LSB for id flag
///   * that compacts memory usage while avoiding the double lookup of
///   * token threaded indexing.
///   * However, it limits function pointer spread within range of 64KB
///   3.For ease of byte counting, U8* is used for pmem instead of U16*.
///   * this makes IP increment by 2 instead of word size.
///   * If needed, it can be readjusted.
///
///> Dictionary structure (N=E4_DICT_SZ in config.h)
///     dict[0].xt ---------> pointer to primitive word lambda[0]
///     dict[1].xt ---------> pointer to primitive word lambda[1]
///     ...
///     dict[N-1].xt -------> pointer to last primitive word lambda[N-1]
///
///> Parameter memory structure (memory block=E4_PMEM_SZ in config.h)
///     dict[N].xt ----+ user defined colon word)    dict[N+1].xt------+
///                    |                                               |
///     +--MEM0        v                                               v
///     +--------------+--------+--------+-----+------+----------------+-----
///     | str nameN \0 |  parm1 |  parm2 | ... | ffff | str nameN+1 \0 | ...
///     +--------------+--------+--------+-----+------+----------------+-----
///     ^              ^        ^        ^     ^      ^
///     | strlen+1     | 2-byte | 2-byte |     |      |
///     +--------------+--------+--------+-----+------+---- 2-byte aligned
///
///> Parameter structure - 16-bit aligned (use LSB for colon word flag)
///   * primitive word
///     16-bit xt offset with LSB set to 0 (1 less mem lookup for xt)
///     +--------------+-+
///     | dict.xtoff() |0|   call (XT0 + *IP)() to execute
///     +--------------+-+   
///     |0| dict.xtoff() |   WASM XT0=0, xt is index to vtable
///     +--------------+-+   
///
///   * colon word (user defined)
///     16-bit pmem offset with LSB set to 1
///     +--------------+-+
///     |  dict.pfa    |1|   next IP = *(MEM0 + (*IP & ~1))
///     +--------------+-+
///     |1|   dict.pfa   |   WASM (32K max parameter space)
///     +--------------+-+
///
List<DU,   E4_RS_SZ>   rs;         ///< return stack
List<DU,   E4_SS_SZ>   ss;         ///< parameter stack
List<Code, E4_DICT_SZ> dict;       ///< dictionary
List<U8,   E4_PMEM_SZ> pmem;       ///< parameter memory (for colon definitions)
U8  *MEM0 = &pmem[0];              ///< base of parameter memory block
///
///> Macros to abstract dict and pmem physical implementation
///  Note:
///    so we can change pmem implementation anytime without affecting opcodes defined below
///
///@name Dictionary and data stack access macros
///@{
#define BOOL(f)   ((f)?-1:0)               /**< Forth boolean representation            */
#define HERE      (pmem.idx)               /**< current parameter memory index          */
#define MEM(ip)   (MEM0 + (IU)UINT(ip))    /**< pointer to IP address fetched from pmem */
#define CELL(a)   (*(DU*)&pmem[a])         /**< fetch a cell from parameter memory      */
#define SETJMP(a) (*(IU*)&pmem[a] = HERE)  /**< address offset for branching opcodes    */
///@}
///
/// enum used for built-in opcode tokens to simplify compiler
/// i.g, add_w(find("donext ")) can be reduced to add_w(NEXT)
/// but make sure the sequence is in-sync with word list in dict_compile
///
typedef enum {
    EXIT=0, NEXT, LOOP, LIT, VAR, STR, DOTQ, BRAN, ZBRAN, DOES, FOR, DO
} forth_opcode;
///
///====================================================================
///
///> VM states (single task)
///
DU   top     = -DU1;    ///< top of stack (cached)
IU   IP      = 0;       ///< current instruction pointer
bool run     = true;    ///< VM nest() control
bool compile = false;   ///< compiler flag
bool ucase   = false;   ///< case sensitivity control
IU   *base;             ///< numeric radix (a pointer)
IU   *dflt;             ///< use float data unit flag
///
///====================================================================
///
///> Dictionary search functions - can be adapted for ROM+RAM
///
int streq(const char *s1, const char *s2) {
    return ucase ? strcasecmp(s1, s2)==0 : strcmp(s1, s2)==0;
}
IU find(const char *s) {
    IU v = 0;
    for (IU i = dict.idx - 1; i > 0; --i) {
        if (streq(s, dict[i].name)) { v = i; break; }
    }
#if CC_DEBUG > 1
    LOG_HDR("find", s); if (v) { LOG_DIC(v); } else LOG_NA();
#endif // CC_DEBUG > 1
    return v;
}
///====================================================================
///
///> Colon word compiler
///  Note:
///    * we separate dict and pmem space to make word uniform in size
///    * if they are combined then can behaves similar to classic Forth
///    * with an addition link field added.
///
void colon(const char *name) {
    char *nfa = (char*)&pmem[HERE]; ///> current pmem pointer
    int sz = STRLEN(name);          ///> string length, aligned
    pmem.push((U8*)name,  sz);      ///> setup raw name field

    Code c(nfa, (FPTR)~0, false);   ///> create a local blank word
    c.attr = UDF_ATTR;              ///> specify a colon (user defined) word
    c.pfa  = HERE;                  ///> capture code field index

    dict.push(c);                   ///> deep copy Code struct into dictionary
}
void add_iu(IU i) { pmem.push((U8*)&i, sizeof(IU)); }  ///< add an instruction into pmem
void add_du(DU v) { pmem.push((U8*)&v, sizeof(DU)); }  ///< add a cell into pmem
int  add_str(const char *s) {       ///< add a string to pmem
    int sz = STRLEN(s);
    pmem.push((U8*)s,  sz);
    return sz;
}
void add_w(IU w) {                  ///< add a word index into pmem
    Code &c = dict[w];              ///< ref to dictionary entry
    IU   ip = c.attr & UDF_ATTR     ///< check whether colon word
        ? (c.pfa | UDF_FLAG)        ///< pfa with colon word flag
        : (w==EXIT ? EXIT : c.xtoff());   ///< XT offset
    add_iu(ip);
#if CC_DEBUG > 1
    LOG_KV("add_w(", w); LOG_KX(") => ", ip);
    LOG_KX(":", c.xtoff()); LOGS(" "); LOGS(c.name); LOGS("\n");
#endif // CC_DEBUG > 1
}
///====================================================================
///
///> Forth inner interpreter (handles a colon word)
///  Note:
///  * overhead here in C call/return vs NEXT threading (in assembly)
///  * use of dp (iterative depth control) instead of WP by Dr. Ting
///    speeds up 8% vs recursive calls but loose the flexibity of Forth
///  * use of cached _NXT address speeds up 10% on AMD but
///    5% slower on ESP32 probably due to shallow pipeline
///  * computed label runs 15% faster, but needs long macros (for enum)
///  * use local stack speeds up 10%, but allot 4*64 bytes extra
///
///  TODO: performance tuning
///    1. Just-in-time cache(ip, dp)
///    2. Co-routine
///
void nest() {
    static IU _NXT = dict[find("next ")].xtoff(); ///> cache offsets to funtion pointers
    static IU _LIT = dict[find("lit ")].xtoff();
    int dp = 0;                        ///> iterator implementation (instead of recursive)
    while (dp >= 0) {                  ///> depth control
        IU ix = *(IU*)MEM(IP);         ///> fetch opcode, hopefully cached
        run = true;                    ///> re-enable loop control
        while (run && ix) {            ///> loop till 0 (EXIT) hit
            IP += sizeof(IU);          /// * advance inst. ptr
            if (ix & UDF_FLAG) {       ///> is it a colon word?
                rs.push(IP);
                IP = ix & ~UDF_FLAG;   ///> word pfa (def masked)
                dp++;                  ///> go one level deeper
            }
            else if (ix == _NXT) {     ///> cached NEXT, LIT handlers,
                if (GT(rs[-1] -= DU1, -DU1)) {       ///> loop done?
                    IP = *(IU*)MEM(IP);              ///> 10% faster on AMD, 5% on ESP32
                }
                else { IP += sizeof(IU); rs.pop(); } ///> perhaps due to shallow pipeline
            }
            else if (ix == _LIT) {
                ss.push(top);
                top = *(DU*)MEM(IP);   ///> from hot cache, hopefully
                IP += sizeof(DU);
            }
            else Code::exec(ix);       ///> execute primitive word

            ix = *(IU*)MEM(IP);        ///> fetch next opcode
        }
        if (dp-- > 0) IP = rs.pop();   ///> pop off a level

        yield();                       ///> give other tasks some time
    }
}
///
///> CALL - inner-interpreter proxy (inline macro does not run faster)
///
void CALL(IU w) {
    if (IS_UDF(w)) { IP = dict[w].pfa; nest(); }
    else dict[w].call();
}
///====================================================================
///
/// utilize C++ standard template libraries for core IO functions only
/// Note:
///   * we use STL for its convinence, but
///   * if it takes too much memory for target MCU,
///   * these functions can be replaced with our own implementation
///
#include <iomanip>                     /// setbase, setw, setfill
#include <iostream>                    /// cin, cout
#include <sstream>                     /// stringstream
using namespace std;                   /// default to C++ standard template library
istringstream   fin;                   ///< forth_in
ostringstream   fout;                  ///< forth_out
string          pad;                   ///< input string buffer
void (*fout_cb)(int, const char*);     ///< forth output callback function (see ENDL macro)
///====================================================================
///
/// macros to reduce verbosity
///
int def_word(const char* name) {                ///< display if redefined
    if (name[0]=='\0') { fout << " name?" << ENDL; return 0; }  /// * missing name?
    if (find(name)) {                           /// * word redefined?
        fout << name << " reDef? " << ENDL;
    }
    colon(name);                                /// * create a colon word
    return 1;                                   /// * created OK
}
char *word() {                                  ///< get next idiom
    if (!(fin >> pad)) pad.clear();             /// * input buffer exhausted?
    return (char*)pad.c_str();
}
inline char *scan(char c) { getline(fin, pad, c); return (char*)pad.c_str(); }
inline void PUSH(DU v) { ss.push(top); top = v; }
inline DU   POP()      { DU n=top; top=ss.pop(); return n; }
///====================================================================
///
///> IO & debug functions
///
void spaces(int n)  { for (int i = 0; i < n; i++) fout << " "; }
void s_quote(IU op) {
    const char *s = scan('"')+1;       ///> string skip first blank
    if (compile) {
        add_w(op);                     ///> dostr, (+parameter field)
        add_str(s);                    ///> byte0, byte1, byte2, ..., byteN
    }
    else {                             ///> use PAD ad TEMP storage
        IU h0  = HERE;
        DU len = add_str(s);           ///> write string to PAD
        PUSH(h0);                      ///> push string address
        PUSH(len);                     ///> push string length
        HERE = h0;                     ///> restore memory addr
    }
}
///
/// recursively disassemble colon word
///
void to_s(IU w, U8 *ip) {
#if CC_DEBUG
    fout << "( " << setfill('0') << setw(4) << (ip - MEM0) << "["; ///> addr
    if (w==EXIT) fout << "EOW";
    else         fout << setfill(' ') << setw(3) << w;
    fout << "] ) ";
#endif // CC_DEBUG
    ip += sizeof(IU);                  ///> calculate next ip
    switch (w) {
    case EXIT: fout << ";";                         break;
    case LIT:  fout << *(DU*)ip << " ( lit )";      break;
    case VAR: {
        int  n = *(IU*)ip;  ip += sizeof(IU);       ///> number of elements
        for (int i = 0; i < n; i+=sizeof(DU)) {
            fout << *(DU*)ip << ' '; ip += sizeof(DU);
        }
        fout << " ( var )";
    } break;
    case STR:  fout << "s\" " << (char*)ip << '"';  break;
    case DOTQ: fout << ".\" " << (char*)ip << '"';  break;
    default:   fout << dict[w].name;                break;
    }
#if CC_DEBUG    
    switch (w) {
    case NEXT: case LOOP:
    case BRAN: case ZBRAN:                         ///> display jmp target
        fout << "( " << setfill('0') << setw(4) << *(IU*)ip << " )";
        break;
    default: /* do nothing */         break;
    }
#endif // CC_DEBUG    
    fout << setfill(' ') << setw(-1); ///> restore output format settings
}
void see(IU pfa, int dp=1) {
    auto pfa2opcode = [](IU ix) {                  ///> reverse lookup
        if (ix==EXIT) return (int)EXIT;            ///> end of word handler
        IU   pfa = ix & ~UDF_FLAG;                 ///> pfa (mask colon word)
        FPTR xt  = Code::XT(pfa);                  ///> lambda pointer
        for (int i = dict.idx - 1; i > 0; --i) {
            if (ix & UDF_FLAG) {
                if (dict[i].pfa == pfa) return i;  ///> compare pfa in PMEM
            }
            else if (dict[i].xt == xt) return i;   ///> compare xt
        }
        return -1;                                 /// * not found
    };
    U8 *ip = MEM(pfa);
    while (1) {
        IU w = pfa2opcode(*(IU*)ip);    ///> fetch word index by pfa
        if (w==(IU)-1) break;           ///> loop guard
        
        fout << ENDL; for (int i=dp; i>0; i--) fout << "  "; ///> indent
        to_s(w, ip);                    ///> display opcode
        if (w==EXIT) break;             ///> done with the colon word

        ip += sizeof(IU);               ///> advance ip (next opcode)
        switch (w) {                    ///> extra bytes to skip
        case LIT:   ip += sizeof(DU);                    break;
        case VAR:   ip += sizeof(IU) + *(IU*)ip;         break;
        case STR:   case DOTQ:  ip += STRLEN((char*)ip); break;
        case BRAN:  case ZBRAN:
        case NEXT:  case LOOP:  ip += sizeof(IU);        break;
        }
#if CC_DEBUG > 1
        ///> walk recursively
        if (IS_UDF(w) && dp < 2) {      ///> is a colon word
            see(dict[w].pfa, dp+1);     ///> recursive into child
        }
#endif // CC_DEBUG > 1
    }
}
void words() {
    const int WIDTH = 60;
    int sz = 0;
    fout << setbase(10);
    for (int i=0; i<dict.idx; i++) {
        const char *nm = dict[i].name;
        const int  len = strlen(nm);
#if CC_DEBUG > 1
        if (nm[0]) {
#else  //  CC_DEBUG > 1
        if (nm[len-1] != ' ') {
#endif // CC_DEBUG > 1           
            sz += len + 2;
            fout << "  " << nm;
        }
        if (sz > WIDTH) {
            sz = 0;
            fout << ENDL;
            yield();
        }
    }
    fout << setbase(*base) << ENDL;
}
void ss_dump() {
    static char buf[34];
    auto rdx = [](DU v, int b) {          ///> display v by radix
#if USE_FLOAT
        sprintf(buf, "%0.6g", v);
        return buf;
#else  // !USE_FLOAT
        int i = 33;  buf[i]='\0';         /// * C++ can do only 8,10,16
        DU  n = ABS(v);                   ///< handle negative
        while (n && i) {                  ///> digit-by-digit
            U8 d = (U8)MOD(n,b);  n /= b;
            buf[--i] = d > 9 ? (d-10)+'a' : d+'0';
        }
        if (v < DU0) buf[--i]='-';
        return &buf[i];
#endif // USE_FLOAT
    };
    ss.push(top);
    for (int i=0; i<ss.idx; i++) {
        fout << rdx(ss[i], *base) << ' ';
    }
    top = ss.pop();
    fout << "-> ok" << ENDL;
}
void mem_dump(U32 p0, IU sz) {
    fout << setbase(16) << setfill('0');
    for (IU i=ALIGN16(p0); i<=ALIGN16(p0+sz); i+=16) {
        fout << setw(4) << i << ": ";
        for (int j=0; j<16; j++) {
            U8 c = pmem[i+j];
            fout << setw(2) << (int)c << (MOD(j,4)==3 ? " " : "");
        }
        for (int j=0; j<16; j++) {   // print and advance to next byte
            U8 c = pmem[i+j] & 0x7f;
            fout << (char)((c==0x7f||c<0x20) ? '_' : c);
        }
        fout << ENDL;
        yield();
    }
    fout << setbase(*base) << setfill(' ');
}
///====================================================================
///
///> System statistics - for heap, stack, external memory debugging
///
void mem_stat();   ///< forward decl (implemented in platform specific)
void dict_dump() {
    fout << setbase(16) << setfill('0') << "XT0=" << Code::XT0 << ENDL;
    for (int i=0; i<dict.idx; i++) {
        Code &c = dict[i];
        fout << setfill('0') << setw(3) << i
             << "> attr=" << (c.attr & 0x3)
             << ", xt="   << setw(4) << (IS_UDF(i) ? c.pfa : c.xtoff())
             << ":"       << setw(8) << (UFP)c.xt
             << ", name=" << setw(8) << (UFP)c.name
             << " "       << c.name << ENDL;
    }
    fout << setbase(*base) << setfill(' ') << setw(-1);
}
///====================================================================
///
///> Javascript/WASM interface
///
#if DO_WASM
/// function in worker thread
EM_JS(void, js, (const char *ops), {
    postMessage(['js', UTF8ToString(ops)])
});
void call_js() {                           ///> ( n addr u -- )
    stringstream n;
    auto t2s = [&n](char c) {              ///< template to string
        n.str("");                         /// * clear stream
        switch (c) {
        case 'd':
        case 'p': n << UINT(POP());                break;
        case 'f': n << (DU)POP();                  break;
        case 'x': n << "0x" << hex << UINT(POP()); break;
        case 's': POP(); n << (char*)MEM(POP());   break;  /// also handles raw stream
        default : n << c << '?';                   break;
        }
        return n.str();
    };
    POP();                                 /// * strlen, not used
    pad.clear();                           /// * borrow PAD for string op
    pad.append((char*)MEM(POP()));         /// copy string on stack
    for (size_t i=pad.find_last_of('%');   ///> find % from back
         i!=string::npos;                  /// * until not found
         i=pad.find_last_of('%',i?i-1:0)) {
        if (i && pad[i-1]=='%') {          /// * double %%
            pad.replace(--i,1,"");         /// * drop one %
        }
        else pad.replace(i, 2, t2s(pad[i+1]));
    }
    js(pad.c_str());    /// * call Emscripten js function
}
#endif // DO_WASM
///====================================================================
///
///> eForth dictionary assembler
///  Note: sequenced by enum forth_opcode as following
///
UFP Code::XT0 = ~0;    ///< init base of xt pointers (before calling CODE macros)

void dict_compile() {  ///< compile primitive words into dictionary
    ///
    /// @defgroup Execution flow ops
    /// @brief - DO NOT change the sequence here (see forth_opcode enum)
    ///        - the build-in control words have extra last blank char
    /// @{
    CODE("exit ",   {});                                      // dict[0] also the storage for base
    CODE("next ",                                             // handled in nest()
         if (GT(rs[-1] -= DU1, -DU1)) IP = *(IU*)MEM(IP);     // rs[-1]-=1 saved 200ms/1M cycles
         else { IP += sizeof(IU); rs.pop(); });
    CODE("loop ",                                             // handled in nest()
         if (GT(rs[-2], rs[-1] += DU1)) IP = *(IU*)MEM(IP);
         else { IP += sizeof(IU); rs.pop(); rs.pop(); });
    CODE("lit ",    PUSH(*(DU*)MEM(IP)); IP += sizeof(DU));
    CODE("var ",    PUSH(IP + sizeof(IU));                    // get var addr (skip over EXIT)
                    IP += sizeof(IU) + *(IU*)MEM(IP));        // len + data
    CODE("str ",    const char *s = (const char*)MEM(IP);     // get string pointer
                    IU    len = STRLEN(s);
                    PUSH(IP); PUSH(len); IP += len);
    CODE("dotq ",   const char *s = (const char*)MEM(IP);     // get string pointer
                    fout << s;  IP += STRLEN(s));             // send to output console
    CODE("bran " ,  IP = *(IU*)MEM(IP));                      // unconditional branch
    CODE("0bran ",  IP = POP() ? IP + sizeof(IU) : *(IU*)MEM(IP)); // conditional branch
    CODE("does> ",  add_w(BRAN); add_iu(IP); run = false);    // encode current IP, and bail
    CODE("for ",    rs.push(POP()));
    CODE("do ",     rs.push(ss.pop()); rs.push(POP()));       // DO..LOOP control
    /// - DO NOT change the sequence above (see forth_opcode enum)
    /// @}
    /// @defgroup Stack ops
    /// @brief - opcode sequence can be changed below this line
    /// @{
    CODE("dup",     PUSH(top));
    CODE("drop",    top = ss.pop());
    CODE("over",    PUSH(ss[-1]));
    CODE("swap",    DU n = ss.pop(); PUSH(n));
    CODE("rot",     DU n = ss.pop(); DU m = ss.pop(); ss.push(n); PUSH(m));
    CODE("-rot",    DU n = ss.pop(); DU m = ss.pop(); PUSH(m); PUSH(n));
    CODE("nip",     ss.pop());
    CODE("pick",    DU i = top; top = ss[-i]);
    /// @}
    /// @defgroup Stack ops - double
    /// @{
    CODE("2dup",    PUSH(ss[-1]); PUSH(ss[-1]));
    CODE("2drop",   ss.pop(); top = ss.pop());
    CODE("2over",   PUSH(ss[-3]); PUSH(ss[-3]));
    CODE("2swap",   DU n = ss.pop(); DU m = ss.pop(); DU l = ss.pop();
                    ss.push(n); PUSH(l); PUSH(m));
    CODE("?dup",    if (top != DU0) PUSH(top));
    /// @}
    /// @defgroup ALU ops
    /// @{
    CODE("+",       top += ss.pop());
    CODE("*",       top *= ss.pop());
    CODE("-",       top =  ss.pop() - top);
    CODE("/",       top =  ss.pop() / top);
    CODE("mod",     top =  MOD(ss.pop(), top));
    CODE("*/",      top =  (DU2)ss.pop() * ss.pop() / top);
    CODE("/mod",    DU  n = ss.pop();
                    DU  t = top;
                    DU  m = MOD(n, t);
                    ss.push(m); top = UINT(n / t));
    CODE("*/mod",   DU2 n = (DU2)ss.pop() * ss.pop();
                    DU2 t = top;
                    DU  m = MOD(n, t);
                    ss.push(m); top = UINT(n / t));
    CODE("and",     top = UINT(top) & UINT(ss.pop()));
    CODE("or",      top = UINT(top) | UINT(ss.pop()));
    CODE("xor",     top = UINT(top) ^ UINT(ss.pop()));
    CODE("abs",     top = ABS(top));
    CODE("negate",  top = -top);
    CODE("invert",  top = ~UINT(top));
    CODE("rshift",  top = UINT(ss.pop()) >> UINT(top));
    CODE("lshift",  top = UINT(ss.pop()) << UINT(top));
    CODE("max",     DU n=ss.pop(); top = (top>n) ? top : n);
    CODE("min",     DU n=ss.pop(); top = (top<n) ? top : n);
    CODE("2*",      top *= 2);
    CODE("2/",      top /= 2);
    CODE("1+",      top += 1);
    CODE("1-",      top -= 1);
    /// @}
    /// @defgroup Logic ops
    /// @{
    CODE("0=",      top = BOOL(ZEQ(top)));
    CODE("0<",      top = BOOL(LT(top, DU0)));
    CODE("0>",      top = BOOL(GT(top, DU0)));
    CODE("=",       top = BOOL(EQ(ss.pop(), top)));
    CODE(">",       top = BOOL(GT(ss.pop(), top)));
    CODE("<",       top = BOOL(LT(ss.pop(), top)));
    CODE("<>",      top = BOOL(!EQ(ss.pop(), top)));
    CODE(">=",      top = BOOL(!LT(ss.pop(), top)));
    CODE("<=",      top = BOOL(!GT(ss.pop(), top)));
    CODE("u<",      top = BOOL(UINT(ss.pop()) < UINT(top)));
    CODE("u>",      top = BOOL(UINT(ss.pop()) > UINT(top)));
    /// @}
    /// @defgroup IO ops
    /// @{
    CODE("case!",   ucase = POP() == DU0);    // case insensitive
    CODE("base",    PUSH(((U8*)base - MEM0)));
    CODE("decimal", fout << setbase(*base = 10));
    CODE("hex",     fout << setbase(*base = 16));
    CODE("bl",      fout << " ");
    CODE("cr",      fout << ENDL);
    CODE(".",       fout << POP() << " ");
    CODE("u.",      fout << UINT(POP()) << " ");
    CODE(".r",      fout << setbase(*base) << setw(POP()) << POP());
    CODE("u.r",     fout << setbase(*base) << setw(POP()) << UINT(POP()));
    CODE("type",    POP();                    // string length (not used)
         fout << (const char*)MEM(POP()));    // get string pointer
    CODE("key",     PUSH(word()[0]));
    CODE("emit",    char b = (char)POP(); fout << b);
    CODE("space",   spaces(1));
    CODE("spaces",  spaces(POP()));
    /// @}
    /// @defgroup Literal ops
    /// @{
    CODE("[",       compile = false);
    CODE("]",       compile = true);
    IMMD("(",       scan(')'));
    IMMD(".(",      fout << scan(')'));
    IMMD("\\",      scan('\n'));
    IMMD("s\"",     s_quote(STR));
    IMMD(".\"",     s_quote(DOTQ));
    /// @}
    /// @defgroup Branching ops
    /// @brief - if...then, if...else...then
    /// @{
    IMMD("if",      add_w(ZBRAN); PUSH(HERE); add_iu(0));       // if    ( -- here )
    IMMD("else",                                                // else ( here -- there )
         add_w(BRAN);
         IU h=HERE; add_iu(0); SETJMP(POP()); PUSH(h));
    IMMD("then",    SETJMP(POP()));                             // backfill jump address
    /// @}
    /// @defgroup Loops
    /// @brief  - begin...again, begin...f until, begin...f while...repeat
    /// @{
    IMMD("begin",   PUSH(HERE));
    IMMD("again",   add_w(BRAN);  add_iu(POP()));               // again    ( there -- )
    IMMD("until",   add_w(ZBRAN); add_iu(POP()));               // until    ( there -- )
    IMMD("while",   add_w(ZBRAN); PUSH(HERE); add_iu(0));       // while    ( there -- there here )
    IMMD("repeat",  add_w(BRAN);                                // repeat    ( there1 there2 -- )
         IU t=POP(); add_iu(POP()); SETJMP(t));                 // set forward and loop back address
    /// @}
    /// @defgrouop FOR...NEXT loops
    /// @brief  - for...next, for...aft...then...next
    /// @{
    IMMD("for" ,    add_w(FOR); PUSH(HERE));                    // for ( -- here )
    IMMD("next",    add_w(NEXT); add_iu(POP()));                // next ( here -- )
    IMMD("aft",                                                 // aft ( here -- here there )
         POP(); add_w(BRAN);
         IU h=HERE; add_iu(0); PUSH(HERE); PUSH(h));
    /// @}
    /// @}
    /// @defgrouop DO..LOOP loops
    /// @{
    IMMD("do" ,     add_w(DO); PUSH(HERE));                     // for ( -- here )
    CODE("i",       PUSH(rs[-1]));
    CODE("leave",   rs.pop(); rs.pop(); run = false);           // quit DO..LOOP
    IMMD("loop",    add_w(LOOP); add_iu(POP()));                // next ( here -- )
    /// @}
    /// @defgrouop return stack ops
    /// @{
    CODE(">r",      rs.push(POP()));
    CODE("r>",      PUSH(rs.pop()));
    CODE("r@",      PUSH(rs[-1]));                      // same as I (the loop counter)
    /// @}
    /// @defgrouop Compiler ops
    /// @{
    CODE(":",       compile = def_word(word()));
    IMMD(";",       add_w(EXIT); compile = false);
    CODE("exit",    run = false);                               // early exit the colon word
    CODE("variable",                                            // create a variable
         def_word(word());                                      // create a new word on dictionary
         add_w(VAR); add_iu(sizeof(DU)); add_du(DU0);           // dovar (len=4, default 0)
         add_w(EXIT));
    CODE("constant",                                            // create a constant
         def_word(word());                                      // create a new word on dictionary
         add_w(LIT); add_du(POP());                             // dovar (+parameter field)
         add_w(EXIT));
    IMMD("immediate", dict[-1].attr |= IMM_ATTR);
    /// @}
    /// @defgroup metacompiler
    /// @brief - dict is directly used, instead of shield by macros
    /// @{
    CODE("exec",  IU w = POP(); CALL(w));                       // execute word
    CODE("create", def_word(word()); add_w(VAR));               // dovar (no parameter field)
    IMMD("does>", add_w(DOES));
    CODE("to",              // 3 to x                           // alter the value of a constant
         IU w = find(word()); if (!w) return;                   // to save the extra @ of a variable
         *(DU*)(MEM(dict[w].pfa) + sizeof(IU)) = POP());
    CODE("is",              // ' y is x                         // alias a word
         IU w = find(word()); if (!w) return;                   // copy entire union struct
         dict[POP()].xt = dict[w].xt);
    ///
    /// be careful with memory access, especially BYTE because
    /// it could make access misaligned which slows the access speed by 2x
    ///
    CODE("@",     IU w = UINT(POP()); PUSH(CELL(w)));           // w -- n
    CODE("!",     IU w = UINT(POP()); CELL(w) = POP(););        // n w --
    CODE(",",     DU n = POP(); add_du(n));                     // n -- 
    CODE("cells", IU i = UINT(POP()); PUSH(i * sizeof(DU)));    // n -- n'
    CODE("allot",                                               // n --
         IU n = UINT(POP());                                    // number of elements
         add_iu(n);                                             // encode length, default 0
         for (IU i = 0; i < n; i+=sizeof(IU)) add_iu(0));       // Note: need EXIT manually added
    CODE("+!",    IU w = UINT(POP()); CELL(w) += POP());        // n w --
    CODE("?",     IU w = UINT(POP()); fout << CELL(w) << " ");  // w --
    /// @}
    /// @defgroup Debug ops
    /// @{
    CODE("abort", top = -DU1; ss.clear(); rs.clear());          // clear ss, rs
    CODE("here",  PUSH(HERE));
    CODE("'",     IU w = find(word()); if (w) PUSH(w));
    CODE(".s",    ss_dump());
    CODE("depth", PUSH(ss.idx));
    CODE("words", words());
    CODE("see",
         IU w = find(word()); if (!w) return;
         fout << ": " << dict[w].name;
         if (IS_UDF(w)) see(dict[w].pfa);
         else           fout << " ( primitive ) ;";
         fout << ENDL);
    CODE("dump",  U32 n = UINT(POP()); mem_dump(UINT(POP()), n));
    CODE("dict",  dict_dump());
    CODE("forget",
         IU w = find(word()); if (!w) return;
         IU b = find("boot")+1;
         if (w > b) {
             pmem.clear(dict[w].pfa - STRLEN(dict[w].name));
             dict.clear(w);
         }
         else { dict.clear(b); pmem.clear(sizeof(DU)); }
    );
    /// @}
    /// @defgroup OS ops
    /// @{
    CODE("mstat", mem_stat());
    CODE("ms",    PUSH(millis()));
    CODE("rnd",   PUSH(RND()));             // generate random number
    CODE("delay", delay(UINT(POP())));
    CODE("included",                        // include external file
         POP();                             // string length, not used
         U8 *fn = MEM(POP());               // file name
         forth_include((const char*)fn));   // include file
#if DO_WASM    
    CODE("JS",    call_js());               // Javascript interface
#else  // !DO_WASM
    CODE("bye",   exit(0));
#endif // DO_WASM    
    /// @}
    CODE("boot",  dict.clear(find("boot") + 1); pmem.clear(sizeof(DU)));
}
///====================================================================
///
///> ForthVM - Outer interpreter
///
DU parse_number(const char *idiom, int *err) {
    int b = static_cast<int>(*base);
    switch (*idiom) {                        ///> base override
    case '%': b = 2;  idiom++; break;
    case '&':
    case '#': b = 10; idiom++; break;
    case '$': b = 16; idiom++; break;
    }
    char *p;
    *err = errno = 0;
#if USE_FLOAT
    DU n = (b==10)
        ? static_cast<DU>(strtof(idiom, &p))
        : static_cast<DU>(strtol(idiom, &p, b));
#else  // !USE_FLOAT
    DU n = static_cast<DU>(strtol(idiom, &p, b));
#endif // USE_FLOAT
    if (errno || *p != '\0') *err = 1;
    return n;
}

int forth_core(const char *idiom) {
    IU w = find(idiom);                  ///> * get token by searching through dict
    if (w) {                             ///> * word found?
        if (compile && !IS_IMM(w)) {     /// * in compile mode?
            add_w(w);                    /// * add to colon word
        }
        else CALL(w);                    /// * execute forth word
        return 0;
    }
    // try as a number
    int err = 0;
    DU  n   = parse_number(idiom, &err);
    if (err) {                           /// * not number
        fout << idiom << "? " << ENDL;   ///> display error prompt
        compile = false;                 ///> reset to interpreter mode
        return 1;                        ///> skip the entire input buffer
    }
    // is a number
    if (compile) {                       /// * a number in compile mode?
        add_w(LIT);                      ///> add to current word
        add_du(n);
    }
    else PUSH(n);                        ///> or, add value onto data stack
    return 0;
}
///====================================================================
///
/// Forth VM external command processor
///
void forth_init() {
    static bool init = false;
    if (init) return;                    ///> check dictionary initilized
    
    base = (IU*)MEM(HERE);               ///< set pointer to base
    add_iu(10);                          ///< allocate space for base
    dflt = (IU*)MEM(HERE);               ///< set pointer to dfmt
    add_iu(USE_FLOAT);
    
    dict_compile();                      ///> compile dictionary
}
void forth_vm(const char *cmd, void(*hook)(int, const char*)) {
    auto outer = []() {                  ///> outer interpreter loop
        string idiom;
        while (fin >> idiom) {           
            forth_core(idiom.c_str());   ///> send word to Forth core
        }
    };
    auto cb = [](int,const char *rst) { printf("%s", rst); };
    fout_cb = hook ? hook : cb;          ///> setup callback function
    
    istringstream istm(cmd);             ///< input stream
    string        line;                  ///< line command
    fout.str("");                        /// * clean output buffer
    while (getline(istm, line)) {        /// * fetch one line at a time
        fin.clear();                     /// * clear input stream error bit if any
        fin.str(line);                   /// * feed user command into input stream
        outer();                         /// * invoke Forth outer interpreter
    }
#if DO_WASM    
    if (!compile) fout << "ok" << ENDL;
#else
    if (!compile) ss_dump();   /// * dump stack and display ok prompt
#endif  // DO_WASM
}
///====================================================================
///> WASM specific code
///
///> Memory statistics - for heap, stack, external memory debugging
///
void mem_stat() {
    fout << APP_VERSION
         << "\n  dict: " << dict.idx  << "/" << E4_DICT_SZ
         << "\n  ss  : " << ss.idx    << "/" << E4_SS_SZ << " (max " << ss.max
         << ")\n  rs  : " << rs.idx   << "/" << E4_RS_SZ << " (max " << rs.max
         << ")\n  mem : " << HERE     << "/" << E4_PMEM_SZ << endl;
}

int  forth_include(const char *fn) {
#if DO_WASM
    const int BUF = HERE + 64;
    const int rst = EM_ASM_INT({
        const xhr = new XMLHttpRequest();
        xhr.responseType = 'text';                /// Forth script
        xhr.open('GET', UTF8ToString($0), false); /// synchronized GET
        xhr.send(null);
        if (xhr.status!=200) {
            console.log(xhr.statusText);
            return 0;
        }
        const wa  = wasmExports;               ///< WASM export block
        const adr = wa.vm_mem() + $1;          ///< memory address
        const len = xhr.responseText.length+1; ///< script + '\0'
        const buf = new Uint8Array(wa.memory.buffer, adr, len);
        for (var i=0; i < len; i++) {
            buf[i] = xhr.responseText.charCodeAt(i);
        }
        buf[len-1] = '\0';                     /// * \0 terminated str
        return len;
        }, fn, BUF);
    if (rst==0) return 0;                      /// * fetch failed, bail
    ///
    /// preserve I/O states, call VM, restore IO states
    ///
    void (*cb)(int, const char*) = fout_cb;    ///< keep output function
    string in; getline(fin, in);               ///< keep input buffers
    fout << ENDL;                              /// * flush output
    
    forth_vm((const char*)&pmem[BUF]);         /// * send script to VM
    
    fout_cb = cb;                              /// * restore output cb
    fin.clear(); fin.str(in);                  /// * restore input
#endif // DO_WASM
    return 0;
}

int  main(int ac, char* av[]) {
    forth_init();
    srand(time(0));
    
#if !DO_WASM
    cout << APP_VERSION << endl;
    string cmd;
    while (getline(cin, cmd)) {         ///> fetch user input
        // printf("cmd=<%s>\n", cmd.c_str());
        forth_vm(cmd.c_str());          ///> execute outer interpreter
    }
    cout << "done!" << endl;
#endif // DO_WASM
    
    return 0;
}
///====================================================================
///
/// WASM/Emscripten ccall interfaces
///
#if DO_WASM
extern "C" {
void forth(int n, char *cmd) { forth_vm(cmd); }
int  vm_base()       { return *base;    }
int  vm_dflt()       { return *dflt;    }
int  vm_ss_idx()     { return ss.idx;   }
int  vm_dict_idx()   { return dict.idx; }
int  vm_mem_idx()    { return pmem.idx; }       // HERE
DU   *vm_ss()        { return &ss[0];   }
char *vm_dict(int i) { return (char*)dict[i].name; }
char *vm_mem()       { return (char*)&pmem[0]; }
}
#endif // DO_WSAM
