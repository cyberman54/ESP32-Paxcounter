/*******************************************************************************
 * 
 * ttn-esp32 - The Things Network device library for ESP-IDF / SX127x
 * 
 * Copyright (c) 2018 Manuel Bleichenbacher
 * 
 * Licensed under MIT License
 * https://opensource.org/licenses/MIT
 *
 * AES encryption using ESP32's hardware AES unit.
 *******************************************************************************/

#ifdef USE_MBEDTLS_AES

#include "mbedtls/aes.h"
#include "../../lmic/oslmic.h"

void lmic_aes_encrypt(u1_t *data, u1_t *key)
{
    mbedtls_aes_context ctx;
    mbedtls_aes_init(&ctx);
    mbedtls_aes_setkey_enc(&ctx, key, 128);
    mbedtls_aes_crypt_ecb(&ctx, MBEDTLS_AES_ENCRYPT, data, data);
    mbedtls_aes_free(&ctx);
}


#endif