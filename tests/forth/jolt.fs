\ weForth demo - Jolt Core
6.2832 constant 2PI
: rad ( d -- r ) 2PI * 360 / ;          \ radian => degree
: V3 create 3 cells allot ;             \ geometry/vec3
: 3! >r r@ 2 th ! r@ 1 th ! r> ! ;      \ ( x y z a -- )
: 4! >r r@ 3 th ! r> 3! ;               \ ( x y z w a -- )
\ struct/field constructor
0 constant struct
: field ( struct-sz fsz -- )
  create over , + does> @ + ;
\ variables
V3 px                                   \ [x0,x1,x2], params for shapes
struct                                  \ anonymous struct
  1 cells field .id                       \ object id
  1 cells field .type                     \ type or color
  3 cells field .pos                      \ position [x, y, z]
  4 cells field .rot                      \ rotation [x, y, z, w]
  3 cells field .lv                       \ linear velocity [x, y, z]
  3 cells field .av                       \ angular velociy [x, y, z]
create ds allot                         \ create DYNASET
here ds - 1 cells / constant DSZ        \ size of DYNASET (15 cells)
\ create sandbox (id=0, shape=0)
: sandbox
  40 1 0.8 px 3!                        \ 40x40 mesh with cell size 1, max height=0.8
  0        ds .id   !                     \ id = 0
  0        ds .type !                     \ shape = 0
  0 -5 0   ds .pos  3!                    \ pos xyz=[0,-5,0]
  0 0 0 1  ds .rot  4!                    \ rot xyzw=[0,0,0,1]
  $f0fff0 3 px DSZ ds                     \ color, gemoetry, shape config
  s" sandbox %x %p %p" JS ;            \ foreward to front-end thread
.( Jolt.fs loaded )
