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
#define MEM(ip)   (MEM0 + (IU)(ip))        /**< pointer to IP address fetched from pmem */
#define CELL(a)   (*(DU*)&pmem[a])         /**< fetch a cell from parameter memory      */
#define SETJMP(a) (*(IU*)&pmem[a] = HERE)  /**< address offset for branching opcodes    */
///@}
///@name Global memory access macros
///@brief macros for ESP memory space access (be very careful of these)
///@note  4000_0000 is instruction bus, access needs to be 32-bit aligned
///       3fff_ffff and below is data bus, no alignment requirement
///@{
#define PEEK(a)    (DU)(*(DU*)((UFP)(a)))
#define POKE(a, c) (*(DU*)((UFP)(a))=(DU)(c))
///@}
///
/// enum used for built-in opcode tokens to simplify compiler
/// i.g, add_w(find("_donext")) can be reduced to add_w(DONEXT)
/// but make sure the sequence is in-sync with word list in dict_compile
///
typedef enum {
    EOW = 0xffff & ~UDF_ATTR,        ///< token for end of colon word
    EXIT=0, DONEXT, DOLIT, DOVAR, DOSTR, DOTSTR, BRAN, ZBRAN, DODOES, TOR
} forth_opcode;
///
///====================================================================
///
///> VM states (single task)
///
DU   top     = -1;      ///< top of stack (cached)
IU   IP      = 0;       ///< current instruction pointer
bool compile = false;   ///< compiler flag
bool ucase   = false;   ///< case sensitivity control
DU   *base;             ///< numeric radix (a pointer)
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
    for (IU i = dict.idx - (compile ? 2 : 1); i > 0; --i) {
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
    bool x  = w==EOW;               ///< adjust end of word token
    Code &c = dict[x ? EXIT : w];   ///< ref to dictionary entry
    IU   ip = c.attr & UDF_ATTR     ///< check whether colon word
        ? (c.pfa | UDF_FLAG)        ///< pfa with colon word flag
        : (x ? EOW : c.xtoff());    ///< XT offset
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
    static IU _NXT = dict[find("_donext")].xtoff();  ///> cache offsets to funtion pointers
    static IU _LIT = dict[find("_dolit")].xtoff();
    int dp = 0;                        ///> iterator implementation (instead of recursive)
    while (dp >= 0) {                  ///> depth control
        IU ix = *(IU*)MEM(IP);         ///> fetch opcode, hopefully cached
        while (ix != EOW) {            ///> fetch till end of word hit
            IP += sizeof(IU);          /// * advance inst. ptr
            if (ix & UDF_FLAG) {       ///> is it a colon word?
                rs.push(IP);
                IP = ix & ~UDF_FLAG;   ///> word pfa (def masked)
                dp++;                  ///> go one level deeper
            }
            else if (ix == _NXT) {     ///> cached DONEXT, DOLIT handlers,
                if ((rs[-1] -= 1) >= 0) IP = *(IU*)MEM(IP); ///> 10% faster on AMD, 5% on ESP32
                else { IP += sizeof(IU); rs.pop(); }        ///> perhaps due to shallow pipeline
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
#include <sstream>                     /// iostream, stringstream
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
char *word() {                            ///< get next idiom
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
        IU h0  = HERE;                 ///> keep current memory addr
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
    auto show_addr = [](IU w, U8 *ip) {
#if CC_DEBUG
        fout << "( " << setfill('0') << setw(4) << (ip - MEM0) << "["; ///> addr
        if (w==EOW) fout << "EOW";
        else        fout << setfill(' ') << setw(3) << w;
        fout << "] ) ";
#endif // CC_DEBUG
    };
    auto jmp = [](const char *n, U8 *ip) {
        fout << n;
#if CC_DEBUG        
        fout << " ( " << setfill('0') << setw(4) << *(IU*)ip << " )";
#endif // CC_DEBUG        
    };
    
    show_addr(w, ip);        ///> display address & opcode
    ip += sizeof(IU);        ///> calculate next ip
    switch (w) {
    case EOW:      fout << ";";                         break;
    case EXIT:     fout << "exit";                      break;
    case DONEXT:   jmp("next", ip);                     break;
    case DOLIT:    fout << *(DU*)ip;                    break;
    case DOVAR:    fout << *(DU*)ip << " ( variable )"; break;
    case DOSTR:    fout << "s\" " << (char*)ip << '"';  break;
    case DOTSTR:   fout << ".\" " << (char*)ip << '"';  break;
    case BRAN:     jmp("bran",  ip);                    break;
    case ZBRAN:    jmp("0bran", ip);                    break;
    case DODOES:   fout << "does>" ;                    break;
    default:       fout << dict[w].name;                break;
    }
    fout << setfill(' ') << setw(-1); ///> restore output format settings
}
void see(IU pfa, int dp=1) {
    auto pfa2opcode = [](IU ix) {                  ///> reverse lookup
        if (ix==EOW) return (int)EOW;              ///> end of word handler
        IU   pfa = ix & ~UDF_FLAG;                 ///> pfa (mask colon word)
        FPTR xt  = Code::XT(pfa);                  ///> lambda pointer
        for (int i = dict.idx - 1; i >= 0; --i) {
            if (ix & UDF_FLAG) {
                if (dict[i].pfa == pfa) return i;  ///> compare pfa in PMEM
            }
            else if (dict[i].xt == xt) return i;   ///> compare xt
        }
        return -1;
    };
    U8 *ip = MEM(pfa);
    while (1) {
        IU w = pfa2opcode(*(IU*)ip);    ///> fetch word index by pfa
        if (w==(IU)-1) break;           ///> loop guard
        
        fout << ENDL; for (int i=dp; i>0; i--) fout << "  "; ///> indent
        to_s(w, ip);                    ///> display opcode
        if (w==EOW) break;              ///> done with the colon word

        ip += sizeof(IU);               ///> advance ip (next opcode)
        switch (w) {                    ///> extra bytes to skip
        case DOLIT:  case DOVAR:  ip += sizeof(DU);        break;
        case DOSTR:  case DOTSTR: ip += STRLEN((char*)ip); break;
        case BRAN:   case ZBRAN:
        case DONEXT: case DODOES: ip += sizeof(IU);        break;
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
#if CC_DEBUG > 1
        if (nm[0]) {
#else  //  CC_DEBUG > 1
        if (nm[0] != '_') {
#endif // CC_DEBUG > 1           
            sz += strlen(nm) + 2;
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
    fout << " <";
    for (int i=0; i<ss.idx; i++) { fout << ss[i] << " "; }
    fout << top << "> ok" << ENDL;
}
void mem_dump(IU p0, DU sz) {
    fout << setbase(16) << setfill('0');
    for (IU i=ALIGN16(p0); i<=ALIGN16(p0+sz); i+=16) {
        fout << setw(4) << i << ": ";
        for (int j=0; j<16; j++) {
            U8 c = pmem[i+j];
            fout << setw(2) << (int)c << (j%4==3 ? " " : "");
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
             << "> attr=" << (c.attr & ~MSK_ATTR)
             << ", xt="   << setw(4) << c.xtoff()
             << ":"       << setw(8) << (UFP)c.xt
             << ", name=" << setw(8) << (UFP)c.name
             << " "       << c.name << ENDL;
    }
    fout << setbase(*base) << setfill(' ') << setw(-1);
}
///====================================================================
///
///> eForth dictionary assembler
///  Note: sequenced by enum forth_opcode as following
///
#if DO_WASM
UFP Code::XT0 =  0;    ///< WASM xt is index to vtable
/// function in worker thread
EM_JS(void, canvas, (const char *arg, U32 v=0), {
        postMessage(['ui', [ UTF8ToString(arg), v]])
    });
#else  // !DO_WASM
UFP Code::XT0 = ~0;    ///< init base of xt pointers (before calling CODE macros)
#endif // DO_WASM

void dict_compile() {  ///< compile primitive words into dictionary
    base = (int*)MEM(pmem.idx);                         ///< set pointer to base
    add_du(10);                                         ///< default radix=10
    ///
    /// @defgroup Execution flow ops
    /// @brief - DO NOT change the sequence here (see forth_opcode enum)
    /// @{
    CODE("_exit",   IP = rs.pop());                     // handled in nest()
    CODE("_donext",                                     // handled in nest()
         if ((rs[-1] -= 1) >= 0) IP = *(IU*)MEM(IP);    // rs[-1]-=1 saved 200ms/1M cycles
         else { IP += sizeof(IU); rs.pop(); });
    CODE("_dolit",  PUSH(*(DU*)MEM(IP)); IP += sizeof(DU));
    CODE("_dovar",  PUSH(IP);            IP += sizeof(DU));
    CODE("_dostr",  const char *s = (const char*)MEM(IP);     // get string pointer
                    IU    len = STRLEN(s);
                    PUSH(IP); PUSH(len); IP += len);
    CODE("_dotstr", const char *s = (const char*)MEM(IP);     // get string pointer
                    fout << s;  IP += STRLEN(s));             // send to output console
    CODE("_bran" ,  IP = *(IU*)MEM(IP));                      // unconditional branch
    CODE("_0bran",  IP = POP() ? IP + sizeof(IU) : *(IU*)MEM(IP)); // conditional branch
    CODE("_dodoes", add_w(BRAN);                              // branch to does> section
                    add_iu(IP + sizeof(IU));                  // encode IP
                    add_w(EOW));                              // not really needed but cleaner
    CODE(">r",      rs.push(POP()));
    CODE("r>",      PUSH(rs.pop()));
    CODE("r@",      PUSH(rs[-1]));                      // same as I (the loop counter)
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
    CODE("mod",     top =  ss.pop() % top);
    CODE("*/",      top =  (DU2)ss.pop() * ss.pop() / top);
    CODE("/mod",    DU n = ss.pop(); DU t = top;
                    ss.push(n % t); top = (n / t));
    CODE("*/mod",   DU2 n = (DU2)ss.pop() * ss.pop();
                    DU2 t = top;
                    ss.push((DU)(n % t)); top = (DU)(n / t));
    CODE("and",     top = ss.pop() & top);
    CODE("or",      top = ss.pop() | top);
    CODE("xor",     top = ss.pop() ^ top);
    CODE("abs",     top = abs(top));
    CODE("negate",  top = -top);
    CODE("invert",  top = ~top);
    CODE("rshift",  top = ss.pop() >> top);
    CODE("lshift",  top = ss.pop() << top);
    CODE("max",     DU n=ss.pop(); top = (top>n)?top:n);
    CODE("min",     DU n=ss.pop(); top = (top<n)?top:n);
    CODE("2*",      top *= 2);
    CODE("2/",      top /= 2);
    CODE("1+",      top += 1);
    CODE("1-",      top -= 1);
    /// @}
    /// @defgroup Logic ops
    /// @{
    CODE("0=",      top = BOOL(top == DU0));
    CODE("0<",      top = BOOL(top <  DU0));
    CODE("0>",      top = BOOL(top >  DU0));
    CODE("=",       top = BOOL(ss.pop() == top));
    CODE(">",       top = BOOL(ss.pop() >  top));
    CODE("<",       top = BOOL(ss.pop() <  top));
    CODE("<>",      top = BOOL(ss.pop() != top));
    CODE(">=",      top = BOOL(ss.pop() >= top));
    CODE("<=",      top = BOOL(ss.pop() <= top));
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
    IMMD("s\"",     s_quote(DOSTR));
    IMMD(".\"",     s_quote(DOTSTR));
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
    /// @defgrouop For loops
    /// @brief  - for...next, for...aft...then...next
    /// @{
/*
  do
  loop
  +loop
*/
    IMMD("for" ,    add_w(TOR); PUSH(HERE));                    // for ( -- here )
    IMMD("next",    add_w(DONEXT); add_iu(POP()));              // next ( here -- )
    IMMD("aft",                                                 // aft ( here -- here there )
         POP(); add_w(BRAN);
         IU h=HERE; add_iu(0); PUSH(HERE); PUSH(h));
    /// @}
    /// @defgrouop Compiler ops
    /// @{
    CODE(":",       compile = def_word(word()));
    IMMD(";",       add_w(EOW); compile = false);
    CODE("exit",    add_w(EXIT));                               // early exit the colon word
    CODE("variable",                                            // create a variable
         if (def_word(word())) {                          // create a new word on dictionary
             add_w(DOVAR);                                      // dovar (+parameter field)
             add_du(DU0);                                       // data storage (32-bit integer now)
             add_w(EOW);
         });
    CODE("constant",                                            // create a constant
         if (def_word(word())) {                          // create a new word on dictionary
             add_w(DOLIT);                                      // dovar (+parameter field)
             add_du(POP());                                     // data storage (32-bit integer now)
             add_w(EOW);
         });
    IMMD("immediate", dict[-1].attr |= IMM_ATTR);
    /// @}
    /// @defgroup metacompiler
    /// @brief - dict is directly used, instead of shield by macros
    /// @{
    CODE("exec",  IU w = POP(); CALL(w));                       // execute word
    CODE("create",                                              // dovar (+ parameter field)
         if (def_word(word())) {                          // create a new word on dictionary
             add_w(DOVAR);
         });
    IMMD("does>", add_w(DODOES); add_w(EOW));                   // CREATE...DOES>... meta-program
    CODE("to",              // 3 to x                           // alter the value of a constant
         IU w = find(word());                             // to save the extra @ of a variable
         *(DU*)(MEM(dict[w].pfa) + sizeof(IU)) = POP());
    CODE("is",              // ' y is x                         // alias a word
         IU w = find(word());                             // copy entire union struct
         dict[POP()].xt = dict[w].xt);
    ///
    /// be careful with memory access, especially BYTE because
    /// it could make access misaligned which slows the access speed by 2x
    ///
    CODE("@",     IU w = POP(); PUSH(CELL(w)));                 // w -- n
    CODE("!",     IU w = POP(); CELL(w) = POP(););              // n w --
    CODE(",",     DU n = POP(); add_du(n));
    CODE("allot", for (IU n = POP(), i = 0; i < n; i++) add_du(DU0)); // n --
    CODE("+!",    IU w = POP(); CELL(w) += POP());              // n w --
    CODE("?",     IU w = POP(); fout << CELL(w) << " ");        // w --
    /// @}
    /// @defgroup Debug ops
    /// @{
    CODE("here",  PUSH(HERE));
    CODE("'",     IU w = find(word()); if (w) PUSH(w));
    CODE(".s",    ss_dump());
    CODE("words", words());
    CODE("see",
         IU w = find(word()); if (!w) return;
         fout << ": " << dict[w].name;
         if (IS_UDF(w)) see(dict[w].pfa);
         else           fout << " ( primitive ) ;";
         fout << ENDL);
    CODE("dump",  DU n = POP(); IU a = POP(); mem_dump(a, n));
    CODE("dict",  dict_dump());
    CODE("peek",  DU a = POP(); PUSH(PEEK(a)));
    CODE("poke",  DU a = POP(); POKE(a, POP()));
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
    CODE("delay", delay(POP()));
    CODE("included",                        // include external file
         POP();                             // string length, not used
         U8 *fn = MEM(POP());               // file name
         forth_include((const char*)fn));   // include file
    /// @}
#ifdef DO_LOGO
    /// @defgroup LOGO ops
    /// @{
    CODE("CS",    canvas("cs"));         // clear screen
    CODE("HT",    canvas("ht"));         // hide turtle
    CODE("ST",    canvas("st"));         // show turtle
    CODE("CT",    canvas("ct"));         // center turtle
    CODE("PD",    canvas("pd"));         // pen down
    CODE("PU",    canvas("pu"));         // pen up
    CODE("HD",    canvas("hd", POP()));  // set heading
    CODE("FD",    canvas("fd", POP()));  // forward
    CODE("BK",    canvas("bk", POP()));  // backward
    CODE("RT",    canvas("rt", POP()));  // right turn
    CODE("LT",    canvas("lt", POP()));  // left turn
    CODE("PC",                           // pencolor
         int c = top | (ss.pop()<<8) | (ss.pop()<<16);
         top = ss.pop();
         canvas("pc", c));
    CODE("PW",    canvas("pw", POP()));  // penwidth
    CODE("XY",                           // set x, y
         int xy = top | (ss.pop()<<16);
         top = ss.pop();
         canvas("xy", xy));
    CODE("JS",    POP();                   // string length, not used
         canvas((const char*)MEM(POP()))); // get string pointer
#endif // DO_LOGO
    CODE("bye",   exit(0));
    /// @}
    CODE("boot",  dict.clear(find("boot") + 1); pmem.clear(sizeof(DU)));
}
///====================================================================
///
///> ForthVM - Outer interpreter
///
DU parse_number(const char *idiom, int *err) {
    int b = *base;
    switch (*idiom) {                        ///> base override
    case '%': b = 2;  idiom++; break;
    case '&':
    case '#': b = 10; idiom++; break;
    case '$': b = 16; idiom++; break;
    }
    char *p;
    *err = errno = 0;
#if DU==float
    DU n = (b==10)
        ? static_cast<DU>(strtof(idiom, &p))
        : static_cast<DU>(strtol(idiom, &p, b));
#else
    DU n = static_cast<DU>(strtol(idiom, &p, b));
#endif
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
        add_w(DOLIT);                    ///> add to current word
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
    if (init) return;          ///> check dictionary initilized
    
    dict_compile();            ///> compile dictionary
}
void forth_vm(const char *cmd, void(*callback)(int, const char*)) {
    fin.clear();               ///> clear input stream error bit if any
    fin.str(cmd);              ///> feed user command into input stream
    fout_cb = callback;        ///> setup callback function
    fout.str("");              ///> clean output buffer, ready for next run
    while (fin >> pad) {       ///> outer interpreter loop
        const char *idiom = pad.c_str();
        forth_core(idiom);     ///> single command to Forth core
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
const char* APP_VERSION = "weForth v4.1";
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
    /* not implemented */
    return 0;
}

int  main(int ac, char* av[]) {
    forth_init();
    return 0;
}
///====================================================================
///
/// WASM/Emscripten ccall interfaces
///
#if DO_WASM
#include <emscripten.h>
extern "C" {
void forth(int n, char *cmd) {
    auto rsp_to_con =
        [](int len, const char *rst) { printf("%s", rst); };
    forth_vm(cmd, rsp_to_con);
}
int  vm_base()       { return *base;    }
int  vm_ss_idx()     { return ss.idx;   }
int  vm_dict_idx()   { return dict.idx; }
DU   *vm_ss()        { return &ss[0];   }
char *vm_dict(int i) { return (char*)dict[i].name; }
char *vm_mem()       { return (char*)&pmem[0]; }
}
#endif // DO_WSAM
