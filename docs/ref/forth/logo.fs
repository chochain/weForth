: RGB rot $10 lshift + swap $8 lshift + ;
: CS s" cs" JS ;
: HT s" ht" JS ;
: ST s" st" JS ;
: CT s" ct" JS ;
: PD s" pd" JS ;
: PU s" pu" JS ;
: HD s" hd %d" JS ;
: FD s" fd %d" JS ;
: BK s" bk %d" JS ;
: RT s" rt %d" JS ;
: LT s" lt %d" JS ;
: PC s" pc %d" JS ;
: FG RGB s" fg %d" JS ;
: BG RGB s" bg %d" JS ;
: PW s" pw %d" JS ;
: XY $10 lshift swap $ffff and or s" xy %d" JS ;
ST
.( LOGO loaded )

