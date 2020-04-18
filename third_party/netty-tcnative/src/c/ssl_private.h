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

#ifndef SSL_PRIVATE_H
#define SSL_PRIVATE_H

/* Exclude unused OpenSSL features
 * even if the OpenSSL supports them
 */
#ifndef OPENSSL_NO_IDEA
#define OPENSSL_NO_IDEA
#endif
#ifndef OPENSSL_NO_KRB5
#define OPENSSL_NO_KRB5
#endif
#ifndef OPENSSL_NO_MDC2
#define OPENSSL_NO_MDC2
#endif
#ifndef OPENSSL_NO_RC5
#define OPENSSL_NO_RC5
#endif

#include "apr_thread_rwlock.h"
#include "apr_atomic.h"
#include <stdbool.h>

/* OpenSSL headers */
#include <openssl/opensslv.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <openssl/x509.h>
#include <openssl/pem.h>
#include <openssl/pkcs12.h> 
#include <openssl/crypto.h>
#include <openssl/evp.h>
#include <openssl/rand.h>
#include <openssl/x509v3.h>

#define ERR_LEN 256

/* Avoid tripping over an engine build installed globally and detected
 * when the user points at an explicit non-engine flavor of OpenSSL
 */
#ifndef OPENSSL_NO_ENGINE
#include <openssl/engine.h>
#endif

#ifndef RAND_MAX
#include <limits.h>
#define RAND_MAX INT_MAX
#endif

/*
 * Define IDs for the temporary RSA keys and DH params
 */

#define SSL_TMP_KEY_DH_512      (1)
#define SSL_TMP_KEY_DH_1024     (2)
#define SSL_TMP_KEY_DH_2048     (3)
#define SSL_TMP_KEY_DH_4096     (4)
#define SSL_TMP_KEY_MAX         (5)

/*
 * Define the SSL Protocol options
 */
#define SSL_PROTOCOL_NONE       (0)
#define SSL_PROTOCOL_SSLV2      (1<<0)
#define SSL_PROTOCOL_SSLV3      (1<<1)
#define SSL_PROTOCOL_TLSV1      (1<<2)
#define SSL_PROTOCOL_TLSV1_1    (1<<3)
#define SSL_PROTOCOL_TLSV1_2    (1<<4)
/* TLS_*method according to https://www.openssl.org/docs/manmaster/ssl/SSL_CTX_new.html */
#define SSL_PROTOCOL_TLS        (SSL_PROTOCOL_SSLV3|SSL_PROTOCOL_TLSV1|SSL_PROTOCOL_TLSV1_1|SSL_PROTOCOL_TLSV1_2)
#define SSL_PROTOCOL_ALL        (SSL_PROTOCOL_SSLV2|SSL_PROTOCOL_TLS)

#define SSL_MODE_CLIENT         (0)
#define SSL_MODE_SERVER         (1)
#define SSL_MODE_COMBINED       (2)

#define SSL_DEFAULT_CACHE_SIZE  (256)
#define SSL_DEFAULT_VHOST_NAME  ("_default_:443")

#define SSL_CVERIFY_IGNORED             (-1)
#define SSL_CVERIFY_NONE                (0)
#define SSL_CVERIFY_OPTIONAL            (1)
#define SSL_CVERIFY_REQUIRED            (2)

#define SSL_TO_APR_ERROR(X)         (APR_OS_START_USERERR + 1000 + X)

#define MAX_ALPN_NPN_PROTO_SIZE 65535

extern const char* TCN_UNKNOWN_AUTH_METHOD;

/* ECC: make sure we have at least 1.0.0 */
#if !defined(OPENSSL_NO_EC) && defined(TLSEXT_ECPOINTFORMAT_uncompressed)
#define HAVE_ECC              1
#endif

/* OpenSSL 1.0.2 compatibility */
#if OPENSSL_VERSION_NUMBER < 0x10100000L || defined(LIBRESSL_VERSION_NUMBER)
#define TLS_method SSLv23_method
#define TLS_client_method SSLv23_client_method
#define TLS_server_method SSLv23_server_method
#define OPENSSL_VERSION SSLEAY_VERSION
#define OpenSSL_version SSLeay_version
#define OPENSSL_malloc_init CRYPTO_malloc_init
#define X509_REVOKED_get0_serialNumber(x) x->serialNumber
#define OpenSSL_version_num SSLeay
#define BIO_get_init(x)       ((x)->init)
#define BIO_set_init(x,v)     ((x)->init=(v))
#define BIO_get_data(x)       ((x)->ptr)
#define BIO_set_data(x,v)     ((x)->ptr=(v))
#define BIO_set_shutdown(x,v) ((x)->shutdown=(v))
#define BIO_get_shutdown(x)   ((x)->shutdown)
#endif /* OPENSSL_VERSION_NUMBER < 0x10100000L */

#define SSL_SELECTOR_FAILURE_NO_ADVERTISE                       0
#define SSL_SELECTOR_FAILURE_CHOOSE_MY_LAST_PROTOCOL            1

#define SSL_SESSION_TICKET_KEY_NAME_LEN 16
#define SSL_SESSION_TICKET_AES_KEY_LEN  16
#define SSL_SESSION_TICKET_HMAC_KEY_LEN 16
#define SSL_SESSION_TICKET_KEY_SIZE     48

extern void *SSL_temp_keys[SSL_TMP_KEY_MAX];

// HACK!
// LibreSSL 2.4.x doesn't support the X509_V_ERR_UNSPECIFIED so we introduce a work around to make sure a supported alert is used.
// This should be reverted when we support LibreSSL 2.5.x (which does support X509_V_ERR_UNSPECIFIED).
#ifndef X509_V_ERR_UNSPECIFIED
#define TCN_X509_V_ERR_UNSPECIFIED 99999
#else
#define TCN_X509_V_ERR_UNSPECIFIED (X509_V_ERR_UNSPECIFIED)
#endif /*X509_V_ERR_UNSPECIFIED*/

