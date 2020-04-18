/*
 * Copyright 2016 The Netty Project
 *
 * The Netty Project licenses this file to you under the Apache License,
 * version 2.0 (the "License"); you may not use this file except in compliance
 * with the License. You may obtain a copy of the License at:
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
 * License for the specific language governing permissions and limitations
 * under the License.
 */
/* Licensed to the Apache Software Foundation (ASF) under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.
 * The ASF licenses this file to You under the Apache License, Version 2.0
 * (the "License"); you may not use this file except in compliance with
 * the License.  You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "tcn.h"

#include "ssl_private.h"

/*  _________________________________________________________________
**
**  Additional High-Level Functions for OpenSSL
**  _________________________________________________________________
*/


/*
 * Adapted from OpenSSL:
 * http://osxr.org/openssl/source/ssl/ssl_locl.h#0291
 */
/* Bits for algorithm_mkey (key exchange algorithm) */
#define SSL_kRSA        0x00000001L /* RSA key exchange */
#define SSL_kDHr        0x00000002L /* DH cert, RSA CA cert */ /* no such ciphersuites supported! */
#define SSL_kDHd        0x00000004L /* DH cert, DSA CA cert */ /* no such ciphersuite supported! */
#define SSL_kEDH        0x00000008L /* tmp DH key no DH cert */
#define SSL_kKRB5       0x00000010L /* Kerberos5 key exchange */
#define SSL_kECDHr      0x00000020L /* ECDH cert, RSA CA cert */
#define SSL_kECDHe      0x00000040L /* ECDH cert, ECDSA CA cert */
#define SSL_kEECDH      0x00000080L /* ephemeral ECDH */
#define SSL_kPSK        0x00000100L /* PSK */
#define SSL_kGOST       0x00000200L /* GOST key exchange */
#define SSL_kSRP        0x00000400L /* SRP */

/* Bits for algorithm_auth (server authentication) */
#define SSL_aRSA        0x00000001L /* RSA auth */
#define SSL_aDSS        0x00000002L /* DSS auth */
#define SSL_aNULL       0x00000004L /* no auth (i.e. use ADH or AECDH) */
#define SSL_aDH         0x00000008L /* Fixed DH auth (kDHd or kDHr) */ /* no such ciphersuites supported! */
#define SSL_aECDH       0x00000010L /* Fixed ECDH auth (kECDHe or kECDHr) */
#define SSL_aKRB5       0x00000020L /* KRB5 auth */
#define SSL_aECDSA      0x00000040L /* ECDSA auth*/
#define SSL_aPSK        0x00000080L /* PSK auth */
#define SSL_aGOST94     0x00000100L /* GOST R 34.10-94 signature auth */
#define SSL_aGOST01     0x00000200L /* GOST R 34.10-2001 signature auth */

const char* TCN_UNKNOWN_AUTH_METHOD = "UNKNOWN";

/* OpenSSL end */

/*
 * Adapted from Android:
 * https://android.googlesource.com/platform/external/openssl/+/master/patches/0003-jsse.patch
 */
const char* SSL_cipher_authentication_method(const SSL_CIPHER* cipher){
#ifndef OPENSSL_IS_BORINGSSL
    switch (cipher->algorithm_mkey)
        {
    case SSL_kRSA:
        return SSL_TXT_RSA;
    case SSL_kDHr:
        return SSL_TXT_DH "_" SSL_TXT_RSA;

    case SSL_kDHd:
        return SSL_TXT_DH "_" SSL_TXT_DSS;
    case SSL_kEDH:
        switch (cipher->algorithm_auth)
            {
        case SSL_aDSS:
            return "DHE_" SSL_TXT_DSS;
        case SSL_aRSA:
            return "DHE_" SSL_TXT_RSA;
        case SSL_aNULL:
            return SSL_TXT_DH "_anon";
        default:
            return TCN_UNKNOWN_AUTH_METHOD;
            }
    case SSL_kKRB5:
        return SSL_TXT_KRB5;
    case SSL_kECDHr:
        return SSL_TXT_ECDH "_" SSL_TXT_RSA;
    case SSL_kECDHe:
        return SSL_TXT_ECDH "_" SSL_TXT_ECDSA;
    case SSL_kEECDH:
        switch (cipher->algorithm_auth)
            {
        case SSL_aECDSA:
            return "ECDHE_" SSL_TXT_ECDSA;
        case SSL_aRSA:
            return "ECDHE_" SSL_TXT_RSA;
        case SSL_aNULL:
            return SSL_TXT_ECDH "_anon";
        default:
            return TCN_UNKNOWN_AUTH_METHOD;
            }
    default:
        return TCN_UNKNOWN_AUTH_METHOD;
    }
#else
    return SSL_CIPHER_get_kx_name(cipher);
#endif

}

