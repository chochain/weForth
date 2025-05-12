\
\ ChaCha20 implementation - packed
\
: a@  ( n arr -- arr[n] ) swap th @ ; \ fetch array[n]
: a!  ( v n arr -- ) swap th ! ;      \ array[n] = v
: a+! ( v n arr -- ) swap th +! ;     \ array[n] += v
: a4@ ( dcba arr -- v dcb arr )
  >r ( keep arr )
  dup 4 rshift swap ( dcb dcba )
  $f and i a@ swap  ( v dcb )
  r> ;
: 4@ ( dcba arr -- va vb vc vd )      \ dcba 4-nibble indices
  3 for a4@ next 2drop ;
: 4! ( dcba va vb vc vd arr -- )
  3 for
    swap over ( ..arr vd arr )
    4 i + pick i 4 * rshift ( ..vd arr d )
    $f and swap a! ( dcba va vb vc arr )
  next 2drop ;
\
\ ChaCha20 utilities  
\ ROL = v << n | (v >> ($20 - n))
\
: rol ( v n -- v' )                    \ rotate left
  >r dup r@ lshift swap
  $20 r> - rshift or ;
: hx ( c d a b n -- a' b c d' )        \ one hash line
  >r ( keep u  ) swap over + dup
  >r ( keep a' ) swap
  2swap  ( a' b c d )
  r> xor ( a' b c d^a' ) r> rol ;
: qround ( a b c d -- a b c d )        \ quarter round
  2swap  ( c d a b )
  $10 hx ( a b c d ) $C hx ( c d a b )
  $8  hx ( a b c d ) $7 hx ( c d a b )
  2swap ;
  
\ test cases
create t0 $11111111 , $01020304 , $9b8d6f43 , $01234567 ,
\ a = 0x11111111, b = 0x01020304, d = 0x01234567
\ qround expect > abcd = $ea2a92f4 $cb1cf8ce $4581472e $5881c4bb  
\ $3210 t0 4@ qround
  
\ ChaCha20 core
create st                             \ st[16]
  $61707865 , $3320646e , $79622d32 , $6b206574 ,
  $03020100 , $07060504 , $0b0a0908 , $0f0e0d0c ,
  $13121110 , $17161514 , $1b1a1918 , $1f1e1d1c ,
  $00000001 , $09000000 , $4a000000 , $00000000 ,
create gold                           \ expected xt after one_block
  $e4e7f110 , $15593bd1 , $1fdd0f50 , $c47120a3 ,
  $c7f4d1c7 , $0368c033 , $9aaa2204 , $4e6cd4c3 ,
  $466482d2 , $09aa9f07 , $05d7c214 , $a2028bd9 ,
  $d19c12b5 , $b94e16de , $e883d0cb , $4e3c50a2 ,
create xt $40 allot                   \ 64-byte tmp calc array

: st2xt ( -- )                        \ st := xt 
  $F for i st a@ i xt a! next ;    
: xt+=st ( -- )                       \ xt += st
  $F for i st a@ i xt a+! next ;
: quarter ( xxxxdcba -- )             \ one full round
  dup xt 4@ qround xt 4! ;
create hidx                           \ quater round indices
  $e943d872 , $cb61fa50 ,             \ diag 2, 3, 0, 1
  $fb73ea62 , $d951c840 ,             \ col  2, 3, 0, 1
: odd_even ( -- )
  3 for
    i hidx a@ ( hgfedcba )
    dup        quarter                \ low  round
    $10 rshift quarter                \ high round
  next ;
: one_block ( -- )
  st2xt
  9 for odd_even next                 \ 10x2 rounds
  xt+=st ;
: check ( -- )
  one_block
  $F for
    i xt a@ i gold a@
    <> if i . ." miss " then
  next ." done " ;
: qbench
  clock negate
  $3210 t0 4@
  99999 for qround next
  2drop 2drop
  clock + ;
: bench
  clock negate
  999 for one_block next
  clock + ;

  
