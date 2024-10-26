#include <stdint.h>
#include <string.h>

typedef uint8_t  U8;
typedef uint32_t U32;

constexpr int BLK_SZ = 64;                   ///< ChaCha20 is a 512-bit cipher
constexpr int ST_SZ  = (BLK_SZ/sizeof(U32)); ///< states in 32-bit
constexpr int NROUND = 20;                   ///< spec. 20 rounds, 8 was said sufficent

#include <stdio.h>
static void dump(const char *msg, U8 s[], int len) {
    printf("%s:", msg);
    for (int i = 0; i < len; i++) {
        if (i%16==0) {
            printf("\n");
            for (int j = 0; j < 16; j++) {
                char c = i+j < len ? s[i+j] : ' ';
                printf("%c", (c > 31 && c < 127) ? c : '_');
            }
        }
        printf(" %02x", s[i]);
    }
    printf("\n");
}

class ChaCha20 {
public:
    ChaCha20(U8 key[32], U32 counter, U8 nonce[12]);
    void xcrypt(U8 *in, U8 *out, int len);
    
private:
    U32 st[ST_SZ];    ///< states
    U8  xt[BLK_SZ];   ///< pre-alloc temp storage for processing

    inline U32 ROL(U32 v, int n) { return v << n | (v >> (-n & 31)); }
    void _quarter(U32 *x, int a, int b, int c, int d);
    void _one_block(U8 out[BLK_SZ], int nrounds);
    int  _self_diag();
};
///
///> quarterround
///
void ChaCha20::_quarter(U32 *x, int a, int b, int c, int d)
{
    x[a] += x[b]; x[d] = ROL(x[d] ^ x[a], 16);
    x[c] += x[d]; x[b] = ROL(x[b] ^ x[c], 12);
    x[a] += x[b]; x[d] = ROL(x[d] ^ x[a],  8);
    x[c] += x[d]; x[b] = ROL(x[b] ^ x[c],  7);
}
///
///> process one block (64-byte)
///
void ChaCha20::_one_block(U8 out[BLK_SZ], int nrounds)
{
    U32 *x = (U32*)xt;
    memcpy((void*)x, (void*)st, BLK_SZ);

    for (int i = nrounds; i > 0; i -= 2) { /// 20 rounds, 2 rounds per loop
        _quarter(x, 0, 4,  8, 12);         /// column 0
        _quarter(x, 1, 5,  9, 13);         /// column 1
        _quarter(x, 2, 6, 10, 14);         /// column 2
        _quarter(x, 3, 7, 11, 15);         /// column 3
        _quarter(x, 0, 5, 10, 15);         /// diag 1
        _quarter(x, 1, 6, 11, 12);         /// diag 2
        _quarter(x, 2, 7,  8, 13);         /// diag 3
        _quarter(x, 3, 4,  9, 14);         /// diag 4
    }
    ///> serialize output
    for (int i = 0; i < ST_SZ; i++) {
        *(U32*)&out[i * 4] = x[i] + st[i];
    }
}

int ChaCha20::_self_diag()
{
    U32 s0[ST_SZ] = {
        0x61707865, 0x3320646e, 0x79622d32, 0x6b206574,
        0x03020100, 0x07060504, 0x0b0a0908, 0x0f0e0d0c,
        0x13121110, 0x17161514, 0x1b1a1918, 0x1f1e1d1c,
        0x00000001, 0x09000000, 0x4a000000, 0x00000000
    };
    U8 gold[BLK_SZ] = {
        0x10, 0xf1, 0xe7, 0xe4, 0xd1, 0x3b, 0x59, 0x15, 0x50, 0x0f, 0xdd, 0x1f, 0xa3, 0x20, 0x71, 0xc4,
        0xc7, 0xd1, 0xf4, 0xc7, 0x33, 0xc0, 0x68, 0x03, 0x04, 0x22, 0xaa, 0x9a, 0xc3, 0xd4, 0x6c, 0x4e,
        0xd2, 0x82, 0x64, 0x46, 0x07, 0x9f, 0xaa, 0x09, 0x14, 0xc2, 0xd7, 0x05, 0xd9, 0x8b, 0x02, 0xa2,
        0xb5, 0x12, 0x9c, 0xd1, 0xde, 0x16, 0x4e, 0xb9, 0xcb, 0xd0, 0x83, 0xe8, 0xa2, 0x50, 0x3c, 0x4e
    };
    static bool passed = false;
    if (passed) return 0;

    memcpy((void*)st, (void*)s0, BLK_SZ);
    _one_block(xt, NROUND);

    printf("ChaCha20_self_check: ==========\n");
    dump("gold",  gold, BLK_SZ);
    dump("block", xt,  BLK_SZ);
    
    int sum = 0;
    for (int i=0; i < BLK_SZ; i++) {
        sum += (gold[i]!=xt[i]);
    }
    passed = (sum == 0);
    printf("===========>: %s\n\n", sum ? "failed" : "passed");
    
    return sum;
}

