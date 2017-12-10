
# Spec for  SCS-protocol
__(Secured Communication Session)__


In this text, let's assume peer A (or simply `A`) is connecting to peer B (or simply `B`). In other words, `B` must have been listening on some port `P` and `A` must've initiated a TCP-connection to `B` on port `P`.

### Initial contact
After establishing a connection over TCP peers `A` and `B` now have to initiate the _SCS_ through a handshake. Since `A` initiated the connection it will be `A` who begins the handshake. `A` does so by simply sending the raw _ASCII data_ `SCS INIT` followed by a null (or zero) byte. So `A` sends 9 bytes to start the handshake. In hexadecimal the 9 bytes sent would look like this: `0x53 0x43 0x53 0x20 0x49 0x4e 0x49 0x54 0x00`

### Key exchange
In response `B` will send the raw _ASCII data_ `SCS SKEY` (_SCS server key_), followed by a null byte and then the 2048-bit _DH public key_ of `B`. So, in total `B` sends 265 bytes (9 bytes for the initial _ASCII data_ and null byte, 256 bytes for the key). Next `B` also needs to send their 256-bit nonce by first sending the _ASCII data_ `SCS SNON`, followed by a null byte and then the 256-bit nonce. In hexadecimal it would look like this when `[KEY]` is replaced with the 256 bytes of the public key of `B` and `[NONCE]` is replaced by the nonce of `B`:
```
0x53 0x43 0x53 0x20 0x53 0x4b 0x45 0x59 0x00 [KEY] 0x53 0x43 0x53 0x20 0x53 0x4e 0x4f 0x4e 0x00 [NONCE]
```

After `A` has received the public key and nonce of `B`, `A` sends its public key and nonce in the same way but with different leading _ASCII data_. A starts by sending `SCS CKEY` (_SCS client key_) instead of `SCS SKEY` (_SCS server key_) and also used `SCS CNON` instead of `SCS SCNON`. In hexadecimal it would look like this when `[KEY]` is replaced with the 256) bytes of the public key of `A`:
```
0x53 0x43 0x53 0x20 0x43 0x4b 0x45 0x59 0x00 [KEY] 0x53 0x43 0x53 0x20 0x43 0x4e 0x4f 0x4e 0x00 [NONCE].
```


### Shared keys
Now that both peers have each other's public key they can both use _Modular Exponatiation_ (see Diffie Hellman key exhange) to derive the same _Shared Secret_. In other words, if _x_ is the peer's private (256 -bit) private key and _y_ is the other peer's public (2048-bit) public key, the peer calculates _x_^_y_ (mod _p_) where _p_ is the prime specified by the diffie hellman parameters (see appendix). The independent calculations on both peer devices will result in the same result, the _shared secret_ (or `SS`). The size of `SS` is the same as the public keys, ie 256 bytes.

From the shared secret `SS`, nonce from `A` (`NA`) and nonce from `B` (`NB`) four shared keys will be generated, all 256 bits (32 bytes) since that is the key size of ChaCha20. Let `spoch(l, msg)` be the spoch digest of length `l` of message `msg`.
```
CKA = spoch(32, append("CiphKeyA", NA, NB, SS)
CKB = spoch(32, append("CiphKeyB", NA, NB, SS)
```

So the length of the data that is hashed is always 8 + 32 + 32 + 256 = 328 bytes.

+ `CKA` Is used as the ChaCha20 key when `A` sends a message. So `A` encrypts with it and `B` decrypts with it.
+ `CKB` Is used as the ChaCha20 key when `B` sends a message. So `B` encrypts with it and `A` decrypts with it.

```c
assert(sizeof(sk_bytes) == 32);
assert(sizeof(ss_bytes) == 256);
unsigned int i;
for(i=0 ; i < 32 ; ++i) {
  sk_bytes[i] =
    ss_bytes[i]     ^ ss_bytes[i+32]  ^ 
    ss_bytes[i+64]  ^ ss_bytes[i+96]  ^
    ss_bytes[i+128] ^ ss_bytes[i+160] ^
    ss_bytes[i+192] ^ ss_bytes[i+224] 
}
```

### Key verification
The peers could easily verify that they have derived the same `SK` by using some _hash function_ and sending each other the hash of the key. Since we want to limit ourselves to as few cryptographic primitives as possible we don't want to do this. For now, let's just not perform any key verification? (__TODO 5__).

### Data transfer
Now that both `A` and `B` have securely established a _shared key_ they can securely transfer data between each other. Data is transferred in _envelopes_. An _envelope_ can contain from 1 byte up to 65535 (256<sup>2</sup>-1) bytes of data. If a peer has more than 65535 bytes of data to send it will have to be split up into multiple _envelopes_.

