#include <cstring>
#include <fstream>

typedef unsigned char      U8;
typedef unsigned int       U32;
typedef unsigned long long U64;

const U32 BLOCK_SZ  = 64;      /* 512 bits in bytes */
const U32 DIGEST_SZ = 32;      /* 256 bits in bytes */

class SHA256 {
    const static U32 K[];
    
    U32 _tot;
    U32 _len;
    U8  _block[2 * BLOCK_SZ];
    U32 _h[8];
    
    void _transform(const U8 *msg, U32 nblock);
    
public:
    void init();
    void update(const U8 *msg, U32 len);
    void final(U8 *digest);
};
 
#define ROR(x, n)    ((x >> n) | (x << ((sizeof(x) << 3) - n)))
#define ROL(x, n)    ((x << n) | (x >> ((sizeof(x) << 3) - n)))
#define CH(x, y, z)  ((x & y) ^ (~x & z))
#define MAJ(x, y, z) ((x & y) ^ (x & z) ^ (y & z))
#define E0(x)        (ROR(x,  2) ^ ROR(x, 13) ^ ROR(x, 22))
#define E1(x)        (ROR(x,  6) ^ ROR(x, 11) ^ ROR(x, 25))
#define S0(x)        (ROR(x,  7) ^ ROR(x, 18) ^ (x >> 3))
#define S1(x)        (ROR(x, 17) ^ ROR(x, 19) ^ (x >> 10))
#define UNPACK32(x, str)                  \
{                                         \
    *((str) + 3) = (U8)((x)      );       \
    *((str) + 2) = (U8)((x) >>  8);       \
    *((str) + 1) = (U8)((x) >> 16);       \
    *((str) + 0) = (U8)((x) >> 24);       \
}
#define PACK32(str, x)                    \
{                                         \
    *(x) =   ((U32) *((str) + 3)      )   \
           | ((U32) *((str) + 2) <<  8)   \
           | ((U32) *((str) + 1) << 16)   \
           | ((U32) *((str) + 0) << 24);  \
}

const U32 SHA256::K[64] = { //UL = uint32
    0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5,
    0x3956c25b, 0x59f111f1, 0x923f82a4, 0xab1c5ed5,
    0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3,
    0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174,
    0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc,
    0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
    0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7,
    0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967,
    0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13,
    0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85,
    0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3,
    0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
    0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5,
    0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3,
    0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208,
    0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2
};

void SHA256::_transform(const U8 *msg, U32 nblock)
{
    U32 m[64], h[8], t1, t2;
    const U8 *sub_block;
    for (int i = 0; i < (int)nblock; i++) {
        sub_block = msg + (i << 6);
        for (int j = 0; j < 16; j++) {
            PACK32(&sub_block[j << 2], &m[j]);
        }
        for (int j = 16; j < 64; j++) {
            m[j] =  S1(m[j -  2]) + m[j -  7] + S0(m[j - 15]) + m[j - 16];
        }
        for (int j = 0; j < 8; j++) {
            h[j] = _h[j];
        }
        for (int j = 0; j < 64; j++) {
            t1 = h[7] + E1(h[4]) + CH(h[4], h[5], h[6])
                + K[j] + m[j];
            t2 = E0(h[0]) + MAJ(h[0], h[1], h[2]);

            h[7] = h[6];
            h[6] = h[5];
            h[5] = h[4];
            h[4] = h[3] + t1;
            h[3] = h[2];
            h[2] = h[1];
            h[1] = h[0];
            h[0] = t1 + t2;
        }
        for (int j = 0; j < 8; j++) {
            _h[j] += h[j];
        }
    }
}
 
void SHA256::init()
{
    _h[0] = 0x6a09e667;
    _h[1] = 0xbb67ae85;
    _h[2] = 0x3c6ef372;
    _h[3] = 0xa54ff53a;
    _h[4] = 0x510e527f;
    _h[5] = 0x9b05688c;
    _h[6] = 0x1f83d9ab;
    _h[7] = 0x5be0cd19;
    _len  = 0;
    _tot  = 0;
}
 
void SHA256::update(const U8 *msg0, U32 len)
{
    U32 tmp_len = BLOCK_SZ - _len;
    U32 rem_len = len < tmp_len ? len : tmp_len;
    memcpy(&_block[_len], msg0, rem_len);
    
    if (_len + len < BLOCK_SZ) {
        _len += len;
        return;
    }
    U32 new_len    = len - rem_len;
    U32 nblock     = new_len / BLOCK_SZ;
    const U8 *msg1 = msg0 + rem_len;
    _transform(_block, 1);
    _transform(msg1, nblock);
    
    rem_len = new_len % BLOCK_SZ;
    memcpy(_block, &msg1[nblock << 6], rem_len);
    
    _len = rem_len;
    _tot += (nblock + 1) << 6;
}
 
void SHA256::final(U8 *digest)
{
    U32 nblock = (1 + ((BLOCK_SZ - 9) < (_len % BLOCK_SZ)));
    U32 len_b  = (_tot + _len) << 3;
    U32 pm_len = nblock << 6;
    memset(_block + _len, 0, pm_len - _len);
    
    _block[_len] = 0x80;
    UNPACK32(len_b, _block + pm_len - 4);
    _transform(_block, nblock);
    
    for (int i = 0 ; i < 8; i++) {
        UNPACK32(_h[i], &digest[i << 2]);
    }
}
 
#include <iostream>
using namespace std;
 
string sha256(SHA256 &ctx, string input)
{
    U8 digest[DIGEST_SZ];
    ctx.init();
    ctx.update((U8*)input.c_str(), input.length());
    ctx.final(digest);
 
    char buf[2*DIGEST_SZ+1];
    buf[2*DIGEST_SZ] = 0;
    for (int i = 0; i < DIGEST_SZ; i++)
        sprintf(buf+i*2, "%02x", digest[i]);
    return string(buf);
}

int main(int argc, char *argv[])
{
    const string gold[] = {
        "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad",
        "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855",
        "248d6a61d20638b8e5c026930c3e6039a33ce45964ff2167f6ecedd419db06c1"
    };
    const string input[] = {
        "abc",
        "",
        "abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq"
    };
    SHA256 ctx = SHA256();
    for (int i=0; i< sizeof(input)/sizeof(string); i++) {
        string output = sha256(ctx, input[i]);
        cout << ((output==gold[i]) ? "OK " : "ERR")
             << ": '"<< input[i] << "'\n   > "
             << output << endl;
    }
    return 0;
}