/* we initialize this index at startup time
 * and never write to it at request time,
 * so this static is thread safe.
 * also note that OpenSSL increments at static variable when
 * SSL_get_ex_new_index() is called, so we _must_ do this at startup.
 */
static int SSL_app_data2_idx = -1;
static int SSL_app_data3_idx = -1;
static int SSL_app_data4_idx = -1;
void SSL_init_app_data_idx()
{
    int i;

    if (SSL_app_data2_idx == -1) {
        /* we _do_ need to call this two times */
        for (i = 0; i <= 1; i++) {
            SSL_app_data2_idx = SSL_get_ex_new_index(0, "tcn_ssl_ctxt_t*", NULL, NULL, NULL);
        }
    }

    if (SSL_app_data3_idx == -1) {
        SSL_app_data3_idx = SSL_get_ex_new_index(0, "int* handshakeCount", NULL, NULL, NULL);
    }

    if (SSL_app_data4_idx == -1) {
        SSL_app_data4_idx = SSL_get_ex_new_index(0, "tcn_ssl_verify_config_t*", NULL, NULL, NULL);
    }
}

void *SSL_get_app_data2(SSL *ssl)
{
    return (void *)SSL_get_ex_data(ssl, SSL_app_data2_idx);
}

void SSL_set_app_data2(SSL *ssl, void *arg)
{
    SSL_set_ex_data(ssl, SSL_app_data2_idx, (char *)arg);
    return;
}

void *SSL_get_app_data3(SSL *ssl)
{
    return SSL_get_ex_data(ssl, SSL_app_data3_idx);
}

void SSL_set_app_data3(SSL *ssl, void *arg)
{
    SSL_set_ex_data(ssl, SSL_app_data3_idx, arg);
}

void *SSL_get_app_data4(SSL *ssl)
{
    return SSL_get_ex_data(ssl, SSL_app_data4_idx);
}

void SSL_set_app_data4(SSL *ssl, void *arg)
{
    SSL_set_ex_data(ssl, SSL_app_data4_idx, arg);
}

int SSL_password_callback(char *buf, int bufsiz, int verify,
                          void *cb)
{
    char *password = (char *) cb;

    if (buf == NULL || password == NULL)
        return 0;
    *buf = '\0';

    if (password[0]) {
        /* Return already obtained password */
        strncpy(buf, password, bufsiz);
    }

    buf[bufsiz - 1] = '\0';
    return (int)strlen(buf);
}

#if (OPENSSL_VERSION_NUMBER < 0x10100000L) || defined(OPENSSL_USE_DEPRECATED) || defined(LIBRESSL_VERSION_NUMBER)

