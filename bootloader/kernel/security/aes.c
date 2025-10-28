#include "crypto.h"
#include <string.h>

// AES S-Box
static const uint8_t sbox[256] = {
    0x63, 0x7c, 0x77, 0x7b, 0xf2, 0x6b, 0x6f, 0xc5, 0x30, 0x01, 0x67, 0x2b, 0xfe, 0xd7, 0xab, 0x76,
    // ... (full S-box table)
};

// AES Inverse S-Box
static const uint8_t inv_sbox[256] = {
    0x52, 0x09, 0x6a, 0xd5, 0x30, 0x36, 0xa5, 0x38, 0xbf, 0x40, 0xa3, 0x9e, 0x81, 0xf3, 0xd7, 0xfb,
    // ... (full inverse S-box table)
};

// Round constants
static const uint8_t rcon[11] = {
    0x00, 0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80, 0x1b, 0x36
};

// Galois Field multiplication
static uint8_t gf_mul(uint8_t a, uint8_t b) {
    uint8_t p = 0;
    for (int i = 0; i < 8; i++) {
        if (b & 1) p ^= a;
        bool hi_bit = (a & 0x80) != 0;
        a <<= 1;
        if (hi_bit) a ^= 0x1B; // x^8 + x^4 + x^3 + x + 1
        b >>= 1;
    }
    return p;
}

// AES Key Expansion
static void key_expansion(const uint8_t* key, uint32_t* w, int key_bits) {
    int nk = key_bits / 32; // Number of 32-bit words in key
    int nr = nk + 6;        // Number of rounds
    
    // Copy key
    for (int i = 0; i < nk; i++) {
        w[i] = (key[4*i] << 24) | (key[4*i+1] << 16) | 
               (key[4*i+2] << 8) | key[4*i+3];
    }
    
    // Expand key
    for (int i = nk; i < 4 * (nr + 1); i++) {
        uint32_t temp = w[i-1];
        
        if (i % nk == 0) {
            // RotWord
            temp = ((temp << 8) | (temp >> 24));
            // SubWord
            temp = (sbox[(temp >> 24) & 0xFF] << 24) |
                   (sbox[(temp >> 16) & 0xFF] << 16) |
                   (sbox[(temp >> 8) & 0xFF] << 8) |
                   sbox[temp & 0xFF];
            // XOR with Rcon
            temp ^= (rcon[i/nk] << 24);
        } else if (nk > 6 && i % nk == 4) {
            // SubWord
            temp = (sbox[(temp >> 24) & 0xFF] << 24) |
                   (sbox[(temp >> 16) & 0xFF] << 16) |
                   (sbox[(temp >> 8) & 0xFF] << 8) |
                   sbox[temp & 0xFF];
        }
        
        w[i] = w[i-nk] ^ temp;
    }
}

void aes_init(aes_context_t* ctx, const uint8_t* key, int key_bits) {
    ctx->rounds = (key_bits / 32) + 6;
    key_expansion(key, ctx->key, key_bits);
}

// SubBytes transformation
static void sub_bytes(uint8_t* state) {
    for (int i = 0; i < 16; i++) {
        state[i] = sbox[state[i]];
    }
}

// ShiftRows transformation
static void shift_rows(uint8_t* state) {
    uint8_t temp;
    
    // Row 1: shift left by 1
    temp = state[1];
    state[1] = state[5];
    state[5] = state[9];
    state[9] = state[13];
    state[13] = temp;
    
    // Row 2: shift left by 2
    temp = state[2];
    state[2] = state[10];
    state[10] = temp;
    temp = state[6];
    state[6] = state[14];
    state[14] = temp;
    
    // Row 3: shift left by 3
    temp = state[15];
    state[15] = state[11];
    state[11] = state[7];
    state[7] = state[3];
    state[3] = temp;
}

// MixColumns transformation
static void mix_columns(uint8_t* state) {
    for (int i = 0; i < 4; i++) {
        uint8_t* col = state + (i * 4);
        uint8_t a[4];
        memcpy(a, col, 4);
        
        col[0] = gf_mul(a[0], 2) ^ gf_mul(a[1], 3) ^ a[2] ^ a[3];
        col[1] = a[0] ^ gf_mul(a[1], 2) ^ gf_mul(a[2], 3) ^ a[3];
        col[2] = a[0] ^ a[1] ^ gf_mul(a[2], 2) ^ gf_mul(a[3], 3);
        col[3] = gf_mul(a[0], 3) ^ a[1] ^ a[2] ^ gf_mul(a[3], 2);
    }
}

// AddRoundKey transformation
static void add_round_key(uint8_t* state, const uint32_t* key) {
    for (int i = 0; i < 4; i++) {
        state[4*i]   ^= (key[i] >> 24) & 0xFF;
        state[4*i+1] ^= (key[i] >> 16) & 0xFF;
        state[4*i+2] ^= (key[i] >> 8) & 0xFF;
        state[4*i+3] ^= key[i] & 0xFF;
    }
}

void aes_encrypt_block(aes_context_t* ctx, const uint8_t* in, uint8_t* out) {
    uint8_t state[16];
    memcpy(state, in, 16);
    
    // Initial round
    add_round_key(state, &ctx->key[0]);
    
    // Main rounds
    for (int round = 1; round < ctx->rounds; round++) {
        sub_bytes(state);
        shift_rows(state);
        mix_columns(state);
        add_round_key(state, &ctx->key[round * 4]);
    }
    
    // Final round (no MixColumns)
    sub_bytes(state);
    shift_rows(state);
    add_round_key(state, &ctx->key[ctx->rounds * 4]);
    
    memcpy(out, state, 16);
}