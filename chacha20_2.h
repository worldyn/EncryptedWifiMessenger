typedef unsigned char byte;
typedef uint32_t uint32;

void quarterround(uint32 a, uint32 b, uint32 c, uint32 d);
void columnround(uint32 mat[]);
void diagonalround(uint32 mat[]);
void doubleround(uint32 mat[]);
void chacha20_hash(const uint32 in[16], uint32 out[16]);
// void chacha20_enc();