static unsigned char dh0512_p[]={
    0xD9,0xBA,0xBF,0xFD,0x69,0x38,0xC9,0x51,0x2D,0x19,0x37,0x39,
    0xD7,0x7D,0x7E,0x3E,0x25,0x58,0x55,0x94,0x90,0x60,0x93,0x7A,
    0xF2,0xD5,0x61,0x5F,0x06,0xE8,0x08,0xB4,0x57,0xF4,0xCF,0xB4,
    0x41,0xCC,0xC4,0xAC,0xD4,0xF0,0x45,0x88,0xC9,0xD1,0x21,0x4C,
    0xB6,0x72,0x48,0xBD,0x73,0x80,0xE0,0xDD,0x88,0x41,0xA0,0xF1,
    0xEA,0x4B,0x71,0x13
};
static unsigned char dh1024_p[]={
    0xA2,0x95,0x7E,0x7C,0xA9,0xD5,0x55,0x1D,0x7C,0x77,0x11,0xAC,
    0xFD,0x48,0x8C,0x3B,0x94,0x1B,0xC5,0xC0,0x99,0x93,0xB5,0xDC,
    0xDC,0x06,0x76,0x9E,0xED,0x1E,0x3D,0xBB,0x9A,0x29,0xD6,0x8B,
    0x1F,0xF6,0xDA,0xC9,0xDF,0xD5,0x02,0x4F,0x09,0xDE,0xEC,0x2C,
    0x59,0x1E,0x82,0x32,0x80,0x9B,0xED,0x51,0x68,0xD2,0xFB,0x1E,
    0x25,0xDB,0xDF,0x9C,0x11,0x70,0xDF,0xCA,0x19,0x03,0x3D,0x3D,
    0xC1,0xAC,0x28,0x88,0x4F,0x13,0xAF,0x16,0x60,0x6B,0x5B,0x2F,
    0x56,0xC7,0x5B,0x5D,0xDE,0x8F,0x50,0x08,0xEC,0xB1,0xB9,0x29,
    0xAA,0x54,0xF4,0x05,0xC9,0xDF,0x95,0x9D,0x79,0xC6,0xEA,0x3F,
    0xC9,0x70,0x42,0xDA,0x90,0xC7,0xCC,0x12,0xB9,0x87,0x86,0x39,
    0x1E,0x1A,0xCE,0xF7,0x3F,0x15,0xB5,0x2B
};
static unsigned char dh2048_p[]={
    0xF2,0x4A,0xFC,0x7E,0x73,0x48,0x21,0x03,0xD1,0x1D,0xA8,0x16,
    0x87,0xD0,0xD2,0xDC,0x42,0xA8,0xD2,0x73,0xE3,0xA9,0x21,0x31,
    0x70,0x5D,0x69,0xC7,0x8F,0x95,0x0C,0x9F,0xB8,0x0E,0x37,0xAE,
    0xD1,0x6F,0x36,0x1C,0x26,0x63,0x2A,0x36,0xBA,0x0D,0x2A,0xF5,
    0x1A,0x0F,0xE8,0xC0,0xEA,0xD1,0xB5,0x52,0x47,0x1F,0x9A,0x0C,
    0x0F,0xED,0x71,0x51,0xED,0xE6,0x62,0xD5,0xF8,0x81,0x93,0x55,
    0xC1,0x0F,0xB4,0x72,0x64,0xB3,0x73,0xAA,0x90,0x9A,0x81,0xCE,
    0x03,0xFD,0x6D,0xB1,0x27,0x7D,0xE9,0x90,0x5E,0xE2,0x10,0x74,
    0x4F,0x94,0xC3,0x05,0x21,0x73,0xA9,0x12,0x06,0x9B,0x0E,0x20,
    0xD1,0x5F,0xF7,0xC9,0x4C,0x9D,0x4F,0xFA,0xCA,0x4D,0xFD,0xFF,
    0x6A,0x62,0x9F,0xF0,0x0F,0x3B,0xA9,0x1D,0xF2,0x69,0x29,0x00,
    0xBD,0xE9,0xB0,0x9D,0x88,0xC7,0x4A,0xAE,0xB0,0x53,0xAC,0xA2,
    0x27,0x40,0x88,0x58,0x8F,0x26,0xB2,0xC2,0x34,0x7D,0xA2,0xCF,
    0x92,0x60,0x9B,0x35,0xF6,0xF3,0x3B,0xC3,0xAA,0xD8,0x58,0x9C,
    0xCF,0x5D,0x9F,0xDB,0x14,0x93,0xFA,0xA3,0xFA,0x44,0xB1,0xB2,
    0x4B,0x0F,0x08,0x70,0x44,0x71,0x3A,0x73,0x45,0x8E,0x6D,0x9C,
    0x56,0xBC,0x9A,0xB5,0xB1,0x3D,0x8B,0x1F,0x1E,0x2B,0x0E,0x93,
    0xC2,0x9B,0x84,0xE2,0xE8,0xFC,0x29,0x85,0x83,0x8D,0x2E,0x5C,
    0xDD,0x9A,0xBB,0xFD,0xF0,0x87,0xBF,0xAF,0xC4,0xB6,0x1D,0xE7,
    0xF9,0x46,0x50,0x7F,0xC3,0xAC,0xFD,0xC9,0x8C,0x9D,0x66,0x6B,
    0x4C,0x6A,0xC9,0x3F,0x0C,0x0A,0x74,0x94,0x41,0x85,0x26,0x8F,
    0x9F,0xF0,0x7C,0x0B
};
static unsigned char dh4096_p[] = {
    0x8D,0xD3,0x8F,0x77,0x6F,0x6F,0xB0,0x74,0x3F,0x22,0xE9,0xD1,
    0x17,0x15,0x69,0xD8,0x24,0x85,0xCD,0xC4,0xE4,0x0E,0xF6,0x52,
    0x40,0xF7,0x1C,0x34,0xD0,0xA5,0x20,0x77,0xE2,0xFC,0x7D,0xA1,
    0x82,0xF1,0xF3,0x78,0x95,0x05,0x5B,0xB8,0xDB,0xB3,0xE4,0x17,
    0x93,0xD6,0x68,0xA7,0x0A,0x0C,0xC5,0xBB,0x9C,0x5E,0x1E,0x83,
    0x72,0xB3,0x12,0x81,0xA2,0xF5,0xCD,0x44,0x67,0xAA,0xE8,0xAD,
    0x1E,0x8F,0x26,0x25,0xF2,0x8A,0xA0,0xA5,0xF4,0xFB,0x95,0xAE,
    0x06,0x50,0x4B,0xD0,0xE7,0x0C,0x55,0x88,0xAA,0xE6,0xB8,0xF6,
    0xE9,0x2F,0x8D,0xA7,0xAD,0x84,0xBC,0x8D,0x4C,0xFE,0x76,0x60,
    0xCD,0xC8,0xED,0x7C,0xBF,0xF3,0xC1,0xF8,0x6A,0xED,0xEC,0xE9,
    0x13,0x7D,0x4E,0x72,0x20,0x77,0x06,0xA4,0x12,0xF8,0xD2,0x34,
    0x6F,0xDC,0x97,0xAB,0xD3,0xA0,0x45,0x8E,0x7D,0x21,0xA9,0x35,
    0x6E,0xE4,0xC9,0xC4,0x53,0xFF,0xE5,0xD9,0x72,0x61,0xC4,0x8A,
    0x75,0x78,0x36,0x97,0x1A,0xAB,0x92,0x85,0x74,0x61,0x7B,0xE0,
    0x92,0xB8,0xC6,0x12,0xA1,0x72,0xBB,0x5B,0x61,0xAA,0xE6,0x2C,
    0x2D,0x9F,0x45,0x79,0x9E,0xF4,0x41,0x93,0x93,0xEF,0x8B,0xEF,
    0xB7,0xBF,0x6D,0xF0,0x91,0x11,0x4F,0x7C,0x71,0x84,0xB5,0x88,
    0xA3,0x8C,0x1A,0xD5,0xD0,0x81,0x9C,0x50,0xAC,0xA9,0x2B,0xE9,
    0x92,0x2D,0x73,0x7C,0x0A,0xA3,0xFA,0xD3,0x6C,0x91,0x43,0xA6,
    0x80,0x7F,0xD7,0xC4,0xD8,0x6F,0x85,0xF8,0x15,0xFD,0x08,0xA6,
    0xF8,0x7B,0x3A,0xF4,0xD3,0x50,0xB4,0x2F,0x75,0xC8,0x48,0xB8,
    0xA8,0xFD,0xCA,0x8F,0x62,0xF1,0x4C,0x89,0xB7,0x18,0x67,0xB2,
    0x93,0x2C,0xC4,0xD4,0x71,0x29,0xA9,0x26,0x20,0xED,0x65,0x37,
    0x06,0x87,0xFC,0xFB,0x65,0x02,0x1B,0x3C,0x52,0x03,0xA1,0xBB,
    0xCF,0xE7,0x1B,0xA4,0x1A,0xE3,0x94,0x97,0x66,0x06,0xBF,0xA9,
    0xCE,0x1B,0x07,0x10,0xBA,0xF8,0xD4,0xD4,0x05,0xCF,0x53,0x47,
    0x16,0x2C,0xA1,0xFC,0x6B,0xEF,0xF8,0x6C,0x23,0x34,0xEF,0xB7,
    0xD3,0x3F,0xC2,0x42,0x5C,0x53,0x9A,0x00,0x52,0xCF,0xAC,0x42,
    0xD3,0x3B,0x2E,0xB6,0x04,0x32,0xE1,0x09,0xED,0x64,0xCD,0x6A,
    0x63,0x58,0xB8,0x43,0x56,0x5A,0xBE,0xA4,0x9F,0x68,0xD4,0xF7,
    0xC9,0x04,0xDF,0xCD,0xE5,0x93,0xB0,0x2F,0x06,0x19,0x3E,0xB8,
    0xAB,0x7E,0xF8,0xE7,0xE7,0xC8,0x53,0xA2,0x06,0xC3,0xC7,0xF9,
    0x18,0x3B,0x51,0xC3,0x9B,0xFF,0x8F,0x00,0x0E,0x87,0x19,0x68,
    0x2F,0x40,0xC0,0x68,0xFA,0x12,0xAE,0x57,0xB5,0xF0,0x97,0xCA,
    0x78,0x23,0x31,0xAB,0x67,0x7B,0x10,0x6B,0x59,0x32,0x9C,0x64,
    0x20,0x38,0x1F,0xC5,0x07,0x84,0x9E,0xC4,0x49,0xB1,0xDF,0xED,
    0x7A,0x8A,0xC3,0xE0,0xDD,0x30,0x55,0xFF,0x95,0x45,0xA6,0xEE,
    0xCB,0xE4,0x26,0xB9,0x8E,0x89,0x37,0x63,0xD4,0x02,0x3D,0x5B,
    0x4F,0xE5,0x90,0xF6,0x72,0xF8,0x10,0xEE,0x31,0x04,0x54,0x17,
    0xE3,0xD5,0x63,0x84,0x80,0x62,0x54,0x46,0x85,0x6C,0xD2,0xC1,
    0x3E,0x19,0xBD,0xE2,0x80,0x11,0x86,0xC7,0x4B,0x7F,0x67,0x86,
    0x47,0xD2,0x38,0xCD,0x8F,0xFE,0x65,0x3C,0x11,0xCD,0x96,0x99,
    0x4E,0x45,0xEB,0xEC,0x1D,0x94,0x8C,0x53,
};
static unsigned char dhxxx2_g[]={
    0x02
};

