\ weForth demo - Falling Objects
s" forth/jolt.fs" included             \ load jolt core
sandbox                                \ display meshed floor
\ randomized parameters
: rx 2* rnd 0.5 - * ;                  \ ( n -- n' ) random with range [-n, n)
: rnd_bdy ( id -- ) ds !               \ create a randam shape config with given id
  rnd SMAX * 1+ int ds .T!                 \ shape
  rnd 0.5 + rnd 0.5 + rnd 0.5 + px 3! ;    \ x0, x1, x2
: rnd_geo                              \ random geometry (jump up from [0,0,0])
  0 0 0                         ds .P!     \ position[x, y, z]
  rnd rnd rnd rnd 2PI *         ds .R! ;   \ rotation[x, y, z, w]
: rnd_v 3 rx rnd 10 * 10 + 3 rx ds .V! ;   \ linear velocity[x, y, z]
: rnd_w 1 rx rnd           1 rx ds .W! ;   \ angular velocity[x, y, z]
\ random shapes creation
variable id 0 id !                     \ object id
: one ( id -- )                        \ create one random body ( id should > 0 )
  1 id +! id @                         \ fetch id
  rnd_bdy rnd_geo rnd_v rnd_w          \ create random shape, geometry, velocities
  color 3 px DSZ ds                    \ get color, geometry, shape config
  s" body %x %p %p" JS ;               \ foreward to front-end thread
: ten  9 for one 250 delay next ;
: spit 9 for ten i 10 * . cr next ;
\ shape removal
: remove ( id -- ) s" drop %d" JS ;    \ remove body with given id from scene
: wipe
  99 for i 1+ remove 100 delay next ;
.( shapes.fs loaded )