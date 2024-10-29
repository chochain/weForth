\
\ ChaCha20 implementation
\
: array ( n -- )                    \ create an n-element array 
  create here 1 cells + , cells allot
  does> @ swap cells + ;
create st                           \ st[16]
  $61707865 , $3320646e , $79622d32 , $6b206574 ,
  $03020100 , $07060504 , $0b0a0908 , $0f0e0d0c ,
  $13121110 , $17161514 , $1b1a1918 , $1f1e1d1c ,
  $00000001 , $09000000 , $4a000000 , $00000000 ,
create xt $40 allot                 \ xt 64 bytes
  
: a@ ( n a -- a[n] ) swap th @ ;         \ fetch array[n]
: a! ( v n a -- ) rot th ! ;             \ array[n] = v
: 4@ ( a b c d s -- sa sb sc sd )
  >r ( keep s )
  2swap i a@ swap i  a@ swap
  2swap i a@ swap r> a@ swap ;
: 4! ( va vb vc vd a b c d s -- )
  >r ( keep s )
  5 pick i a! 5 pick i  a!
  5 pick i a! 5 pick r> a!
  2drop 2drop ;
\ ROL = v << n | (v >> ($20 - n))
: rol ( v n -- v' )                 \ rotate left
  >r dup r@ lshift swap
  $20 r> - rshift or ;
\ quarter round
\   1.  a += b; d ^= a; d <<<= 16;  a b c d
\   2.  c += d; b ^= c; b <<<= 12;      c d a b
\   3.  a += b; d ^= a; d <<<= 8;           a b c d
\   4.  c += d; b ^= c; b <<<= 7;               c d a b
: q4 ( c d a b n -- a' b c d' )
  >r ( keep u  ) swap over + dup
  >r ( keep a' ) swap
  2swap  ( a' b c d )
  r> xor ( a' b c d^a' ) r> rol ;
: qround ( a b c d -- a b c d )
  2swap  ( cdab )
  $10 q4 ( abcd ) $C q4 ( cdab )
  $8  q4 ( abcd ) $7 q4 ( cdab )
  2swap ;
  
\ test cases
create t0 $11111111 , $01020304 , $9b8d6f43 , $01234567 ,
\ > a = 0x11111111, b = 0x01020304, d = 0x01234567
\ > a = a + b = 0x11111111 + 0x01020304 = 0x12131415
\ > d = d ^ a = 0x01234567 ^ 0x12131415 = 0x13305172
\ > d = d<<<16 = 0x51721330
\ 0 1 2 3 t0 4@ 2swap 16 q4
\ qround expected results
\ > abcd = $ea2a92f4 $cb1cf8ce $4581472e $5881c4bb  
\ 0 1 2 3 t0 4@ qround

: quarter ( a b c d u -- )
  >r ( keep u ) 2over 2over
  r@ 4@ qround r> 4! ;
: round                       \         c0 | c1 | c2 | c3
  0  4  8 $C quarter          \ col  0,  0 |  1 |  2 |  3
  1  5  9 $D quarter          \ col  1,  4 |  5 |  6 |  7
  2  6 $A $E quarter          \ col  2,  8 |  9 | 10 | 11
  3  7 $B $F quarter          \ col  3, 12 | 13 | 14 | 15
  0  5 $A $F quarter          \ diag 0
  1  6 $B $C quarter          \ diag 1
  2  7  8 $D quarter          \ diag 2
  3  4  9 $E quarter ;        \ diag 3
: one_block
  $F for st i th @ xt i th ! next     \ st <= xt
   9 for round next                   \ 10 rounds
  $F for xt i th @ st i th +! next ;  \ xt += st


  