static DH *get_dh(int idx)
{
    DH *dh;
    if ((dh = DH_new()) == NULL)
        return NULL;
    switch (idx) {
        case SSL_TMP_KEY_DH_512:
            dh->p = BN_bin2bn(dh0512_p, sizeof(dh0512_p), NULL);
        break;
        case SSL_TMP_KEY_DH_1024:
            dh->p = BN_bin2bn(dh1024_p, sizeof(dh1024_p), NULL);
        break;
        case SSL_TMP_KEY_DH_2048:
            dh->p = BN_bin2bn(dh2048_p, sizeof(dh2048_p), NULL);
        break;
        case SSL_TMP_KEY_DH_4096:
            dh->p = BN_bin2bn(dh4096_p, sizeof(dh2048_p), NULL);
        break;
    }
    dh->g = BN_bin2bn(dhxxx2_g, sizeof(dhxxx2_g), NULL);
    if ((dh->p == NULL) || (dh->g == NULL)) {
        DH_free(dh);
        return NULL;
    }
    else
        return dh;
    return NULL;
}
#else
static DH *get_dh(int idx)
{
    return NULL;
}
#endif

DH *SSL_dh_get_tmp_param(int key_len)
{
    DH *dh;

    if (key_len == 512)
        dh = get_dh(SSL_TMP_KEY_DH_512);
    else if (key_len == 1024)
        dh = get_dh(SSL_TMP_KEY_DH_1024);
    else if (key_len == 2048)
        dh = get_dh(SSL_TMP_KEY_DH_2048);
    else if (key_len == 4096)
        dh = get_dh(SSL_TMP_KEY_DH_4096);
    else
        dh = get_dh(SSL_TMP_KEY_DH_1024);
    return dh;
}

