/* public api for steve reid's public domain SHA-1 implementation */
/* this file is in the public domain */

#pragma once

#include "Platform.h"

struct SHA1_CTX
{
    uint32_t state[5];
    uint32_t count[2];
    uint8_t  buffer[64];
};

#define SHA1_DIGEST_SIZE 20

void SHA1_Init(SHA1_CTX* context);
void SHA1_Update(SHA1_CTX* context, const uint8_t* data, const size_t len);
void SHA1_Final(SHA1_CTX* context, uint8_t digest[SHA1_DIGEST_SIZE]);

void sha1_32a ( const void * key, int len, uint32_t seed, void * out );