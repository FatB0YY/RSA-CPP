[English](./README.md) | [Русский](./README.ru.md)

# RSA in C++

This program generates an RSA key pair, encrypts a line of text one byte at a time, and prints the decrypted line. The key calculations use `boost::multiprecision::cpp_int`. The sieve, primality test, modular exponentiation, and extended Euclidean algorithm are implemented in [rsa.cpp](./rsa.cpp).

## Build and run

You need a C++17 compiler with thread support and the Boost headers for Multiprecision and Random. From the directory containing `rsa.cpp`, run

```sh
g++ -std=c++17 -O2 -pthread rsa.cpp -o rsa
./rsa
```

1. Enter the desired **bit length of the modulus**, the product of the two prime factors. Press **Enter** for **2048 bits**. You can enter odd lengths such as `513`. Larger values take longer to generate.
2. After the program prints the factors and both keys, type a message and press **Enter**. Spaces and multibyte text such as UTF-8 are preserved.

The minimum length is 4 bits because the smallest product of two distinct odd primes is 15. With keys shorter than 9 bits, each input byte must be smaller than the generated modulus. From 9 bits up, every byte value fits.

## How the algorithm works

1. **Filter the candidates.** The sieve of Eratosthenes builds a list of primes below 400. Two threads independently generate odd candidates. Each thread checks the small divisors up to the candidate's square root and discards the candidate if it finds a factor.
2. **Test for primality.** For each surviving candidate greater than 3, `isProbablyPrime` runs up to 64 Miller–Rabin rounds. Each round starts by raising a random base to the odd part of `candidate − 1` with `modularPower`, then squares the residue as needed. A failed round rejects the candidate immediately.
3. **Build the keys.** The program retries if the two primes coincide or their product has the wrong bit length. It computes `modulus = firstPrime * secondPrime` and `eulerTotient = (firstPrime - 1) * (secondPrime - 1)`. It chooses a small random public exponent. The extended Euclidean algorithm checks that this exponent is coprime to `eulerTotient` and finds its inverse modulo `eulerTotient`. That inverse is the private exponent.
4. **Encrypt and decrypt.** For each unsigned message byte, `modularPower` raises the byte to the public exponent modulo the modulus. The encrypted blocks go into a queue. The program takes them out in order, applies the private exponent with the same modulus, and reconstructs the original bytes.

## One byte with a 12-bit key

One possible pair of primes is 61 and 53. Their product is 3233 and Euler's totient is 3120. The public exponent 17 and private exponent 2753 work together because $17 \times 2753 \bmod 3120 = 1$.

| Operation                     | Calculation                                     |
| ----------------------------- | ----------------------------------------------- |
| Encrypt the byte for `A` (65) | $65^{17} \bmod 3233 = 2790$                     |
| Decrypt the block 2790        | $2790^{2753} \bmod 3233 = 65$, the byte for `A` |

## Security properties

Each byte is encrypted independently without randomized padding, so equal bytes produce equal blocks under the same key. The console also displays the prime factors and the private key. These properties make this program unsuitable for confidential data. Standard RSA encryption schemes are specified in [RFC 8017, section 7](https://datatracker.ietf.org/doc/html/rfc8017#section-7).
