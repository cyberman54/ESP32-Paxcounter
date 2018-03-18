/*******************************************************************************
 * Copyright (c) 2016 Matthijs Kooijman
 *
 * LICENSE
 *
 * Permission is hereby granted, free of charge, to anyone
 * obtaining a copy of this document and accompanying files,
 * to do whatever they want with them without any restriction,
 * including, but not limited to, copying, modification and
 * redistribution.
 *
 * NO WARRANTY OF ANY KIND IS PROVIDED.
 *******************************************************************************/

/*
 * The original LMIC AES implementation integrates raw AES encryption
 * with CMAC and AES-CTR in a single piece of code. Most other AES
 * implementations (only) offer raw single block AES encryption, so this
 * file contains an implementation of CMAC and AES-CTR, and offers the
 * same API through the os_aes() function as the original AES
 * implementation. This file assumes that there is an encryption
 * function available with this signature:
 *
 *      extern "C" void lmic_aes_encrypt(u1_t *data, u1_t *key);
 *
 *  That takes a single 16-byte buffer and encrypts it wit the given
 *  16-byte key.
 */

#include "../lmic/oslmic.h"

#if !defined(USE_ORIGINAL_AES)

// This should be defined elsewhere
void lmic_aes_encrypt(u1_t *data, u1_t *key);

// global area for passing parameters (aux, key)
u4_t AESAUX[16/sizeof(u4_t)];
u4_t AESKEY[16/sizeof(u4_t)];

// Shift the given buffer left one bit
static void shift_left(xref2u1_t buf, u1_t len) {
    while (len--) {
        u1_t next = len ? buf[1] : 0;

        u1_t val = (*buf << 1);
        if (next & 0x80)
            val |= 1;
        *buf++ = val;
    }
}

// Apply RFC4493 CMAC, using AESKEY as the key. If prepend_aux is true,
// AESAUX is prepended to the message. AESAUX is used as working memory
// in any case. The CMAC result is returned in AESAUX as well.
static void os_aes_cmac(xref2u1_t buf, u2_t len, u1_t prepend_aux) {
    if (prepend_aux)
        lmic_aes_encrypt(AESaux, AESkey);
    else
        memset (AESaux, 0, 16);

    while (len > 0) {
        u1_t need_padding = 0;
        for (u1_t i = 0; i < 16; ++i, ++buf, --len) {
            if (len == 0) {
                // The message is padded with 0x80 and then zeroes.
                // Since zeroes are no-op for xor, we can just skip them
                // and leave AESAUX unchanged for them.
                AESaux[i] ^= 0x80;
                need_padding = 1;
                break;
            }
            AESaux[i] ^= *buf;
        }

        if (len == 0) {
            // Final block, xor with K1 or K2. K1 and K2 are calculated
            // by encrypting the all-zeroes block and then applying some
            // shifts and xor on that.
            u1_t final_key[16];
            memset(final_key, 0, sizeof(final_key));
            lmic_aes_encrypt(final_key, AESkey);

            // Calculate K1
            u1_t msb = final_key[0] & 0x80;
            shift_left(final_key, sizeof(final_key));
            if (msb)
                final_key[sizeof(final_key)-1] ^= 0x87;

            // If the final block was not complete, calculate K2 from K1
            if (need_padding) {
                msb = final_key[0] & 0x80;
                shift_left(final_key, sizeof(final_key));
                if (msb)
                    final_key[sizeof(final_key)-1] ^= 0x87;
            }

            // Xor with K1 or K2
            for (u1_t i = 0; i < sizeof(final_key); ++i)
                AESaux[i] ^= final_key[i];
        }

        lmic_aes_encrypt(AESaux, AESkey);
    }
}

// Run AES-CTR using the key in AESKEY and using AESAUX as the
// counter block. The last byte of the counter block will be incremented
// for every block. The given buffer will be encrypted in place.
static void os_aes_ctr (xref2u1_t buf, u2_t len) {
    u1_t ctr[16];
    while (len) {
        // Encrypt the counter block with the selected key
        memcpy(ctr, AESaux, sizeof(ctr));
        lmic_aes_encrypt(ctr, AESkey);

        // Xor the payload with the resulting ciphertext
        for (u1_t i = 0; i < 16 && len > 0; i++, len--, buf++)
            *buf ^= ctr[i];

        // Increment the block index byte
        AESaux[15]++;
    }
}

u4_t os_aes (u1_t mode, xref2u1_t buf, u2_t len) {
    switch (mode & ~AES_MICNOAUX) {
        case AES_MIC:
            os_aes_cmac(buf, len, /* prepend_aux */ !(mode & AES_MICNOAUX));
            return os_rmsbf4(AESaux);

        case AES_ENC:
            // TODO: Check / handle when len is not a multiple of 16
            for (u1_t i = 0; i < len; i += 16)
                lmic_aes_encrypt(buf+i, AESkey);
            break;

        case AES_CTR:
            os_aes_ctr(buf, len);
            break;
    }
    return 0;
}

#endif // !defined(USE_ORIGINAL_AES)
