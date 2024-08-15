\ weForth JOLT demo
: RGB int rot $10 lshift + swap $8 lshift + ;
: V3 create 3 cells allot ;            \ geometry/vec3
: 3! >r r@ 8 + ! r@ 4 + ! r> ! ;       \ ( x y z a -- )
: 4! >r r@ 12 + ! r> 3! ;              \ ( x y z w a -- )
\ shape data structure
: DYNASET create 14 cells allot ;      \ ( "name" -- [ t, pos[3], rot[4], v[3], av[3] ] )
: .POS! 4 + 3! ;                       \ position ( x y z a -- )
: .ROT! 16 + 4! ;                      \ rotation ( x y z w a -- )
: .V!   32 + 3! ;                      \ linear velocity  ( x y z a -- )
: .AV!  44 + 3! ;                      \ angular velocity ( x y z a -- )
\ variables
6.2832  constant 2PI
V3      px                             \ [x0, x1, x2], parameters for shapes
DYNASET ds                             \ dynamic setting [id,pos[3],rot[4],v[3],av[3]]
create  fg 6 cells n,                  \ designated colors, n manually added
$c0f0c0 , $f04040 , $a0a0f0 ,
$80f080 , $f0d080 , $f0a0f0 , 0 n,     \ 0=EXIT, manually added
\ randomized parameters
: rx 2* rnd 0.5 - * ;                  \ ( n -- n' ) random with range [-n, n)
: rnd_bdy
  rnd 5 * 1+ int ds !                  \ randam shape 1:box, 2:ball, 3:pipe, 4:pill, 5:dumbbell
  rnd 0.5 + rnd 0.5 + rnd 0.5 + px 3! ;     \ random parameters x0, x1, x2
: rnd_geo                              \ random geometry
  0 0 0 ds .POS!                            \ position   x, y, z
  rnd rnd rnd rnd 2PI * ds .ROT! ;          \ rotation   x, y, z, w
: rnd_v 3 rx rnd 10 * 10 + 3 rx ds .V! ;    \ velocity   x, y, z
: MESH                                 \ mesh floor
  30 1 0.8 px 3!                       \ 30x30 mesh with cell size 1, max height=0.8
  0 -5 0  ds .POS!                     \ position xyz: (0,-5,0)
  0 0 0 1 ds .ROT!                     \ rotation (xyzw: 0,0,0,-1)
  fg @ 3 px 14 ds                      \ get color, gemoetry, shape config
  s" mesh %x %p %p" JS ;               \ foreward to front-end thread
MESH                                   \ create a mesh floor
\ random shapes
: one                                  \ create one random body
  rnd_bdy rnd_geo rnd_v                \ create random shape, geometry, velocity
  fg ds @ cells + @ 3 px 14 ds         \ get color, geometry, shape config
  s" body %x %p %p" JS ;               \ foreward to front-end thread
: ten 9 for one 250 delay next ;
: rain 9 for ten i 10 * . cr next ;
\ shooter
.( JOLT loaded )
