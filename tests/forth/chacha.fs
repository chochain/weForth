create xt $40 allot                  \ 64 bytes
: a@ ( n a -- ) swap th @ ;          \ fetch array[n]
: 4@ ( a b c d s -- sa sb sc sd )
  >r
  2swap i a@ swap i  a@ swap
  2swap i a@ swap r> a@ swap ;
: 4! ( a b c d s -- )
;
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
  >r ( keep n  ) swap over + dup
  >r ( keep a' ) swap
  2swap  ( a' b c d )
  r> xor ( a' b c d^a' ) r> rol ;
  
create t0 $11111111 , $01020304 , $9b8d6f43 , $01234567 ,
\ o  a = 0x11111111
\ o  b = 0x01020304
\ o  d = 0x01234567
\ o  a = a + b = 0x11111111 + 0x01020304 = 0x12131415
\ o  d = d ^ a = 0x01234567 ^ 0x12131415 = 0x13305172
\ o  d = d<<<16 = 0x51721330

: qround ( a b c d -- a b c d )
  2swap  ( cdab )
  $10 q4 ( abcd ) $C q4 ( cdab )
  $8  q4 ( abcd ) $7 q4 ( cdab )
  2swap ;

\ qround expected results
\  o  a = $ea2a92f4
\  o  b = $cb1cf8ce
\  o  c = $4581472e
\  o  d = $5881c4bb  
0 1 2 3 t0 4@ qround

: array create here , cells allot does> @ swap cells + ;
$16 array st
  
