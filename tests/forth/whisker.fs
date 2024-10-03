\ weForth JOLT demo
s" forth/jolt.fs" included
sandbox
\ vehicle simulator
1000 constant ID                       \ vehicle id
: wheel ( n -- ) ds !                  \ keep wheel index
  ID 3 px DSZ ds                       \ create wheel
  s" wheel %x %p %p" JS ;
: chassis ( -- )
  1.2 0.8 0.8 px 3!                    \ chassis dim[width, height, length]
  ID ds !                              \ car id
  0 10 0 ds .P! 0 0 0 1 ds .R!         \ pos[x,y,z], rot[x,y,z,w]
  $00ff00 3 px DSZ ds                  \ create chassis
  s" fwd %x %p %p" JS ;                \ for front wheel drive
: engine
  1000 10000 1000 px 3!                \ engine[torque,max/min RPMs]
  ID 3 px s" engine %x %p" JS ;
: gearbox
  2 8000 2000 px 3!                    \ transmission[clutch,up,down]
  ID 3 px s" gearbox %x %p" JS ;
: wheels ( -- )
  0.8 0.4 0.2    ds .P!                \ relative pos[x,y,z] to vehicle
  1.5 0.3 0.5    ds .V!                \ suspension[freq, min, max]
  0 dup 500      ds .W!                \ angle[steering, caster], break strength
  0.5 0.5 0.1    px 3!  0 wheel        \ FL wheel dim[r1, r2, width]
  -0.8 0.4 0.2   ds .P! 1 wheel        \ FR wheel, pos[x,y,z], same dim
  0.1 0.2 -1.2   ds .P!                \ pos[x,y,z]
  30 rad dup 500 ds .W!                \ angle[steering, caster], break strength
  0.3 0.3 0.1    px 3!  2 wheel        \ RL wheel dim[r1, r2, width]
  -0.1 0.2 -1.2  ds .P! 3 wheel ;      \ RR wheel, pos[x,y,z], same dim
: start s" start" JS ;                 \ activate current vehicle
: one_bot ( -- )
  chassis engine gearbox wheels
  start ID 1+ to ID ;
: bots ( n -- ) for one_bot next ;
.( whisker.fs loaded )
