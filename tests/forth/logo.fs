: RGB rot $10 lshift + swap $8 lshift + ;
: CS s" cs" JS ;
: HT s" ht" JS ;
: ST s" st" JS ;
: CT s" ct" JS ;
: PD s" pd" JS ;
: PU s" pu" JS ;
: HD s" hd %" JS ;
: FD s" fd %" JS ;
: BK s" bk %" JS ;
: RT s" rt %" JS ;
: LT s" lt %" JS ;
: PC s" pc %" JS ;
: FG RGB s" fg %" JS ;
: BG RGB s" bg %" JS ;
: PW s" pw %" JS ;
: XY $10 lshift swap $ffff and or s" xy %" JS ;
.( LOGO loaded )

