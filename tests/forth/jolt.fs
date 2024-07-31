: RGB rot $10 lshift + swap $8 lshift + ;
: V3 create 3 cells allot 0 , ;        \ geometry/vec3
: 3! >r r@ 8 + ! r@ 4 + ! r> ! ;       \ ( x y z a -- )
: BODYSET create 8 cells allot 0 , ;   \ { id,v3,g4 }, 0=EXIT
: .POS! 4 + 3! ;                       \ position ( x y z a -- )
: .ROT! >r r@ 3! r> 28 + ! ;           \ rotation ( x y z w a -- )
: BOX  s" box  %x %p %p" JS ;          \ box     ( color geoms shape --- )
: BALL s" ball %x %p %p" JS ;          \ shphere
: PIPE s" pipe %x %p %p" JS ;          \ cylinder
: PILL s" pill %x %p %p" JS ;          \ capsule
V3 px                                  \ dimensions
BODYSET ps                             \ body setting {id,pos,rot}
1.1 2.2 3.3 px 3!
: one $FF $80 $80 RGB px ps BOX ;
: all 100 for one 500 delay next ;