/*
 * Hand out the already generated DH parameters...
 */
DH *SSL_callback_tmp_DH(SSL *ssl, int export, int keylen)
{
    int idx;
    switch (keylen) {
        case 512:
            idx = SSL_TMP_KEY_DH_512;
        break;
        case 2048:
            idx = SSL_TMP_KEY_DH_2048;
        break;
        case 4096:
            idx = SSL_TMP_KEY_DH_4096;
        break;
        case 1024:
        default:
            idx = SSL_TMP_KEY_DH_1024;
        break;
    }
    return (DH *)SSL_temp_keys[idx];
}

DH *SSL_callback_tmp_DH_512(SSL *ssl, int export, int keylen)
{
    return (DH *)SSL_temp_keys[SSL_TMP_KEY_DH_512];
}

DH *SSL_callback_tmp_DH_1024(SSL *ssl, int export, int keylen)
{
    return (DH *)SSL_temp_keys[SSL_TMP_KEY_DH_1024];
}

DH *SSL_callback_tmp_DH_2048(SSL *ssl, int export, int keylen)
{
    return (DH *)SSL_temp_keys[SSL_TMP_KEY_DH_2048];
}

DH *SSL_callback_tmp_DH_4096(SSL *ssl, int export, int keylen)
{
    return (DH *)SSL_temp_keys[SSL_TMP_KEY_DH_4096];
}

