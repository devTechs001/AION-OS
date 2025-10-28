#ifndef CRYPTO_H
#define CRYPTO_H

#include <stdint.h>
#include <stddef.h>

// Hash Functions
void sha256(const void* data, size_t len, uint8_t* hash);
void sha512(const void* data, size_t len, uint8_t* hash);
void blake2b(const void* data, size_t len, uint8_t* hash, size_t hash_len);

// Symmetric Encryption
typedef struct {
    uint32_t key[60];
    int rounds;
} aes_context_t;

void aes_init(aes_context_t* ctx, const uint8_t* key, int key_bits);
void aes_encrypt_block(aes_context_t* ctx, const uint8_t* in, uint8_t* out);
void aes_decrypt_block(aes_context_t* ctx, const uint8_t* in, uint8_t* out);

// AES-GCM (Authenticated Encryption)
typedef struct {
    aes_context_t aes;
    uint8_t h[16];
    uint8_t y[16];
    uint8_t tag[16];
} aes_gcm_context_t;

void aes_gcm_init(aes_gcm_context_t* ctx, const uint8_t* key, int key_bits);
void aes_gcm_encrypt(aes_gcm_context_t* ctx, const uint8_t* nonce, 
                     const uint8_t* plaintext, size_t len,
                     const uint8_t* aad, size_t aad_len,
                     uint8_t* ciphertext, uint8_t* tag);
int aes_gcm_decrypt(aes_gcm_context_t* ctx, const uint8_t* nonce,
                    const uint8_t* ciphertext, size_t len,
                    const uint8_t* aad, size_t aad_len,
                    const uint8_t* tag, uint8_t* plaintext);

// ChaCha20-Poly1305
void chacha20_encrypt(const uint8_t* key, const uint8_t* nonce, uint32_t counter,
                      const uint8_t* plaintext, size_t len, uint8_t* ciphertext);
void poly1305_mac(const uint8_t* msg, size_t len, const uint8_t* key, uint8_t* tag);

// Public Key Cryptography (Ed25519)
typedef struct {
    uint8_t private_key[32];
    uint8_t public_key[32];
} ed25519_keypair_t;

void ed25519_generate_keypair(ed25519_keypair_t* keypair);
void ed25519_sign(const uint8_t* message, size_t len,
                  const uint8_t* private_key, uint8_t* signature);
int ed25519_verify(const uint8_t* message, size_t len,
                   const uint8_t* public_key, const uint8_t* signature);

// X25519 (Key Exchange)
void x25519_keypair(uint8_t* public_key, uint8_t* private_key);
void x25519_shared_secret(const uint8_t* my_private, const uint8_t* their_public,
                          uint8_t* shared_secret);

// Random Number Generation
void crypto_random_bytes(void* buffer, size_t size);
uint32_t crypto_random_u32(void);

// Secure Memory
void* secure_malloc(size_t size);
void secure_free(void* ptr, size_t size);
void secure_zero(void* ptr, size_t size);

#endif // CRYPTO_H