Let `D` be the sequence of bytes to send in an _envelope_ and let the length of `D` (in bytes) be `L` (so 1 <= `L` <= 65535). First the peer must generate a random nonce (let's call it `N`) for the _envelope_. _ChaCha20_ uses an 8 byte nonce as part of it's input, so the random nonce should be 8 bytes (__TODO 6__). The peer must then use _ChaCha20_ to encrypt `D` with `N` and `SK`. Let's call the resulting _cipher text_ `C`. The length of `C` will also be `L`. The _envelope_ is then constructed by concatenating the following:

+ The _ASCII data_ `SCS SENV` (_SCS_ start envelope). In hexadecimal: `0x53 0x43 0x53 0x20 0x53 0x45 0x4e 0x56`.
+ A null byte. In hexadecimal: `0x00`.
+ The 8 byte nonce (`N`).
+ The length of the data (`L`) represented as a 2 byte unsigned integer. Eg if the size of the data sent is 1000 bytes then the bytes sent here would like this in hexadecimal: `0x03 0xE8`. Or if the size is 65535 (the max size) it would look like this `0xFF 0xFF`.
+ The cipher text (`C`) (`L` bytes).

### Suggestion for envelope nonce genereration
To avoid the same nonce being for more than one envelope the following strategy could be used. It is not mandated by the protocol, just a recommendation/suggestion.

The suggested strategy is for a peer to use their own public key and the _chacha20_ primtitve to generate new nonces for each new envelope.

Here the function _chacha20_ refers to the function that takes the 4 by 4 word matrix (1 word = 32 bits), applies 10 double rounds to it and finally adds the result to the initial input using word-wise addition (mod 2^32). The result is a 64 byte sequence.

First reduce the public key (256 bytes) dowN to 40 bytes (32+8) by grabbing the last 40 bytes of the public key (the 40 least sig bytes of the integer that is the public key). Of these 40 bytes, let {n<sub>7</sub>, ..., n<sub>0</sub>} be the 8 last bytes (so the 8 least significant bytes) and {k<sub>31</sub>, ..., k<sub>0</sub>} be the other 32 bytes.

Create a _chacha20_ matrix and initialize the constant words as usual and set the counter words to 0. Use the byte sequence {k<sub>31</sub>, ..., k<sub>0</sub>} as the key and the byte sequence {n<sub>7</sub>, ..., n<sub>0</sub>} as the nonce. Let's call thiss matrix `M`.

To generate the first envelope nonce, simply run the _chacha20_ function with `M` as input, keeping the counter at 0. The bytes in the output that correspond the the nonce position will be used as the nonce for the envelope. To generate the nonce for the next envelope, increase the counter by one in `M`, pass `M` to _chacha20_ and extract the nonce from the output. 

## TODO
### 0 - Name
Should it be called _SCS protocol_?


### ~~1 - ECDH~~
Is ECDH going to be used for key exchange? What key size? Is it correct that the public key size is twice the private key size?

### ~~2 - EC curve parameters~~
Decide on what curve to use and specify it in the document above.

### ~~3 - ChaCha20~~
Is ChaCha20 (256-bit key) going to be used as the encryption algorithm?

### 4 - Secret to key
Should the 8 32 byte segments of `SS` simply be _xor-ed_ to derive the `SK` or should some other method be used? Maybe use chacha20 primitive to hash?

### 5 - Key verification
Should there be no verification of the _shared key_ before it is used?

### 6 - Envelope nonce
Should the envelope nonce simply be the 8 byte nonce to the _ChaCha20_ algorithm? It could also be 8+keysize (8+32 = 4) bytes to specify both _ChaCha20_ nonce and a keysized pad to _xor_ the _shared key_ with before encrypting. This is probably unnecessary, probably fine as is.

### 7 - Ending envelopes
Should envelopes be explicitly ended? Something like `SCS EENV` followed by the nonce?

### 8 - Version number
Should there be a version number in the initial `SCS INIT` message?

### 9 - Add example handshake and data transfer


## DH parameters

Suggestion for Diffie Hellman parameters. These are taken from rfc5114, section 2.3. The generator and public keys are 2048-bits, the private keys (exponent) 256-bits.

#### The prime number `p` in hexadecimal:
```
87A8E61D B4B6663C FFBBD19C 65195999 8CEEF608 660DD0F2
5D2CEED4 435E3B00 E00DF8F1 D61957D4 FAF7DF45 61B2AA30
16C3D911 34096FAA 3BF4296D 830E9A7C 209E0C64 97517ABD
5A8A9D30 6BCF67ED 91F9E672 5B4758C0 22E0B1EF 4275BF7B
6C5BFC11 D45F9088 B941F54E B1E59BB8 BC39A0BF 12307F5C
4FDB70C5 81B23F76 B63ACAE1 CAA6B790 2D525267 35488A0E
F13C6D9A 51BFA4AB 3AD83477 96524D8E F6A167B5 A41825D9
67E144E5 14056425 1CCACB83 E6B486F6 B3CA3F79 71506026
C0B857F6 89962856 DED4010A BD0BE621 C3A3960A 54E710C3
75F26375 D7014103 A4B54330 C198AF12 6116D227 6E11715F
693877FA D7EF09CA DB094AE9 1E1A1597
```

#### The generator `g` in hexadecimal:
```
3FB32C9B 73134D0B 2E775066 60EDBD48 4CA7B18F 21EF2054
07F4793A 1A0BA125 10DBC150 77BE463F FF4FED4A AC0BB555
BE3A6C1B 0C6B47B1 BC3773BF 7E8C6F62 901228F8 C28CBB18
A55AE313 41000A65 0196F931 C77A57F2 DDF463E5 E9EC144B
777DE62A AAB8A862 8AC376D2 82D6ED38 64E67982 428EBC83
1D14348F 6F2F9193 B5045AF2 767164E1 DFC967C1 FB3F2E55
A4BD1BFF E83B9C80 D052B985 D182EA0A DB2A3B73 13D3FE14
C8484B1E 052588B9 B7D2BBD2 DF016199 ECD06E15 57CD0915
B3353BBB 64E0EC37 7FD02837 0DF92B52 C7891428 CDC67EB6
184B523D 1DB246C3 2F630784 90F00EF8 D647D148 D4795451
5E2327CF EF98C582 664B4C0F 6CC41659
```

Testing using these parameters with a random-ish private key of 256 bits on the ChipkitUno32, both generating the public key and deriving the shared secret takes about 20 seconds. So in total a key exchange should take about 40 seconds.
