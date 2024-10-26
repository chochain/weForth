\ weForth Caster demo
: box                                  \ create a randam shape configuration
  2 2 2   px 3!                           \ w h l
  1 id +!
  id @    ds !                            \ id
  2       ds .T!                          \ box
  10 -3 0  ds .P!                         \ position[x, y, z]
  0 0 0 1 ds .R!                          \ rotation[x, y, z, w]
  0 0 0   ds .V!                          \ linear velocity[x, y, z]
  0 0 0   ds .W!                          \ angular velocity[x, y, z]
  color 3 px DSZ ds
  s" body %x %p %p" JS ;
box
.( Cast.fs loaded )
