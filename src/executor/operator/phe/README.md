### PHE

We apply the threshold Paillier cryptosystem, which consists of three 
algorithms (`Gen, Enc, Dec`). 

 * The key generation algorithm `(sk,pk) = Gen(keysize)` returns a secret key
 `sk` and a public key `pk`, given a security parameter `keysize`. 
 The default `keysize` is 1024 bits.
 
 * The encryption algorithm `c = Enc(x,pk)` maps a plaintext `x` to a 
 ciphertext `c` using `pk`.
 
 * The decryption algorithm `x = Dec(c,sk)` reverses the encryption by `sk`
 and outputs the plaintext x.

When using a threshold variant of Paillier, the key generation algorithm outputs
a partial secret key for each party. Meanwhile, to decrypt a ciphertext, all
parties need to join in the decryption process.

 
The Paillier cryptosystem has two homomorphic properties. For simplicity, we denote
the ciphertext of `x` as `[x]`.

 * Homomorphic addition (HA): given two ciphertexts [x<sub>1</sub>], [x<sub>2</sub>], 
 the ciphertext of x<sub>1</sub>+x<sub>2</sub> can be obtained by multiplying
 the two ciphertexts, i.e., [x<sub>1</sub>]*[x<sub>2</sub>].
 
 * Homomorphic multiplication (HM): given a plaintext x<sub>1</sub> and a ciphertext
 [x<sub>2</sub>], the ciphertext of the product x<sub>1</sub>x<sub>2</sub> can
 be obtained by raising [x<sub>2</sub>] to the power x<sub>1</sub>, i.e., 
 [x<sub>2</sub>]<sup>x<sub>1</sub></sup>.

According to the above discussions, we define the following PHE operators.

 * key generation
 * encryption
 * threshold decryption
 * single value HA
 * single value HM
 * element-wise HM
 * homomorphic inner product between a plaintext vector and a ciphertext vector
 * homomorphic matrix multiplication between a plaintext matrix and a ciphertext matrix
 * **ciphertext multiplication (with the help of MPC operators)**
 * **ciphertext vector multiplication (with the help of MPC operators)**

Since cryptographic schemes only support operations on integers, we convert the
floating point dataset into fixed-point integer representation before using PHE
operators. 
We based on the [libhcs](https://github.com/tiehuis/libhcs) library to construct the above PHE operators.