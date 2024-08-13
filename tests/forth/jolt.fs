: MESH s" mesh %x %p %p" JS ;          \ mesh floor
: BOX  s" box  %x %p %p" JS ;          \ box     ( color geoms shape --- ), shape = [x,y,z]
: BALL s" ball %x %p %p" JS ;          \ sphere,   shape = [r] )
: PIPE s" pipe %x %p %p" JS ;          \ cylinder, shape = [r,h]
: PILL s" pill %x %p %p" JS ;          \ capsule
: RGB int rot $10 lshift + swap $8 lshift + ;
: V3 create 3 cells allot 0 , ;        \ geometry/vec3
: 3! >r r@ 8 + ! r@ 4 + ! r> ! ;       \ ( x y z a -- )
: BODYSET create 8 cells allot 0 , ;   \ { id,v3,g4 }, 0=EXIT
: .POS! 4 + 3! ;                       \ position ( x y z a -- )
: .ROT! >r r@ 3! r> 28 + ! ;           \ rotation ( x y z w a -- )
variable fg                            \ foreground color
V3       px                            \ [x, y, z]
BODYSET  ps                            \ body setting [id,pos[],rot[]]
1.1 2.2 3.3 px 3!
$FF $80 $80 RGB fg !
: one fg @ px ps BOX ;
: ten 9 for one 250 delay next ;
: shoot 9 for ten i 10 * . cr next ;
fg @ px ps MESH
.( JOLT loaded )
