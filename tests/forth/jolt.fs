\ weForth JOLT demo
: V3 create 3 cells allot ;            \ geometry/vec3
: 3! >r r@ 8 + ! r@ 4 + ! r> ! ;       \ ( x y z a -- )
: 4! >r r@ 12 + ! r> 3! ;              \ ( x y z w a -- )
\ shape types and default colors
6.2832 constant 2PI                    \ 2*PI (for degree => radian calc)
: rad ( d -- r ) 2PI * 360 / ;         
6      constant SMAX                   \ shape 1:box, 2:ball, 3:cynlinder,
                                       \       4:capsule, 5:tapered capsule, 6:dumbbell
create fg
  $f0fff0 , $f04040 , $a0a0f0 , $f0ff40 ,
  $80f080 , $f0d080 , $f0a0f0 ,
15 constant DSZ                        \ size of DYNASET (15 cells)
: DYNASET create DSZ cells allot ;     \ ( "name" -- [ id, t, pos[3], rot[4], v[3], av[3] ] )
: .T! 4 + ! ;                          \ set shape type
: .P! 8 + 3! ;                         \ position ( x y z a -- )
: .R! 20 + 4! ;                        \ rotation ( x y z w a -- )
: .V! 36 + 3! ;                        \ linear velocity  ( x y z a -- )
: .W! 48 + 3! ;                        \ angular velocity ( x y z a -- )
\ variables
variable id 0 id !                     \ body id counter
V3       px                            \ [x0, x1, x2], parameters for shapes
DYNASET  ds                            \ dynamic setting [id,pos[3],rot[4],v[3],av[3]]
: color ds 4 + @ cells fg + @ ;        \ fetch shape color
\ randomized parameters
: rx 2* rnd 0.5 - * ;                  \ ( n -- n' ) random with range [-n, n)
: rnd_bdy                              \ create a randam shape configuration
  1 id +! id @ ds !                        \ id
  rnd SMAX * 1+ int ds .T!                 \ shape
  rnd 0.5 + rnd 0.5 + rnd 0.5 + px 3! ;    \ x0, x1, x2
: rnd_geo                              \ random geometry (jump up from [0,0,0])
  0 0 0 ds .P!                             \ position[x, y, z]
  rnd rnd rnd rnd 2PI * ds .R! ;           \ rotation[x, y, z, w]
: rnd_v 3 rx rnd 10 * 10 + 3 rx ds .V! ;   \ linear velocity[x, y, z]
: rnd_w 1 rx rnd           1 rx ds .W! ;   \ angular velocity[x, y, z]
\ create mesh floor (id=0, shape=0)
: mesh
  40 1 0.8 px 3!                       \ 40x40 mesh with cell size 1, max height=0.8
  0 -5 0  ds .P! 0 0 0 1 ds .R!        \ pos xyz=[0,-5,0], rot xyzw=[0,0,0,1]
  color 3 px DSZ ds                    \ get color[0], gemoetry, shape config
  s" mesh %x %p %p" JS ;               \ foreward to front-end thread
mesh
\ random shapes creation
: one                                  \ create one random body
  rnd_bdy rnd_geo rnd_v rnd_w          \ create random shape, geometry, velocities
  color 3 px DSZ ds                    \ get color, geometry, shape config
  s" body %x %p %p" JS ;               \ foreward to front-end thread
: ten  9 for one 250 delay next ;
: spit 9 for ten i 10 * . cr next ;
\ shape removal
: remove s" drop %d" JS ;              \ ( id -- ) remove body from scene
: skew
  99 for i 1+ remove 100 delay next ;
1000 constant ID
: bike
  0.4 0.5 0.8 px 3!                    \ bike body px[width, height, length]
  ID ds !                              \ bike id
  0 0 0 ds .P! 0 0 0 1 ds .R!          \ pos[3], rot[4]
  $00ff00 3 px DSZ ds                  \ create bike body
  s" bike %x %p %p" JS
  150 10000 1000 px 3!                 \ set engine params
  ID 3 px s" engine %x %p" JS
  2 8000 2000 px 3!                    \ set transmission params
  ID 3 px s" gearbox %x %p" JS ;
: wheel ( -- )
  ID 3 px DSZ ds                       \ create front wheel
  s" wheel %x %p %p" JS ;
: front_wheel ( -- )
  0              ds !                  \ front wheel
  0 -0.2 0.65    ds .P!                \ pos[x,y,z]
  1.5 0.3 0.5    ds .V!                \ suspension[freq, min, max]
  30 rad dup 500 ds .W!                \ steering, caster, break strength
  0.3 0.3 0.08 px 3!                   \ wheel dim[r1, r2, width]
  wheel ;
: back_wheel ( -- )
  1              ds !                  \ back wheel
  0 -0.2 -0.95    ds .P!               \ pos[x,y,z]
  2.0 0.3 0.5    ds .V!                \ suspension[freq, min, max]
  0 dup 250      ds .W!                \ steering[ang, caster, break]
  wheel ;                              \ same dim as front wheel
: car
  1.2 0.3 2.0 px 3!                    \ car body px[width, height, length]
  ID ds !                              \ body id
  0 10 0 ds .P! 0 0 0 1 ds .R!         \ pos[3], rot[4]
  $00ff00 3 px DSZ ds                  \ create car body
  s" fwd %x %p %p" JS
  800 10000 1000 px 3!                 \ set engine params
  ID 3 px s" engine %x %p" JS
  2 8000 2000 px 3!                    \ set transmission params
  ID 3 px s" gearbox %x %p" JS ;
: car_wheels ( -- )
  0              ds !                  \ front wheel
  0.8 0.1 1.2    ds .P!                \ pos[x,y,z]
  1.5 0.3 0.5    ds .V!                \ suspension[freq, min, max]
  30 rad dup 500 ds .W!                \ steering, caster, break strength
  0.3 0.3 0.1    px 3!                 \ wheel dim[r1, r2, width]
  wheel
  1              ds !                  \ back wheel
  -0.8 0.1 1.2   ds .P!                \ pos[x,y,z]
  wheel                                \ same dim as front wheel
  2              ds !                  \ front wheel
  0.8 0.1 -1.2   ds .P!                \ pos[x,y,z]
  0 dup 500      ds .W!                \ steering, caster, break strength
  wheel
  3              ds !                  \ back wheel
 -0.8 0.1 -1.2   ds .P!                \ pos[x,y,z]
  wheel ;                              \ same dim as front wheel
: start s" start" JS ;
: go_bike
  bike front_wheel back_wheel
  start ;
: go_car
  car car_wheels
  start ;
.( JOLT loaded )