typedef struct tcn_ssl_ctxt_t tcn_ssl_ctxt_t;

typedef struct {
    unsigned char   key_name[SSL_SESSION_TICKET_KEY_NAME_LEN];
    unsigned char   hmac_key[SSL_SESSION_TICKET_HMAC_KEY_LEN];
    unsigned char   aes_key[SSL_SESSION_TICKET_AES_KEY_LEN];
} tcn_ssl_ticket_key_t;

typedef struct {
    int verify_depth;
    int verify_mode;
} tcn_ssl_verify_config_t;

struct tcn_ssl_ctxt_t {
    apr_pool_t*              pool;
    SSL_CTX*                 ctx;

    /* Holds the alpn protocols, each of them prefixed with the len of the protocol */
    unsigned char*           alpn_proto_data;
    unsigned char*           next_proto_data;

    /* for client or downstream server authentication */
    char*                    password;

    apr_thread_rwlock_t*     mutex; // Session ticket mutext
    tcn_ssl_ticket_key_t*    ticket_keys;

    /* certificate verifier callback */
    jobject                  verifier;
    jmethodID                verifier_method;

    jobject                  cert_requested_callback;
    jmethodID                cert_requested_callback_method;

    tcn_ssl_verify_config_t  verify_config;

    int                      protocol;
    /* we are one or the other */
    int                      mode;

    unsigned int             next_proto_len;
    int                      next_selector_failure_behavior;

    unsigned int             alpn_proto_len;
    int                      alpn_selector_failure_behavior;

    unsigned int             ticket_keys_len;
    unsigned int             pad;

    /* TLS ticket key session resumption statistics */

    // The client did not present a ticket and we issued a new one.
    apr_uint32_t             ticket_keys_new;
    // The client presented a ticket derived from the primary key
    apr_uint32_t             ticket_keys_resume;
    // The client presented a ticket derived from an older key, and we upgraded to the primary key.
    apr_uint32_t             ticket_keys_renew;
    // The client presented a ticket that did not match any key in the list.
    apr_uint32_t             ticket_keys_fail;

    unsigned char            context_id[SHA_DIGEST_LENGTH];
};

/*
 *  Additional Functions
 */
void        SSL_init_app_data_idx(void);
// The app_data2 is used to store the tcn_ssl_ctxt_t pointer for the SSL instance.
void       *SSL_get_app_data2(SSL *);
void        SSL_set_app_data2(SSL *, void *);
// The app_data3 is used to store the handshakeCount pointer for the SSL instance.
void       *SSL_get_app_data3(SSL *);
void        SSL_set_app_data3(SSL *, void *);
// The app_data4 is used to store the tcn_ssl_verify_config_t pointer for the SSL instance.
// This will initially point back to the tcn_ssl_ctxt_t in tcn_ssl_ctxt_t.
void       *SSL_get_app_data4(SSL *);
void        SSL_set_app_data4(SSL *, void *);
int         SSL_password_callback(char *, int, int, void *);
DH         *SSL_dh_get_tmp_param(int);
DH         *SSL_callback_tmp_DH(SSL *, int, int);
// The following provided callbacks will always return DH of a given length.
// See https://www.openssl.org/docs/manmaster/ssl/SSL_CTX_set_tmp_dh_callback.html
DH         *SSL_callback_tmp_DH_512(SSL *, int, int);
DH         *SSL_callback_tmp_DH_1024(SSL *, int, int);
DH         *SSL_callback_tmp_DH_2048(SSL *, int, int);
DH         *SSL_callback_tmp_DH_4096(SSL *, int, int);
int         SSL_CTX_use_certificate_chain(SSL_CTX *, const char *, bool);
int         SSL_CTX_use_certificate_chain_bio(SSL_CTX *, BIO *, bool);
int         SSL_CTX_use_client_CA_bio(SSL_CTX *, BIO *);
int         SSL_use_certificate_chain_bio(SSL *, BIO *, bool);
X509        *load_pem_cert_bio(const char *, const BIO *);
EVP_PKEY    *load_pem_key_bio(const char *, const BIO *);
int         tcn_set_verify_config(tcn_ssl_verify_config_t* c, jint tcn_mode, jint depth);
int         tcn_EVP_PKEY_up_ref(EVP_PKEY* pkey);
int         tcn_X509_up_ref(X509* cert);
int         SSL_callback_next_protos(SSL *, const unsigned char **, unsigned int *, void *);
int         SSL_callback_select_next_proto(SSL *, unsigned char **, unsigned char *, const unsigned char *, unsigned int,void *);
int         SSL_callback_alpn_select_proto(SSL *, const unsigned char **, unsigned char *, const unsigned char *, unsigned int, void *);
const char *SSL_cipher_authentication_method(const SSL_CIPHER *);

#if defined(__GNUC__) || defined(__GNUG__)
    // only supported with GCC, this will be used to support different openssl versions at the same time.
    extern int SSL_CTX_set_alpn_protos(SSL_CTX *ctx, const unsigned char *protos,
           unsigned protos_len) __attribute__((weak));
    extern void SSL_CTX_set_alpn_select_cb(SSL_CTX *ctx, int (*cb) (SSL *ssl, const unsigned char **out,
           unsigned char *outlen, const unsigned char *in, unsigned int inlen,
           void *arg), void *arg) __attribute__((weak));
    extern void SSL_get0_alpn_selected(const SSL *ssl, const unsigned char **data,
           unsigned *len) __attribute__((weak));
#endif

#endif /* SSL_PRIVATE_H */
