#include <string>
#include <array>
#include <cstdint>
#include <cstring>
#include <sstream>
#include <iomanip>
using namespace std;

typedef uint8_t  U8;
typedef uint32_t U32;
typedef uint64_t U64;

class SHA256 {
	U8  _data[64];
	U32 _h[8];        // a, b, c, d, e, f, g, h

	static constexpr array<U32, 64> K = {
		0x428a2f98,0x71374491,0xb5c0fbcf,0xe9b5dba5,
		0x3956c25b,0x59f111f1,0x923f82a4,0xab1c5ed5,
		0xd807aa98,0x12835b01,0x243185be,0x550c7dc3,
		0x72be5d74,0x80deb1fe,0x9bdc06a7,0xc19bf174,
		0xe49b69c1,0xefbe4786,0x0fc19dc6,0x240ca1cc,
		0x2de92c6f,0x4a7484aa,0x5cb0a9dc,0x76f988da,
		0x983e5152,0xa831c66d,0xb00327c8,0xbf597fc7,
		0xc6e00bf3,0xd5a79147,0x06ca6351,0x14292967,
		0x27b70a85,0x2e1b2138,0x4d2c6dfc,0x53380d13,
		0x650a7354,0x766a0abb,0x81c2c92e,0x92722c85,
		0xa2bfe8a1,0xa81a664b,0xc24b8b70,0xc76c51a3,
		0xd192e819,0xd6990624,0xf40e3585,0x106aa070,
		0x19a4c116,0x1e376c08,0x2748774c,0x34b0bcb5,
		0x391c0cb3,0x4ed8aa4a,0x5b9cca4f,0x682e6ff3,
		0x748f82ee,0x78a5636f,0x84c87814,0x8cc70208,
		0x90befffa,0xa4506ceb,0xbef9a3f7,0xc67178f2
	};
	void _transform();
	void _final(U32 nblk, U64 nbit);
    
public:
	static string toString(const array<U8, 32> &digest);
    
	void init();
	void update(const U8 *msg, size_t len);
	void update(const string &msg);
	array<U8, 32> digest();
};

constexpr array<U32, 64> SHA256::K;

void SHA256::init() {
	_h[0] = 0x6a09e667;
	_h[1] = 0xbb67ae85;
	_h[2] = 0x3c6ef372;
	_h[3] = 0xa54ff53a;
	_h[4] = 0x510e527f;
	_h[5] = 0x9b05688c;
	_h[6] = 0x1f83d9ab;
	_h[7] = 0x5be0cd19;
}

void SHA256::update(const U8 *msg, size_t len) {
    U32 nblk = 0;
    U64 nbit = 0;
	for (size_t i = 0; i < len; i++) {
		_data[nblk++] = msg[i];   // fill current block
		if (nblk == 64) {
            _transform();         // hash current block
            nbit += 512;          // next block
			nblk = 0;
		}
	}
    nbit += nblk * 8;
	_final(nblk, nbit);           // pad the last block if needed
}

void SHA256::update(const string &msg) {
	update(reinterpret_cast<const U8*>(msg.c_str()), msg.size());
}

array<U8,32> SHA256::digest() {
	array<U8,32> hash;
	// SHA uses Big Endian byte ordering
	// revert each 32-bit word to Little Endian
    for (U8 i = 0, *p=&hash[0]; i < 8; i++) {
        U32 h = _h[i];
        *p++ = (h >> 24) & 0xff;
        *p++ = (h >> 16) & 0xff;
        *p++ = (h >> 8)  & 0xff;
        *p++ = (h >> 0)  & 0xff;
    }
	return hash;
}

#define ROR(x,n)   ((x >> n) | (x << (32 - n)))
#define CH(e,f,g)  ((e & f) ^ (~e & g))
#define MAJ(a,b,c) ((a & (b | c)) | (b & c))
#define SIG0(x)    (ROR(x, 7)  ^ ROR(x, 18) ^ (x >> 3))
#define SIG1(x)    (ROR(x, 17) ^ ROR(x, 19) ^ (x >> 10))
#define E0(x)      (ROR(x, 2)  ^ ROR(x, 13) ^ ROR(x, 22))
#define E4(x)      (ROR(x, 6)  ^ ROR(x, 11) ^ ROR(x, 25))
#define PACK(x)    ((*x<<24) | (*(x+1)<<16) | (*(x+2)<<8) | *(x+3))

void SHA256::_transform() {
    U32 w[64], h[8];
    // split data in 32 bit blocks for the 16 first words
	for (int i=0; i < 64; i++) {
        w[i] = (i < 16)
            ? PACK(&_data[i * 4])       // first 16 blocks (to Big Endian)
            : SIG1(w[i - 2]) + w[i - 7] + SIG0(w[i - 15]) + w[i - 16];
	}
    // 32*8=256-bit block morphing
	for (int i = 0; i < 8; i++) {
		h[i] = _h[i];
	}
	for (int i = 0; i < 64; i++) {
		U32 t1 = h[7] + E4(h[4]) + CH(h[4], h[5], h[6]) + K[i] + w[i];
        U32 t2 = E0(h[0]) + MAJ(h[0], h[1], h[2]);
        
        for (int j = 7; j > 0; --j) h[j] = h[j-1];
        h[0] =  t1 + t2;
        h[4] += t1;
	}
	for (int i = 0; i < 8; i++) {
		_h[i] += h[i];
	}
}

void SHA256::_final(U32 nblk, U64 nbit) {
	int i   = nblk;
	int end = nblk < 56 ? 56 : 64;

	_data[i++] = 0x80;        // append a bit 1
	while (i < end) {
		_data[i++] = 0x00;    // pad with zeros
	}
	if (end == 64) {          // this block is full
		_transform();         // process it and
		memset(_data, 0, 56); // add another block
        printf("extra block\n");
	}
	// pad the total message's length in bits
    for (int i=0, s=56; i < 8; i++, s-=8) {
        _data[56 + i] = nbit >> s;
    }
	_transform();             // process the last block
}

#include <iostream>
#include <chrono>
#include <ctime>

string SHA256::toString(const array<U8, 32> &digest) {
	stringstream s;
    
	s << setfill('0') << hex;
	for(int i = 0 ; i < 32 ; i++) {
		s << setw(2) << (unsigned int) digest[i];
	}
	return s.str();
}

int main(int argc, char ** argv) {
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
    SHA256 ctx;
    for (int i=0; i< sizeof(input)/sizeof(string); i++) {
        ctx.init();
		ctx.update(input[i]);
        
		array<U8, 32> digest = ctx.digest();
        string output = SHA256::toString(digest);
        
        cout << ((output==gold[i]) ? "OK " : "ERR")
             << ": '"<< input[i] << "'\n   > "
             << output << endl;
	}
	return EXIT_SUCCESS;
}