/*
 * Read a file that optionally contains the server certificate in PEM
 * format, possibly followed by a sequence of CA certificates that
 * should be sent to the peer in the SSL Certificate message.
 */
int SSL_CTX_use_certificate_chain(SSL_CTX *ctx, const char *file, bool skipfirst)
{
    BIO *bio;
    int n;

    if ((bio = BIO_new(BIO_s_file())) == NULL)
        return -1;
    if (BIO_read_filename(bio, file) <= 0) {
        BIO_free(bio);
        return -1;
    }
    n = SSL_CTX_use_certificate_chain_bio(ctx, bio, skipfirst);
    BIO_free(bio);
    return n;
}

static int SSL_CTX_setup_certs(SSL_CTX *ctx, BIO *bio, bool skipfirst, bool ca)
{
    X509 *x509;
    unsigned long err;
    int n;

    /* optionally skip a leading server certificate */
    if (skipfirst) {
        if ((x509 = PEM_read_bio_X509(bio, NULL, NULL, NULL)) == NULL) {
            return -1;
        }
        X509_free(x509);
    }

    n = 0;
    if (ca) {
        while ((x509 = PEM_read_bio_X509(bio, NULL, NULL, NULL)) != NULL) {
            if (SSL_CTX_add_client_CA(ctx, x509) != 1) {
                X509_free(x509);
                return -1;
            }
            // SSL_CTX_add_client_CA does not take ownership of the x509. It just calls X509_get_subject_name
            // and make a duplicate of this value. So we should always free the x509 after this call.
            // See https://github.com/netty/netty/issues/6249.
            X509_free(x509);
            n++;
        }
    } else {
        /* free a perhaps already configured extra chain */
        SSL_CTX_clear_extra_chain_certs(ctx);

        /* create new extra chain by loading the certs */
        while ((x509 = PEM_read_bio_X509(bio, NULL, NULL, NULL)) != NULL) {
            // SSL_CTX_add_extra_chain_cert transfers ownership of the x509 certificate if the method succeeds.
            if (SSL_CTX_add_extra_chain_cert(ctx, x509) != 1) {
                X509_free(x509);
                return -1;
            }
            n++;
        }
    }

    /* Make sure that only the error is just an EOF */
    if ((err = ERR_peek_error()) > 0) {
        if (!(   ERR_GET_LIB(err) == ERR_LIB_PEM
              && ERR_GET_REASON(err) == PEM_R_NO_START_LINE)) {
            return -1;
        }
        ERR_clear_error();
    }
    return n;
}

