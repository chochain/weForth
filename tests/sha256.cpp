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
    U8  _data[2 * BLOCK_SZ];
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
#define S0(x)        (ROR(x,  7) ^ ROR(x, 18) ^ (x >> 3))
#define S1(x)        (ROR(x, 17) ^ ROR(x, 19) ^ (x >> 10))
#define E0(x)        (ROR(x,  2) ^ ROR(x, 13) ^ ROR(x, 22))
#define E4(x)        (ROR(x,  6) ^ ROR(x, 11) ^ ROR(x, 25))
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
    U32 w[64], h[8];
    for (int i = 0; i < (int)nblock; i++) {
        const U8 *p = msg + (i << 6);
        for (int j = 0; j < 16; j++) {
            PACK32(&p[j << 2], &w[j]);
        }
        for (int j = 16; j < 64; j++) {
            w[j] =  S1(w[j -  2]) + w[j -  7] + S0(w[j - 15]) + w[j - 16];
        }
        for (int j = 0; j < 8; j++) {
            h[j] = _h[j];   // a,b,..,h
        }
        for (int j = 0; j < 64; j++) {
            U32 t1 = h[7] + E4(h[4]) + CH(h[4], h[5], h[6]) + K[j] + w[j];
            U32 t2 = E0(h[0]) + MAJ(h[0], h[1], h[2]);

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
    U32 delta   = BLOCK_SZ - _len;
    U32 rem_len = len < delta ? len : delta;
    memcpy(&_data[_len], msg0, rem_len);
    
    if (len < delta) {
        _len += len;
        return;
    }
    _transform(_data, 1);
    
    const U8 *msg1 = msg0 + rem_len;
    U32 new_len    = len - rem_len;
    U32 nblock     = new_len / BLOCK_SZ;
    printf("nblock=%d\n", nblock);
    _transform(msg1, nblock);
    
    rem_len = new_len % BLOCK_SZ;
    memcpy(_data, &msg1[nblock << 6], rem_len);
    
    _len = rem_len;
    _tot += (nblock + 1) << 6;
}
 
void SHA256::final(U8 *digest)
{
    U32 nblock = (1 + ((BLOCK_SZ - 9) < (_len % BLOCK_SZ)));
    U32 len_b  = (_tot + _len) << 3;
    U32 pm_len = nblock << 6;
    memset(_data + _len, 0, pm_len - _len);
    
    _data[_len] = 0x80;
    UNPACK32(len_b, _data + pm_len - 4);
    _transform(_data, nblock);
    
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
      "248d6a61d20638b8e5c026930c3e6039a33ce45964ff2167f6ecedd419db06c1",
      "cf5b16a778af8380036ce59e7b0492370b249b11e8f07a51afac45037afee9d1",
      "ab64eff7e88e2e46165e29f2bce41826bd4c7b3552f6b382a9e7d3af47c245f8",
      "f08a78cbbaee082b052ae0708f32fa1e50c5c421aa772ba5dbb406a2ea6be342",
      "0ab803344830f92089494fb635ad00d76164ad6e57012b237722df0d7ad26896",
      "e4326d0459653d7d3514674d713e74dc3df11ed4d30b4013fd327fdb9e394c26",
      "d7a8fbb307d7809469ca9abcb0082e4f8d5651e46d3cdb762d02d0bf37c9e592",
      "2ce675bd3b70e104d696d1b25bf3d42b2b45cd776d4f590f210f12c44bf473d5"
    };
    const string input[] = {
        "abc",
        "",
        "abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq",
        "abcdefghbcdefghicdefghijdefghijkefghijklfghijklmghijklmnhijklmnoijklmnopjklmnopqklmnopqrlmnopqrsmnopqrstnopqrstu",
        "This is exactly 64 bytes long, not counting the terminating byte",
        "For this sample, this 63-byte string will be used as input data",
        "And this textual data, astonishing as it may appear, is exactly 128 bytes in length, as are both SHA-384 and SHA-512 block sizes",
        "By hashing data that is one byte less than a multiple of a hash block length (like this 127-byte string), bugs may be revealed.",
        "The quick brown fox jumps over the lazy dog",
        "This test tries to use the n-block utility from the hash library and as a matter of fact we're trying to get only 128 characters"
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
