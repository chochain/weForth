\ weForth demo - Jolt Core
: V3 create 3 cells allot ;            \ geometry/vec3
: 3! >r r@ 8 + ! r@ 4 + ! r> ! ;       \ ( x y z a -- )
: 4! >r r@ 12 + ! r> 3! ;              \ ( x y z w a -- )
\ shape types and default colors
6.2832 constant 2PI                    \ 2*PI (for degree => radian calc)
6      constant SMAX                   \ shape 1:ball, 2:box, 3:cynlinder,
                                       \       4:capsule, 5:tapered capsule, 6:dumbbell
15 constant DSZ                        \ size of DYNASET (15 cells)
: DYNASET create DSZ cells allot ;     \ ( "name" -- [ id, t, pos[3], rot[4], v[3], av[3] ] )
: .T! 4 + ! ;                          \ set shape type
: .P! 8 + 3! ;                         \ position ( x y z a -- )
: .R! 20 + 4! ;                        \ rotation ( x y z w a -- )
: .V! 36 + 3! ;                        \ linear velocity  ( x y z a -- )
: .W! 48 + 3! ;                        \ angular velocity ( x y z a -- )
\ variables
V3       px                            \ [x0, x1, x2], parameters for shapes
DYNASET  ds                            \ dynamic setting [id,pos[3],rot[4],v[3],av[3]]
create fg                              \ default colors for mesh and shapes
  $f0fff0 , $f04040 , $a0a0f0 , $f0ff40 ,
  $80f080 , $f0d080 , $f0a0f0 ,
: color ds 4 + @ cells fg + @ ;        \ fetch shape color (defined in ds[1])
\ create mesh floor (id=0, shape=0)
: sandbox
  40 1 0.8 px 3!                       \ 40x40 mesh with cell size 1, max height=0.8
  0        ds !                            \ id = 0
  0        ds .T!                          \ shape = 0
  0 -5 0   ds .P!                          \ pos xyz=[0,-5,0]
  0 0 0 1  ds .R!                          \ rot xyzw=[0,0,0,1]
  color 3 px DSZ ds                    \ get color[0], gemoetry, shape config
  s" mesh %x %p %p" JS ;               \ foreward to front-end thread
.( Jolt.fs loaded )