int SSL_CTX_use_certificate_chain_bio(SSL_CTX *ctx, BIO *bio, bool skipfirst)
{
    return SSL_CTX_setup_certs(ctx, bio, skipfirst, false);
}


int SSL_CTX_use_client_CA_bio(SSL_CTX *ctx, BIO *bio)
{
    return SSL_CTX_setup_certs(ctx, bio, false, true);
}

int SSL_use_certificate_chain_bio(SSL *ssl, BIO *bio, bool skipfirst)
{
#if OPENSSL_VERSION_NUMBER < 0x10002000L || defined(LIBRESSL_VERSION_NUMBER)
    // Only supported on openssl 1.0.2+
    return -1;
#else
    X509 *x509;
    unsigned long err;
    int n;

    /* optionally skip a leading server certificate */
    if (skipfirst) {
        if ((x509 = PEM_read_bio_X509(bio, NULL, NULL, NULL)) == NULL) {
            return -1;
        }
        X509_free(x509);
    }

    /* create new extra chain by loading the certs */
    n = 0;

    while ((x509 = PEM_read_bio_X509(bio, NULL, NULL, NULL)) != NULL) {
        if (SSL_add0_chain_cert(ssl, x509) != 1) {
            X509_free(x509);
            return -1;
        }
        n++;
    }
    /* Make sure that only the error is just an EOF */
    if ((err = ERR_peek_error()) > 0) {
        if (!(   ERR_GET_LIB(err) == ERR_LIB_PEM
              && ERR_GET_REASON(err) == PEM_R_NO_START_LINE)) {
            return -1;
        }
        ERR_clear_error();
    }
    return n;
#endif
}

X509 *load_pem_cert_bio(const char *password, const BIO *bio)
{
    X509 *cert = PEM_read_bio_X509_AUX((BIO*) bio, NULL,
                (pem_password_cb *)SSL_password_callback,
                (void *)password);
    if (cert == NULL &&
       (ERR_GET_REASON(ERR_peek_last_error()) == PEM_R_NO_START_LINE)) {
        ERR_clear_error();
        BIO_ctrl((BIO*) bio, BIO_CTRL_RESET, 0, NULL);
        cert = d2i_X509_bio((BIO*) bio, NULL);
    }
    return cert;
}

EVP_PKEY *load_pem_key_bio(const char *password, const BIO *bio)
{
    EVP_PKEY *key = PEM_read_bio_PrivateKey((BIO*) bio, NULL,
                    (pem_password_cb *)SSL_password_callback,
                    (void *)password);

    BIO_ctrl((BIO*) bio, BIO_CTRL_RESET, 0, NULL);
    return key;
}

int tcn_EVP_PKEY_up_ref(EVP_PKEY* pkey) {
#if defined(OPENSSL_IS_BORINGSSL)
    // Workaround for https://bugs.chromium.org/p/boringssl/issues/detail?id=89#
    EVP_PKEY_up_ref(pkey);
    return 1;
#elif OPENSSL_VERSION_NUMBER < 0x10100000L || defined(LIBRESSL_VERSION_NUMBER)
    return CRYPTO_add(&pkey->references, 1, CRYPTO_LOCK_EVP_PKEY);
#else
    return EVP_PKEY_up_ref(pkey);
#endif
}

int tcn_X509_up_ref(X509* cert) {
#if defined(OPENSSL_IS_BORINGSSL)
    // Workaround for https://bugs.chromium.org/p/boringssl/issues/detail?id=89#
    X509_up_ref(cert);
    return 1;
#elif OPENSSL_VERSION_NUMBER < 0x10100000L || defined(LIBRESSL_VERSION_NUMBER)
    return CRYPTO_add(&cert->references, 1, CRYPTO_LOCK_X509);
#else
    return X509_up_ref(cert);
#endif
}

