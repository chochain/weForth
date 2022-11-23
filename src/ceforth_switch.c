#define CODE1(n, s, g) case n: { g; } break
#define IMMD1(n, s, g) case n: break

inline void dispatch(int op) {
    //printf("do_op(%d)\n", op);
    ///
    /// @defgroup Execution flow ops
    /// @brief - DO NOT change the sequence here (see forth_opcode enum)
    /// @{
    switch (op) {
    ///
    /// @defgroup Execution flow ops
    /// @brief - DO NOT change the sequence here (see forth_opcode enum)
    /// @{
    CODE1(0, "exit",    IP = rs.pop(); WP = rs.pop());      // handled in nest()
    CODE1(1, "donext",                                      // handled in nest()
         if ((rs[-1] -= 1) >= 0) IP = *(IU*)MEM(IP);        // rs[-1]-=1 saved 200ms/1M cycles
         else { IP += sizeof(IU); rs.pop(); });
    CODE1(2, "dovar",   PUSH(IP);            IP += sizeof(DU));
    CODE1(3, "dolit",   PUSH(*(DU*)MEM(IP)); IP += sizeof(DU));
    CODE1(4, "dostr",
        const char *s = (const char*)MEM(IP);      // get string pointer
        PUSH(IP); IP += STRLEN(s));
    CODE1(5, "dotstr",
        const char *s = (const char*)MEM(IP);      // get string pointer
        fout << s;  IP += STRLEN(s));              // send to output console
    CODE1(6,"branch" , IP = *(IU*)MEM(IP));           // unconditional branch
    CODE1(7,"0branch", IP = POP() ? IP + sizeof(IU) : *(IU*)MEM(IP)); // conditional branch
    CODE1(8,"does",                                   // CREATE...DOES... meta-program
         IU *ip = (IU*)MEM(PFA(WP));
         while (*ip != DOES) ip++;                 // find DOES
         while (*++ip) add_iu(*ip));               // copy&paste code
    CODE1(9, ">r",   rs.push(POP()));
    CODE1(10,"r>",   PUSH(rs.pop()));
    CODE1(11,"r@",   PUSH(rs[-1]));
    /// @}
    /// @defgroup Stack ops
    /// @brief - opcode sequence can be changed below this line
    /// @{
    CODE1(12,"dup",  PUSH(top));
    CODE1(13,"drop", top = ss.pop());
    CODE1(14,"over", PUSH(ss[-1]));
    CODE1(15,"swap", DU n = ss.pop(); PUSH(n));
    CODE1(16,"rot",  DU n = ss.pop(); DU m = ss.pop(); ss.push(n); PUSH(m));
    CODE1(17,"pick", DU i = top; top = ss[-i]);
    /// @}
    /// @defgroup Stack ops - double
    /// @{
    CODE1(18,"2dup", PUSH(ss[-1]); PUSH(ss[-1]));
    CODE1(19,"2drop",ss.pop(); top = ss.pop());
    CODE1(20,"2over",PUSH(ss[-3]); PUSH(ss[-3]));
    CODE1(21,"2swap",
        DU n = ss.pop(); DU m = ss.pop(); DU l = ss.pop();
        ss.push(n); PUSH(l); PUSH(m));
    /// @}
    /// @defgroup ALU ops
    /// @{
    CODE1(22,"+",    top += ss.pop());
    CODE1(23,"*",    top *= ss.pop());
    CODE1(24,"-",    top =  ss.pop() - top);
    CODE1(25,"/",    top =  ss.pop() / top);
    CODE1(26,"mod",  top =  ss.pop() % top);
    CODE1(27,"*/",   top =  (DU2)ss.pop() * ss.pop() / top);
    CODE1(28,"/mod",
        DU n = ss.pop(); DU t = top;
        ss.push(n % t); top = (n / t));
    CODE1(29,"*/mod",
        DU2 n = (DU2)ss.pop() * ss.pop();
        DU2 t = top;
        ss.push((DU)(n % t)); top = (DU)(n / t));
    CODE1(30,"and",  top = ss.pop() & top);
    CODE1(31,"or",   top = ss.pop() | top);
    CODE1(32,"xor",  top = ss.pop() ^ top);
    CODE1(33,"abs",  top = abs(top));
    CODE1(34,"negate", top = -top);
    CODE1(35,"max",  DU n=ss.pop(); top = (top>n)?top:n);
    CODE1(36,"min",  DU n=ss.pop(); top = (top<n)?top:n);
    CODE1(37,"2*",   top *= 2);
    CODE1(38,"2/",   top /= 2);
    CODE1(39,"1+",   top += 1);
    CODE1(40,"1-",   top -= 1);
    /// @}
    /// @defgroup Logic ops
    /// @{
    CODE1(41,"0= ",  top = BOOL(top == 0));
    CODE1(42,"0<",   top = BOOL(top <  0));
    CODE1(43,"0>",   top = BOOL(top >  0));
    CODE1(44,"=",    top = BOOL(ss.pop() == top));
    CODE1(45,">",    top = BOOL(ss.pop() >  top));
    CODE1(46,"<",    top = BOOL(ss.pop() <  top));
    CODE1(47,"<>",   top = BOOL(ss.pop() != top));
    CODE1(48,">=",   top = BOOL(ss.pop() >= top));
    CODE1(49,"<=",   top = BOOL(ss.pop() <= top));
    /// @}
    /// @defgroup IO ops
    /// @{
    CODE1(50,"base@",   PUSH(base));
    CODE1(51,"base!",   fout << setbase(base = POP()));
    CODE1(52,"hex",     fout << setbase(base = 16));
    CODE1(53,"decimal", fout << setbase(base = 10));
    CODE1(54,"cr",      fout << ENDL);
    CODE1(55,".",       fout << POP() << " ");
    CODE1(56,".r",      DU n = POP(); dot_r(n, POP()));
    CODE1(57,"u.r",     DU n = POP(); dot_r(n, abs(POP())));
    CODE1(58,"key",     PUSH(next_idiom()[0]));
    CODE1(59,"emit",    char b = (char)POP(); fout << b);
    CODE1(60,"space",   fout << " ");
    CODE1(61,"spaces",  for (int n = POP(), i = 0; i < n; i++) fout << " ");
    /// @}
    /// @defgroup Literal ops
    /// @{
    CODE1(62,"[",       compile = false);
    CODE1(63,"]",       compile = true);
    IMMD1(64,"(",       scan(')'));
    IMMD1(65,".(",      fout << scan(')'));
    CODE1(66,"\\",      scan('\n'));
    CODE1(67,"$\"",
        const char *s = scan('"')+1;        // string skip first blank
        add_w(DOSTR);                       // dostr, (+parameter field)
        add_str(s));                        // byte0, byte1, byte2, ..., byteN
    IMMD1(68,".\"",
        const char *s = scan('"')+1;        // string skip first blank
        add_w(DOTSTR);                      // dostr, (+parameter field)
        add_str(s));                        // byte0, byte1, byte2, ..., byte
    /// @}
    /// @defgroup Branching ops
    /// @brief - if...then, if...else...then
    /// @{
    IMMD1(69,"if",      add_w(ZBRAN); PUSH(HERE); add_iu(0));       // if    ( -- here )
    IMMD1(70,"else",                                                // else ( here -- there )
        add_w(BRAN);
        IU h=HERE; add_iu(0); SETJMP(POP()); PUSH(h));
    IMMD1(71,"then",    SETJMP(POP()));                             // backfill jump address
    /// @}
    /// @defgroup Loops
    /// @brief  - begin...again, begin...f until, begin...f while...repeat
    /// @{
    IMMD1(72,"begin",   PUSH(HERE));
    IMMD1(73,"again",   add_w(BRAN);  add_iu(POP()));               // again    ( there -- )
    IMMD1(74,"until",   add_w(ZBRAN); add_iu(POP()));               // until    ( there -- )
    IMMD1(75,"while",   add_w(ZBRAN); PUSH(HERE); add_iu(0));       // while    ( there -- there here )
    IMMD1(76,"repeat",  add_w(BRAN);                                // repeat    ( there1 there2 -- )
        IU t=POP(); add_iu(POP()); SETJMP(t));                  // set forward and loop back address
    /// @}
    /// @defgrouop For loops
    /// @brief  - for...next, for...aft...then...next
    /// @{
    IMMD1(77,"for" ,    add_w(TOR); PUSH(HERE));                    // for ( -- here )
    IMMD1(78,"next",    add_w(DONEXT); add_iu(POP()));              // next ( here -- )
    IMMD1(79,"aft",                                                 // aft ( here -- here there )
        POP(); add_w(BRAN);
        IU h=HERE; add_iu(0); PUSH(HERE); PUSH(h));
    /// @}
    /// @defgrouop Compiler ops
    /// @{
    CODE1(80,":", colon(next_idiom()); compile=true);
    IMMD1(81,";", add_w(EXIT); compile = false);
    CODE1(82,"variable",                                         // create a variable
        colon(next_idiom());                                     // create a new word on dictionary
        add_w(DOVAR);                                            // dovar (+parameter field)
        int n = 0; add_du(n);                                    // data storage (32-bit integer now)
        add_w(EXIT));
    CODE1(83,"constant",                                         // create a constant
        colon(next_idiom());                                     // create a new word on dictionary
        add_w(DOLIT);                                            // dovar (+parameter field)
        add_du(POP());                                           // data storage (32-bit integer now)
        add_w(EXIT));
    /// @}
    /// @defgroup metacompiler
    /// @brief - dict is directly used, instead of shield by macros
    /// @{
    CODE1(84,"exec",  CALL(POP()));                              // execute word
    CODE1(85,"create",
        colon(next_idiom());                                     // create a new word on dictionary
        add_w(DOVAR));                                           // dovar (+ parameter field)
    CODE1(86,"to",              // 3 to x                        // alter the value of a constant
        IU w = find(next_idiom());                               // to save the extra @ of a variable
        *(DU*)(PFA(w) + sizeof(IU)) = POP());
    CODE1(87,"is",              // ' y is x                      // alias a word
        IU w = find(next_idiom());                               // copy entire union struct
        dict[POP()].xt = dict[w].xt);
    CODE1(88,"[to]",            // : xx 3 [to] y ;               // alter constant in compile mode
        IU w = *(IU*)MEM(IP); IP += sizeof(IU);                  // fetch constant pfa from 'here'
        *(DU*)MEM(PFA(w) + sizeof(IU)) = POP());
    ///
    /// be careful with memory access, especially BYTE because
    /// it could make access misaligned which slows the access speed by 2x
    ///
    CODE1(89,"@",     IU w = POP(); PUSH(CELL(w)));                  // w -- n
    CODE1(90,"!",     IU w = POP(); CELL(w) = POP());                // n w --
    CODE1(91,",",     DU n = POP(); add_du(n));
    CODE1(92,"allot", DU v = 0; for (IU n = POP(), i = 0; i < n; i++) add_du(v)); // n --
    CODE1(93,"+!",    IU w = POP(); CELL(w) += POP());               // n w --
    CODE1(94,"?",     IU w = POP(); fout << CELL(w) << " ");         // w --
    /// @}
    /// @defgroup Debug ops
    /// @{
    CODE1(95,"here",  PUSH(HERE));
    CODE1(96,"ucase", ucase = POP());
    CODE1(97,"'",     IU w = find(next_idiom()); PUSH(w));
    CODE1(98,".s",    ss_dump());
    CODE1(99,"words", words());
    CODE1(100,"see",
        IU w = find(next_idiom());
        fout << "[ "; to_s(w);
        if (dict[w].def) see(PFA(w));                            // recursive call
        fout << "]" << ENDL);
    CODE1(101,"dump",  DU n = POP(); IU a = POP(); mem_dump(a, n));
    CODE1(102,"peek",  DU a = POP(); PUSH(PEEK(a)));
    CODE1(103,"poke",  DU a = POP(); POKE(a, POP()));
    CODE1(104,"forget",
        IU w = find(next_idiom());
        if (w<0) return;
        IU b = find("boot")+1;
        dict.clear(w > b ? w : b));
    CODE1(105,"clock", PUSH(millis()));
    CODE1(106,"delay", delay(POP()));
    /// @}
    CODE1(107,"bye",   exit(0));
    CODE1(108,"boot",  dict.clear(find("boot") + 1); pmem.clear());
    default: break;
    }
}