ChaCha20::ChaCha20(U8 key[32], U32 counter, U8 nonce[12])
{
    if (_self_diag()) return;
    
    st[0] = 0x61707865;                    ///> "expand 32-byte k"
    st[1] = 0x3320646e;                    ///> 128-bit constant 
    st[2] = 0x79622d32;
    st[3] = 0x6b206574;
    for (int i = 0; i < 8; i++) {          ///> 256-bit key
        st[4 + i] = *(U32*)&key[i * 4];
    }
    st[12] = counter;                      ///> 32-bit counter
    for (int i = 0; i < 3; i++) {          ///> 96-bit nonce
        st[13 + i] = *(U32*)&nonce[i * 4];
    }
}

void ChaCha20::xcrypt(U8 *in, U8 *out, int inlen)
{
    for (int i = 0; i < inlen; i += BLK_SZ) {
        _one_block(xt, NROUND);
        st[12]++;                          /// * increase counter
        dump("xt", xt, BLK_SZ);

        for (int j = i; j < i + BLK_SZ; j++) {
            if (j >= inlen) break;
            out[j] = in[j] ^ xt[j - i];   /// * cipher input stream
        }
    }
}

int main(int argc, char **argv) {
    U8 key[] = {
        0x00, 0x01, 0x02, 0x03,
        0x04, 0x05, 0x06, 0x07,
        0x08, 0x09, 0x0a, 0x0b,
        0x0c, 0x0d, 0x0e, 0x0f,
        0x10, 0x11, 0x12, 0x13,
        0x14, 0x15, 0x16, 0x17,
        0x18, 0x19, 0x1a, 0x1b,
        0x1c, 0x1d, 0x1e, 0x1f
    };
    U8 nonce[] = {
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x4a, 0x00, 0x00, 0x00, 0x00
    };
    U8 plain[] = {
        0x4c, 0x61, 0x64, 0x69, 0x65, 0x73, 0x20, 0x61, 0x6e, 0x64, 0x20, 0x47, 0x65, 0x6e, 0x74, 0x6c,
        0x65, 0x6d, 0x65, 0x6e, 0x20, 0x6f, 0x66, 0x20, 0x74, 0x68, 0x65, 0x20, 0x63, 0x6c, 0x61, 0x73,
        0x73, 0x20, 0x6f, 0x66, 0x20, 0x27, 0x39, 0x39, 0x3a, 0x20, 0x49, 0x66, 0x20, 0x49, 0x20, 0x63,
        0x6f, 0x75, 0x6c, 0x64, 0x20, 0x6f, 0x66, 0x66, 0x65, 0x72, 0x20, 0x79, 0x6f, 0x75, 0x20, 0x6f,
        0x6e, 0x6c, 0x79, 0x20, 0x6f, 0x6e, 0x65, 0x20, 0x74, 0x69, 0x70, 0x20, 0x66, 0x6f, 0x72, 0x20,
        0x74, 0x68, 0x65, 0x20, 0x66, 0x75, 0x74, 0x75, 0x72, 0x65, 0x2c, 0x20, 0x73, 0x75, 0x6e, 0x73,
        0x63, 0x72, 0x65, 0x65, 0x6e, 0x20, 0x77, 0x6f, 0x75, 0x6c, 0x64, 0x20, 0x62, 0x65, 0x20, 0x69,
        0x74, 0x2e
    };
    const char plain1[128] = "this is a test";
    
    constexpr size_t LEN = sizeof(plain);
    U8 *input = (U8*)plain;
    U8 cipher[LEN];
    U8 output[LEN];
    int counter = 1;

    ChaCha20 ctx0(key, counter, nonce);
    ChaCha20 ctx1(key, counter, nonce);

    dump("key",    key,    32);
    dump("nonce",  nonce,  12);
    
    ctx0.xcrypt(input,  cipher, LEN);
    ctx1.xcrypt(cipher, output, LEN);

    dump("input",  input,  LEN);
    dump("cipher", cipher, LEN);
    dump("output", output, LEN);
    
    return 0;
}