int tcn_set_verify_config(tcn_ssl_verify_config_t* c, jint tcn_mode, jint depth) {
    if (depth >= 0) {
        c->verify_depth = depth;
    }

    switch (tcn_mode) {
      case SSL_CVERIFY_IGNORED:
        switch (c->verify_mode) {
          case SSL_CVERIFY_NONE:
            return SSL_VERIFY_NONE;
          case SSL_CVERIFY_OPTIONAL:
            return SSL_VERIFY_PEER;
          default:
            return (SSL_VERIFY_PEER | SSL_VERIFY_FAIL_IF_NO_PEER_CERT);
        }
      case SSL_CVERIFY_NONE:
        c->verify_mode = SSL_CVERIFY_NONE;
        return SSL_VERIFY_NONE;
      case SSL_CVERIFY_OPTIONAL:
        c->verify_mode = SSL_CVERIFY_OPTIONAL;
        return SSL_VERIFY_PEER;
      default:
        c->verify_mode = SSL_CVERIFY_REQUIRED;
        return SSL_VERIFY_PEER | SSL_VERIFY_FAIL_IF_NO_PEER_CERT;
    }
}

int SSL_callback_next_protos(SSL *ssl, const unsigned char **data,
                             unsigned int *len, void *arg)
{
    tcn_ssl_ctxt_t *ssl_ctxt = arg;

    *data = ssl_ctxt->next_proto_data;
    *len = ssl_ctxt->next_proto_len;

    return SSL_TLSEXT_ERR_OK;
}

/* The code here is inspired by nghttp2
 *
 * See https://github.com/tatsuhiro-t/nghttp2/blob/ae0100a9abfcf3149b8d9e62aae216e946b517fb/src/shrpx_ssl.cc#L244 */
int select_next_proto(SSL *ssl, const unsigned char **out, unsigned char *outlen,
        const unsigned char *in, unsigned int inlen, unsigned char *supported_protos,
        unsigned int supported_protos_len, int failure_behavior) {

    unsigned int i = 0;
    unsigned char target_proto_len;
    unsigned char *p;
    const unsigned char *end;
    unsigned char *proto;
    unsigned char proto_len;

    while (i < supported_protos_len) {
        target_proto_len = *supported_protos;
        ++supported_protos;

        p = (unsigned char*) in;
        end = p + inlen;

        while (p < end) {
            proto_len = *p;
            proto = ++p;

            if (proto + proto_len <= end && target_proto_len == proto_len &&
                    memcmp(supported_protos, proto, proto_len) == 0) {

                // We found a match, so set the output and return with OK!
                *out = proto;
                *outlen = proto_len;

                return SSL_TLSEXT_ERR_OK;
            }
            // Move on to the next protocol.
            p += proto_len;
        }

        // increment len and pointers.
        i += target_proto_len;
        supported_protos += target_proto_len;
    }

    if (failure_behavior == SSL_SELECTOR_FAILURE_CHOOSE_MY_LAST_PROTOCOL) {
         // There were no match but we just select our last protocol and hope the other peer support it.
         //
         // decrement the pointer again so the pointer points to the start of the protocol.
         p -= proto_len;
         *out = p;
         *outlen = proto_len;
         return SSL_TLSEXT_ERR_OK;
    }
    // TODO: OpenSSL currently not support to fail with fatal error. Once this changes we can also support it here.
    //       Issue https://github.com/openssl/openssl/issues/188 has been created for this.
    // Nothing matched so not select anything and just accept.
    return SSL_TLSEXT_ERR_NOACK;
}

int SSL_callback_select_next_proto(SSL *ssl, unsigned char **out, unsigned char *outlen,
                         const unsigned char *in, unsigned int inlen,
                         void *arg) {
    tcn_ssl_ctxt_t *ssl_ctxt = arg;
    return select_next_proto(ssl, (const unsigned char**) out, outlen, in, inlen, ssl_ctxt->next_proto_data, ssl_ctxt->next_proto_len, ssl_ctxt->next_selector_failure_behavior);
}

int SSL_callback_alpn_select_proto(SSL* ssl, const unsigned char **out, unsigned char *outlen,
        const unsigned char *in, unsigned int inlen, void *arg) {
    tcn_ssl_ctxt_t *ssl_ctxt = arg;
    return select_next_proto(ssl, out, outlen, in, inlen, ssl_ctxt->alpn_proto_data, ssl_ctxt->alpn_proto_len, ssl_ctxt->alpn_selector_failure_behavior);
}
