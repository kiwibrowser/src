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

#include <stdbool.h>
#include <openssl/bio.h>
#include "tcn.h"
#include "apr_file_io.h"
#include "apr_thread_mutex.h"
#include "apr_atomic.h"
#include "apr_strings.h"
#include "apr_portable.h"
#include "ssl_private.h"

static int ssl_initialized = 0;
extern apr_pool_t *tcn_global_pool;

ENGINE *tcn_ssl_engine = NULL;
void *SSL_temp_keys[SSL_TMP_KEY_MAX];

/* Global reference to the pool used by the dynamic mutexes */
static apr_pool_t *dynlockpool = NULL;

/* Dynamic lock structure */
struct CRYPTO_dynlock_value {
    apr_pool_t *pool;
    const char* file;
    int line;
    apr_thread_mutex_t *mutex;
};

struct TCN_bio_bytebuffer {
    // Pointer arithmetic is done on this variable. The type must correspond to a "byte" size.
    char* buffer;
    char* nonApplicationBuffer;
    jint  nonApplicationBufferSize;
    jint  nonApplicationBufferOffset;
    jint  nonApplicationBufferLength;
    jint  bufferLength;
    bool  bufferIsSSLWriteSink;
};

/*
 * Handle the Temporary RSA Keys and DH Params
 */

#define SSL_TMP_KEY_FREE(type, idx)                     \
    if (SSL_temp_keys[idx]) {                           \
        type##_free((type *)SSL_temp_keys[idx]);        \
        SSL_temp_keys[idx] = NULL;                      \
    } else (void)(0)

#define SSL_TMP_KEYS_FREE(type) \
    SSL_TMP_KEY_FREE(type, SSL_TMP_KEY_##type##_512);   \
    SSL_TMP_KEY_FREE(type, SSL_TMP_KEY_##type##_1024);  \
    SSL_TMP_KEY_FREE(type, SSL_TMP_KEY_##type##_2048);  \
    SSL_TMP_KEY_FREE(type, SSL_TMP_KEY_##type##_4096)

#define SSL_TMP_KEY_INIT_DH(bits)  \
    ssl_tmp_key_init_dh(bits, SSL_TMP_KEY_DH_##bits)

#define SSL_TMP_KEYS_INIT(R)                    \
    R |= SSL_TMP_KEY_INIT_DH(512);              \
    R |= SSL_TMP_KEY_INIT_DH(1024);             \
    R |= SSL_TMP_KEY_INIT_DH(2048);             \
    R |= SSL_TMP_KEY_INIT_DH(4096)

/*
 * supported_ssl_opts is a bitmask that contains all supported SSL_OP_*
 * options at compile-time. This is used in hasOp to determine which
 * SSL_OP_* options are available at runtime.
 *
 * Note that at least up through OpenSSL 0.9.8o, checking SSL_OP_ALL will
 * return JNI_FALSE because SSL_OP_ALL is a mask that covers all bug
 * workarounds for OpenSSL including future workarounds that are defined
 * to be in the least-significant 3 nibbles of the SSL_OP_* bit space.
 *
 * This implementation has chosen NOT to simply set all those lower bits
 * so that the return value for SSL_OP_FUTURE_WORKAROUND will only be
 * reported by versions that actually support that specific workaround.
 */
static const jint supported_ssl_opts = 0
/*
  Specifically skip SSL_OP_ALL
#ifdef SSL_OP_ALL
     | SSL_OP_ALL
#endif
*/
#ifdef SSL_OP_ALLOW_UNSAFE_LEGACY_RENEGOTIATION
     | SSL_OP_ALLOW_UNSAFE_LEGACY_RENEGOTIATION
#endif

#ifdef SSL_OP_CIPHER_SERVER_PREFERENCE
     | SSL_OP_CIPHER_SERVER_PREFERENCE
#endif

#ifdef SSL_OP_CRYPTOPRO_TLSEXT_BUG
     | SSL_OP_CRYPTOPRO_TLSEXT_BUG
#endif

#ifdef SSL_OP_DONT_INSERT_EMPTY_FRAGMENTS
     | SSL_OP_DONT_INSERT_EMPTY_FRAGMENTS
#endif

#ifdef SSL_OP_LEGACY_SERVER_CONNECT
     | SSL_OP_LEGACY_SERVER_CONNECT
#endif

#ifdef SSL_OP_MICROSOFT_BIG_SSLV3_BUFFER
     | SSL_OP_MICROSOFT_BIG_SSLV3_BUFFER
#endif

#ifdef SSL_OP_MICROSOFT_SESS_ID_BUG
     | SSL_OP_MICROSOFT_SESS_ID_BUG
#endif

#ifdef SSL_OP_MSIE_SSLV2_RSA_PADDING
     | SSL_OP_MSIE_SSLV2_RSA_PADDING
#endif

#ifdef SSL_OP_NETSCAPE_CA_DN_BUG
     | SSL_OP_NETSCAPE_CA_DN_BUG
#endif

#ifdef SSL_OP_NETSCAPE_CHALLENGE_BUG
     | SSL_OP_NETSCAPE_CHALLENGE_BUG
#endif

#ifdef SSL_OP_NETSCAPE_DEMO_CIPHER_CHANGE_BUG
     | SSL_OP_NETSCAPE_DEMO_CIPHER_CHANGE_BUG
#endif

#ifdef SSL_OP_NETSCAPE_REUSE_CIPHER_CHANGE_BUG
     | SSL_OP_NETSCAPE_REUSE_CIPHER_CHANGE_BUG
#endif

#ifdef SSL_OP_NO_COMPRESSION
     | SSL_OP_NO_COMPRESSION
#endif

#ifdef SSL_OP_NO_QUERY_MTU
     | SSL_OP_NO_QUERY_MTU
#endif

#ifdef SSL_OP_NO_SESSION_RESUMPTION_ON_RENEGOTIATION
     | SSL_OP_NO_SESSION_RESUMPTION_ON_RENEGOTIATION
#endif

#ifdef SSL_OP_NO_SSLv2
     | SSL_OP_NO_SSLv2
#endif

#ifdef SSL_OP_NO_SSLv3
     | SSL_OP_NO_SSLv3
#endif

#ifdef SSL_OP_NO_TICKET
     | SSL_OP_NO_TICKET
#endif

#ifdef SSL_OP_NO_TLSv1
     | SSL_OP_NO_TLSv1
#endif

#ifdef SSL_OP_PKCS1_CHECK_1
     | SSL_OP_PKCS1_CHECK_1
#endif

#ifdef SSL_OP_PKCS1_CHECK_2
     | SSL_OP_PKCS1_CHECK_2
#endif

#ifdef SSL_OP_NO_TLSv1_1
     | SSL_OP_NO_TLSv1_1
#endif

#ifdef SSL_OP_NO_TLSv1_2
     | SSL_OP_NO_TLSv1_2
#endif

#ifdef SSL_OP_SINGLE_DH_USE
     | SSL_OP_SINGLE_DH_USE
#endif

#ifdef SSL_OP_SINGLE_ECDH_USE
     | SSL_OP_SINGLE_ECDH_USE
#endif

#ifdef SSL_OP_SSLEAY_080_CLIENT_DH_BUG
     | SSL_OP_SSLEAY_080_CLIENT_DH_BUG
#endif

#ifdef SSL_OP_SSLREF2_REUSE_CERT_TYPE_BUG
     | SSL_OP_SSLREF2_REUSE_CERT_TYPE_BUG
#endif

#ifdef SSL_OP_TLS_BLOCK_PADDING_BUG
     | SSL_OP_TLS_BLOCK_PADDING_BUG
#endif

#ifdef SSL_OP_TLS_D5_BUG
     | SSL_OP_TLS_D5_BUG
#endif

#ifdef SSL_OP_TLS_ROLLBACK_BUG
     | SSL_OP_TLS_ROLLBACK_BUG
#endif
     | 0;

static jint tcn_flush_sslbuffer_to_bytebuffer(struct TCN_bio_bytebuffer* bioUserData) {
    jint writeAmount = TCN_MIN(bioUserData->bufferLength, bioUserData->nonApplicationBufferLength) * sizeof(char);
    jint writeChunk = bioUserData->nonApplicationBufferSize - bioUserData->nonApplicationBufferOffset;

#ifdef NETTY_TCNATIVE_BIO_DEBUG
    fprintf(stderr, "tcn_flush_sslbuffer_to_bytebuffer1 bioUserData->nonApplicationBufferLength %d bioUserData->nonApplicationBufferOffset %d writeChunk %d writeAmount %d\n", bioUserData->nonApplicationBufferLength, bioUserData->nonApplicationBufferOffset, writeChunk, writeAmount);
#endif

    // check if we need to account for wrap around when draining the internal SSL buffer.
    if (writeAmount > writeChunk) {
        jint newnonApplicationBufferOffset = writeAmount - writeChunk;
        memcpy(bioUserData->buffer, &bioUserData->nonApplicationBuffer[bioUserData->nonApplicationBufferOffset], (size_t) writeChunk);
        memcpy(&bioUserData->buffer[writeChunk], bioUserData->nonApplicationBuffer, (size_t) newnonApplicationBufferOffset);
        bioUserData->nonApplicationBufferOffset = newnonApplicationBufferOffset;
    } else {
        memcpy(bioUserData->buffer, &bioUserData->nonApplicationBuffer[bioUserData->nonApplicationBufferOffset], (size_t) writeAmount);
        bioUserData->nonApplicationBufferOffset += writeAmount;
    }
    bioUserData->nonApplicationBufferLength -= writeAmount;
    bioUserData->bufferLength -= writeAmount;
    bioUserData->buffer += writeAmount; // Pointer arithmetic based on char* type

    if (bioUserData->nonApplicationBufferLength == 0) {
        bioUserData->nonApplicationBufferOffset = 0;
    }

#ifdef NETTY_TCNATIVE_BIO_DEBUG
    fprintf(stderr, "tcn_flush_sslbuffer_to_bytebuffer2 bioUserData->nonApplicationBufferLength %d bioUserData->nonApplicationBufferOffset %d\n", bioUserData->nonApplicationBufferLength, bioUserData->nonApplicationBufferOffset);
#endif

    return writeAmount;
}

static jint tcn_write_to_bytebuffer(BIO* bio, const char* in, int inl) {
    jint writeAmount = 0;
    jint writeChunk;
    struct TCN_bio_bytebuffer* bioUserData = (struct TCN_bio_bytebuffer*) BIO_get_data(bio);
    TCN_ASSERT(bioUserData != NULL);

#ifdef NETTY_TCNATIVE_BIO_DEBUG
    fprintf(stderr, "tcn_write_to_bytebuffer bioUserData->bufferIsSSLWriteSink %d inl %d [%.*s]\n", bioUserData->bufferIsSSLWriteSink, inl, inl, in);
#endif

    if (in == NULL || inl <= 0) {
        return 0;
    }

    // If the buffer is currently being used for reading then we have to use the internal SSL buffer to queue the data.
    if (!bioUserData->bufferIsSSLWriteSink) {
        jint nonApplicationBufferFreeSpace = bioUserData->nonApplicationBufferSize - bioUserData->nonApplicationBufferLength;
        jint startIndex;

#ifdef NETTY_TCNATIVE_BIO_DEBUG
       fprintf(stderr, "tcn_write_to_bytebuffer nonApplicationBufferFreeSpace %d\n", nonApplicationBufferFreeSpace);
#endif
        if (nonApplicationBufferFreeSpace == 0) {
            BIO_set_retry_write(bio); /* buffer is full */
            return -1;
        }

        writeAmount = TCN_MIN(nonApplicationBufferFreeSpace, (jint) inl) * sizeof(char);
        startIndex = bioUserData->nonApplicationBufferOffset + bioUserData->nonApplicationBufferLength;
        writeChunk = bioUserData->nonApplicationBufferSize - startIndex;

#ifdef NETTY_TCNATIVE_BIO_DEBUG
        fprintf(stderr, "tcn_write_to_bytebuffer bioUserData->nonApplicationBufferLength %d bioUserData->nonApplicationBufferOffset %d startIndex %d writeChunk %d writeAmount %d\n", bioUserData->nonApplicationBufferLength, bioUserData->nonApplicationBufferOffset, startIndex, writeChunk, writeAmount);
#endif

        // check if the write will wrap around the buffer.
        if (writeAmount > writeChunk) {
            memcpy(&bioUserData->nonApplicationBuffer[startIndex], in, (size_t) writeChunk);
            memcpy(bioUserData->nonApplicationBuffer, &in[writeChunk], (size_t) (writeAmount - writeChunk));
        } else {
            memcpy(&bioUserData->nonApplicationBuffer[startIndex], in, (size_t) writeAmount);
        }
        bioUserData->nonApplicationBufferLength += writeAmount;
        // This write amount will not be used by Java, and doesn't correlate to the ByteBuffer source.
        // The internal SSL buffer exists because a SSL_read operation may actually write data (e.g. handshake).
        return writeAmount;
    }

    if (bioUserData->buffer == NULL || bioUserData->bufferLength == 0) {
        BIO_set_retry_write(bio); /* no buffer to write into */
        return -1;
    }

    // First check if we need to drain data queued in the internal SSL buffer.
    if (bioUserData->nonApplicationBufferLength != 0) {
        writeAmount = tcn_flush_sslbuffer_to_bytebuffer(bioUserData);
    }

    // Next write "in" into what ever space the ByteBuffer has available.
    writeChunk = TCN_MIN(bioUserData->bufferLength, (jint) inl) * sizeof(char);

#ifdef NETTY_TCNATIVE_BIO_DEBUG
    fprintf(stderr, "tcn_write_to_bytebuffer2 writeChunk %d\n", writeChunk);
#endif

    memcpy(bioUserData->buffer, in, (size_t) writeChunk);
    bioUserData->bufferLength -= writeChunk;
    bioUserData->buffer += writeChunk; // Pointer arithmetic based on char* type

    return writeAmount + writeChunk;
}

static jint tcn_read_from_bytebuffer(BIO* bio, char *out, int outl) {
    jint readAmount;
    struct TCN_bio_bytebuffer* bioUserData = (struct TCN_bio_bytebuffer*) BIO_get_data(bio);
    TCN_ASSERT(bioUserData != NULL);

#ifdef NETTY_TCNATIVE_BIO_DEBUG
    fprintf(stderr, "tcn_read_from_bytebuffer bioUserData->bufferIsSSLWriteSink %d outl %d [%.*s]\n", bioUserData->bufferIsSSLWriteSink, outl, outl, out);
#endif

    if (out == NULL || outl <= 0) {
        return 0;
    }

    if (bioUserData->bufferIsSSLWriteSink || bioUserData->buffer == NULL || bioUserData->bufferLength == 0) {
        // During handshake this may happen, and it means we are not setup to read yet.
        BIO_set_retry_read(bio);
        return -1;
    }

    readAmount = TCN_MIN(bioUserData->bufferLength, (jint) outl) * sizeof(char);

#ifdef NETTY_TCNATIVE_BIO_DEBUG
    fprintf(stderr, "tcn_read_from_bytebuffer readAmount %d\n", readAmount);
#endif

    memcpy(out, bioUserData->buffer, (size_t) readAmount);
    bioUserData->bufferLength -= readAmount;
    bioUserData->buffer += readAmount; // Pointer arithmetic based on char* type

    return readAmount;
}

static int bio_java_bytebuffer_create(BIO* bio) {
    struct TCN_bio_bytebuffer* bioUserData = (struct TCN_bio_bytebuffer*) OPENSSL_malloc(sizeof(struct TCN_bio_bytebuffer));
    if (bioUserData == NULL) {
        return 0;
    }
    // The actual ByteBuffer is set from java and may be swapped out for each operation.
    bioUserData->buffer = NULL;
    bioUserData->bufferLength = 0;
    bioUserData->bufferIsSSLWriteSink = false;
    bioUserData->nonApplicationBuffer = NULL;
    bioUserData->nonApplicationBufferSize = 0;
    bioUserData->nonApplicationBufferOffset = 0;
    bioUserData->nonApplicationBufferLength = 0;

    BIO_set_data(bio, bioUserData);

    // In order to for OpenSSL to properly manage the lifetime of a BIO it relies on some shutdown and init state.
    // The behavior expected by OpenSSL can be found here: https://www.openssl.org/docs/man1.1.0/crypto/BIO_set_data.html
    BIO_set_shutdown(bio, 1);
    BIO_set_init(bio, 1);

    return 1;
}

static int bio_java_bytebuffer_destroy(BIO* bio) {
    struct TCN_bio_bytebuffer* bioUserData;

    if (bio == NULL) {
        return 0;
    }

    bioUserData = (struct TCN_bio_bytebuffer*) BIO_get_data(bio);
    if (bioUserData == NULL) {
        return 1;
    }

    if (bioUserData->nonApplicationBuffer != NULL) {
        OPENSSL_free(bioUserData->nonApplicationBuffer);
        bioUserData->nonApplicationBuffer = NULL;
    }

    // The buffer is not owned by tcn, so just free the native memory.
    OPENSSL_free(bioUserData);
    BIO_set_data(bio, NULL);

    return 1;
}

static int bio_java_bytebuffer_write(BIO* bio, const char* in, int inl) {
    BIO_clear_retry_flags(bio);
    return (int) tcn_write_to_bytebuffer(bio, in, inl);
}

static int bio_java_bytebuffer_read(BIO* bio, char* out, int outl) {
    BIO_clear_retry_flags(bio);
    return (int) tcn_read_from_bytebuffer(bio, out, outl);
}

static int bio_java_bytebuffer_puts(BIO* bio, const char *in) {
    BIO_clear_retry_flags(bio);
    return (int) tcn_write_to_bytebuffer(bio, in, strlen(in));
}

static int bio_java_bytebuffer_gets(BIO* b, char* out, int outl) {
    // Not supported https://www.openssl.org/docs/man1.0.2/crypto/BIO_write.html
    return -2;
}

static long bio_java_bytebuffer_ctrl(BIO* bio, int cmd, long num, void* ptr) {
    // see https://www.openssl.org/docs/man1.0.1/crypto/BIO_ctrl.html
    switch (cmd) {
        case BIO_CTRL_GET_CLOSE:
            return (long) BIO_get_shutdown(bio);
        case BIO_CTRL_SET_CLOSE:
            BIO_set_shutdown(bio, (int) num);
            return 1;
        case BIO_CTRL_FLUSH:
            return 1;
        default:
            return 0;
    }
}

TCN_IMPLEMENT_CALL(jint, SSL, bioLengthByteBuffer)(TCN_STDARGS, jlong bioAddress) {
    BIO* bio = J2P(bioAddress, BIO*);
    struct TCN_bio_bytebuffer* bioUserData;

    if (bio == NULL) {
        tcn_ThrowException(e, "bio is null");
        return 0;
    }

    bioUserData = (struct TCN_bio_bytebuffer*) BIO_get_data(bio);
    return bioUserData == NULL ? 0 : bioUserData->bufferLength;
}

TCN_IMPLEMENT_CALL(jint, SSL, bioLengthNonApplication)(TCN_STDARGS, jlong bioAddress) {
    BIO* bio = J2P(bioAddress, BIO*);
    struct TCN_bio_bytebuffer* bioUserData;

    if (bio == NULL) {
        tcn_ThrowException(e, "bio is null");
        return 0;
    }

    bioUserData = (struct TCN_bio_bytebuffer*) BIO_get_data(bio);
    return bioUserData == NULL ? 0 : bioUserData->nonApplicationBufferLength;
}

#if OPENSSL_VERSION_NUMBER < 0x10100000L || defined(LIBRESSL_VERSION_NUMBER)
static BIO_METHOD bio_java_bytebuffer_methods = {
    BIO_TYPE_MEM,
    "Java ByteBuffer",
    bio_java_bytebuffer_write,
    bio_java_bytebuffer_read,
    bio_java_bytebuffer_puts,
    bio_java_bytebuffer_gets,
    bio_java_bytebuffer_ctrl,
    bio_java_bytebuffer_create,
    bio_java_bytebuffer_destroy,
    NULL
};
#else
static BIO_METHOD* bio_java_bytebuffer_methods = NULL;

static void init_bio_methods(void) {
    bio_java_bytebuffer_methods = BIO_meth_new(BIO_TYPE_MEM, "Java ByteBuffer");
    BIO_meth_set_write(bio_java_bytebuffer_methods, &bio_java_bytebuffer_write);
    BIO_meth_set_read(bio_java_bytebuffer_methods, &bio_java_bytebuffer_read);
    BIO_meth_set_puts(bio_java_bytebuffer_methods, &bio_java_bytebuffer_puts);
    BIO_meth_set_gets(bio_java_bytebuffer_methods, &bio_java_bytebuffer_gets);
    BIO_meth_set_ctrl(bio_java_bytebuffer_methods, &bio_java_bytebuffer_ctrl);
    BIO_meth_set_create(bio_java_bytebuffer_methods, &bio_java_bytebuffer_create);
    BIO_meth_set_destroy(bio_java_bytebuffer_methods, &bio_java_bytebuffer_destroy);
}

static void free_bio_methods(void) {
    BIO_meth_free(bio_java_bytebuffer_methods);
}
#endif

static BIO_METHOD* BIO_java_bytebuffer() {
#if OPENSSL_VERSION_NUMBER < 0x10100000L || defined(LIBRESSL_VERSION_NUMBER)
    return &bio_java_bytebuffer_methods;
#else
    return bio_java_bytebuffer_methods;
#endif
}

static int ssl_tmp_key_init_dh(int bits, int idx)
{
#if (OPENSSL_VERSION_NUMBER < 0x10100000L) || defined(OPENSSL_USE_DEPRECATED) || defined(LIBRESSL_VERSION_NUMBER)
    return (SSL_temp_keys[idx] = SSL_dh_get_tmp_param(bits)) ? 0 : 1;
#else
    return 0;
#endif
}

TCN_IMPLEMENT_CALL(jint, SSL, version)(TCN_STDARGS)
{
    UNREFERENCED_STDARGS;
    return OpenSSL_version_num();
}

TCN_IMPLEMENT_CALL(jstring, SSL, versionString)(TCN_STDARGS)
{
    UNREFERENCED(o);
    return AJP_TO_JSTRING(OpenSSL_version(OPENSSL_VERSION));
}

/*
 *  the various processing hooks
 */
static apr_status_t ssl_init_cleanup(void *data)
{
    UNREFERENCED(data);

    if (!ssl_initialized)
        return APR_SUCCESS;
    ssl_initialized = 0;

    SSL_TMP_KEYS_FREE(DH);
    /*
     * Try to kill the internals of the SSL library.
     */
#if OPENSSL_VERSION_NUMBER >= 0x00907001 && !defined(OPENSSL_IS_BORINGSSL)
    /* Corresponds to OPENSSL_load_builtin_modules():
     * XXX: borrowed from apps.h, but why not CONF_modules_free()
     * which also invokes CONF_modules_finish()?
     */
    CONF_modules_unload(1);
#endif
    /* Corresponds to SSL_library_init: */
    EVP_cleanup();
#if HAVE_ENGINE_LOAD_BUILTIN_ENGINES
    ENGINE_cleanup();
#endif
#if OPENSSL_VERSION_NUMBER >= 0x00907001
    CRYPTO_cleanup_all_ex_data();
#endif
#if OPENSSL_VERSION_NUMBER < 0x10100000L
    ERR_remove_thread_state(NULL);
#endif
#if OPENSSL_VERSION_NUMBER >= 0x10100000L && !defined(LIBRESSL_VERSION_NUMBER)
    free_bio_methods();
#endif

    /* Don't call ERR_free_strings here; ERR_load_*_strings only
     * actually load the error strings once per process due to static
     * variable abuse in OpenSSL. */

    /*
     * TODO: determine somewhere we can safely shove out diagnostics
     *       (when enabled) at this late stage in the game:
     * CRYPTO_mem_leaks_fp(stderr);
     */
    return APR_SUCCESS;
}

#ifndef OPENSSL_NO_ENGINE
/* Try to load an engine in a shareable library */
static ENGINE *ssl_try_load_engine(const char *engine)
{
    ENGINE *e = ENGINE_by_id("dynamic");
    if (e) {
        if (!ENGINE_ctrl_cmd_string(e, "SO_PATH", engine, 0)
            || !ENGINE_ctrl_cmd_string(e, "LOAD", NULL, 0)) {
            ENGINE_free(e);
            e = NULL;
        }
    }
    return e;
}
#endif

/*
 * To ensure thread-safetyness in OpenSSL
 */

static apr_thread_mutex_t **ssl_lock_cs;
static int                  ssl_lock_num_locks;

static void ssl_thread_lock(int mode, int type,
                            const char *file, int line)
{
    UNREFERENCED(file);
    UNREFERENCED(line);
    if (type < ssl_lock_num_locks) {
        if (mode & CRYPTO_LOCK) {
            apr_thread_mutex_lock(ssl_lock_cs[type]);
        }
        else {
            apr_thread_mutex_unlock(ssl_lock_cs[type]);
        }
    }
}

static unsigned long ssl_thread_id(void)
{
    /* OpenSSL needs this to return an unsigned long.  On OS/390, the pthread
     * id is a structure twice that big.  Use the TCB pointer instead as a
     * unique unsigned long.
     */
#ifdef __MVS__
    struct PSA {
        char unmapped[540];
        unsigned long PSATOLD;
    } *psaptr = 0;

    return psaptr->PSATOLD;
#elif defined(WIN32)
    return (unsigned long)GetCurrentThreadId();
#else
    return (unsigned long)(apr_os_thread_current());
#endif
}

static void ssl_set_thread_id(CRYPTO_THREADID *id)
{
    CRYPTO_THREADID_set_numeric(id, ssl_thread_id());
}

static apr_status_t ssl_thread_cleanup(void *data)
{
    UNREFERENCED(data);
    CRYPTO_set_locking_callback(NULL);
    CRYPTO_THREADID_set_callback(NULL);
    CRYPTO_set_dynlock_create_callback(NULL);
    CRYPTO_set_dynlock_lock_callback(NULL);
    CRYPTO_set_dynlock_destroy_callback(NULL);

    dynlockpool = NULL;

    /* Let the registered mutex cleanups do their own thing
     */
    return APR_SUCCESS;
}

/*
 * Dynamic lock creation callback
 */
static struct CRYPTO_dynlock_value *ssl_dyn_create_function(const char *file,
                                                     int line)
{
    struct CRYPTO_dynlock_value *value;
    apr_pool_t *p;
    apr_status_t rv;

    /*
     * We need a pool to allocate our mutex.  Since we can't clear
     * allocated memory from a pool, create a subpool that we can blow
     * away in the destruction callback.
     */
    rv = apr_pool_create(&p, dynlockpool);
    if (rv != APR_SUCCESS) {
        /* TODO log that fprintf(stderr, "Failed to create subpool for dynamic lock"); */
        return NULL;
    }

    value = (struct CRYPTO_dynlock_value *)apr_palloc(p,
                                                      sizeof(struct CRYPTO_dynlock_value));
    if (!value) {
        /* TODO log that fprintf(stderr, "Failed to allocate dynamic lock structure"); */
        return NULL;
    }

    value->pool = p;
    /* Keep our own copy of the place from which we were created,
       using our own pool. */
    value->file = apr_pstrdup(p, file);
    value->line = line;
    rv = apr_thread_mutex_create(&(value->mutex), APR_THREAD_MUTEX_DEFAULT,
                                p);
    if (rv != APR_SUCCESS) {
        /* TODO log that fprintf(stderr, "Failed to create thread mutex for dynamic lock"); */
        apr_pool_destroy(p);
        return NULL;
    }
    return value;
}

/*
 * Dynamic locking and unlocking function
 */
static void ssl_dyn_lock_function(int mode, struct CRYPTO_dynlock_value *l,
                           const char *file, int line)
{
    if (mode & CRYPTO_LOCK) {
        apr_thread_mutex_lock(l->mutex);
    }
    else {
        apr_thread_mutex_unlock(l->mutex);
    }
}

/*
 * Dynamic lock destruction callback
 */
static void ssl_dyn_destroy_function(struct CRYPTO_dynlock_value *l,
                          const char *file, int line)
{
    apr_status_t rv;
    rv = apr_thread_mutex_destroy(l->mutex);
    if (rv != APR_SUCCESS) {
        /* TODO log that fprintf(stderr, "Failed to destroy mutex for dynamic lock %s:%d", l->file, l->line); */
    }

    /* Trust that whomever owned the CRYPTO_dynlock_value we were
     * passed has no future use for it...
     */
    apr_pool_destroy(l->pool);
}

static void ssl_thread_setup(apr_pool_t *p)
{
    int i;

    ssl_lock_num_locks = CRYPTO_num_locks();
    ssl_lock_cs = apr_palloc(p, ssl_lock_num_locks * sizeof(*ssl_lock_cs));

    for (i = 0; i < ssl_lock_num_locks; i++) {
        apr_thread_mutex_create(&(ssl_lock_cs[i]),
                                APR_THREAD_MUTEX_DEFAULT, p);
    }

    CRYPTO_THREADID_set_callback(ssl_set_thread_id);
    CRYPTO_set_locking_callback(ssl_thread_lock);

    /* Set up dynamic locking scaffolding for OpenSSL to use at its
     * convenience.
     */
    dynlockpool = p;
    CRYPTO_set_dynlock_create_callback(ssl_dyn_create_function);
    CRYPTO_set_dynlock_lock_callback(ssl_dyn_lock_function);
    CRYPTO_set_dynlock_destroy_callback(ssl_dyn_destroy_function);

    apr_pool_cleanup_register(p, NULL, ssl_thread_cleanup,
                              apr_pool_cleanup_null);
}

TCN_IMPLEMENT_CALL(jint, SSL, initialize)(TCN_STDARGS, jstring engine)
{
    int r = 0;

    TCN_ALLOC_CSTRING(engine);

    UNREFERENCED(o);
    if (!tcn_global_pool) {
        TCN_FREE_CSTRING(engine);
        tcn_ThrowAPRException(e, APR_EINVAL);
        return (jint)APR_EINVAL;
    }
    /* Check if already initialized */
    if (ssl_initialized++) {
        TCN_FREE_CSTRING(engine);
        return (jint)APR_SUCCESS;
    }

#if OPENSSL_VERSION_NUMBER < 0x10100000L
    if (SSLeay() < 0x0090700L) {
        TCN_FREE_CSTRING(engine);
        tcn_ThrowAPRException(e, APR_EINVAL);
        ssl_initialized = 0;
        return (jint)APR_EINVAL;
    }
#endif

#if OPENSSL_VERSION_NUMBER < 0x10100000L || defined(LIBRESSL_VERSION_NUMBER)
    /* We must register the library in full, to ensure our configuration
     * code can successfully test the SSL environment.
     */
    OPENSSL_malloc_init();
#endif

    ERR_load_crypto_strings();
    SSL_load_error_strings();
    SSL_library_init();
    OpenSSL_add_all_algorithms();
#if HAVE_ENGINE_LOAD_BUILTIN_ENGINES
    ENGINE_load_builtin_engines();
#endif
#if OPENSSL_VERSION_NUMBER >= 0x00907001
    OPENSSL_load_builtin_modules();
#endif

    /* Initialize thread support */
    ssl_thread_setup(tcn_global_pool);

#ifndef OPENSSL_NO_ENGINE
    if (J2S(engine)) {
        ENGINE *ee = NULL;
        apr_status_t err = APR_SUCCESS;
        if(strcmp(J2S(engine), "auto") == 0) {
            ENGINE_register_all_complete();
        }
        else {
            if ((ee = ENGINE_by_id(J2S(engine))) == NULL
                && (ee = ssl_try_load_engine(J2S(engine))) == NULL)
                err = APR_ENOTIMPL;
            else {
#ifdef ENGINE_CTRL_CHIL_SET_FORKCHECK
                if (strcmp(J2S(engine), "chil") == 0)
                    ENGINE_ctrl(ee, ENGINE_CTRL_CHIL_SET_FORKCHECK, 1, 0, 0);
#endif
                if (!ENGINE_set_default(ee, ENGINE_METHOD_ALL))
                    err = APR_ENOTIMPL;
            }
            /* Free our "structural" reference. */
            if (ee)
                ENGINE_free(ee);
        }
        if (err != APR_SUCCESS) {
            TCN_FREE_CSTRING(engine);
            ssl_init_cleanup(NULL);
            tcn_ThrowAPRException(e, err);
            return (jint)err;
        }
        tcn_ssl_engine = ee;
    }
#endif

    // For SSL_get_app_data*() at request time
    SSL_init_app_data_idx();

#if OPENSSL_VERSION_NUMBER >= 0x10100000L && !defined(LIBRESSL_VERSION_NUMBER)
    init_bio_methods();
#endif

    SSL_TMP_KEYS_INIT(r);
    if (r) {
        ERR_clear_error();
        TCN_FREE_CSTRING(engine);
        ssl_init_cleanup(NULL);
        tcn_ThrowAPRException(e, APR_ENOTIMPL);
        return APR_ENOTIMPL;
    }
    /*
     * Let us cleanup the ssl library when the library is unloaded
     */
    apr_pool_cleanup_register(tcn_global_pool, NULL,
                              ssl_init_cleanup,
                              apr_pool_cleanup_null);
    TCN_FREE_CSTRING(engine);

    return (jint)APR_SUCCESS;
}

TCN_IMPLEMENT_CALL(jlong, SSL, newMemBIO)(TCN_STDARGS)
{
    BIO *bio = NULL;

    UNREFERENCED(o);

    // TODO: Use BIO_s_secmem() once included in stable release
    if ((bio = BIO_new(BIO_s_mem())) == NULL) {
        tcn_ThrowException(e, "Create BIO failed");
        return 0;
    }
    return P2J(bio);
}

TCN_IMPLEMENT_CALL(jstring, SSL, getLastError)(TCN_STDARGS)
{
    char buf[ERR_LEN];
    UNREFERENCED(o);
    ERR_error_string(ERR_get_error(), buf);
    return tcn_new_string(e, buf);
}

TCN_IMPLEMENT_CALL(jboolean, SSL, hasOp)(TCN_STDARGS, jint op)
{
    return op == (op & supported_ssl_opts) ? JNI_TRUE : JNI_FALSE;
}

/*** Begin Twitter 1:1 API addition ***/
TCN_IMPLEMENT_CALL(jint, SSL, getLastErrorNumber)(TCN_STDARGS) {
    UNREFERENCED_STDARGS;
    return ERR_get_error();
}

static void ssl_info_callback(const SSL *ssl, int where, int ret) {
    int *handshakeCount = NULL;
    if (0 != (where & SSL_CB_HANDSHAKE_START)) {
        handshakeCount = (int*) SSL_get_app_data3((SSL*) ssl);
        if (handshakeCount != NULL) {
            ++(*handshakeCount);
        }
    }
}

TCN_IMPLEMENT_CALL(jlong /* SSL * */, SSL, newSSL)(TCN_STDARGS,
                                                   jlong ctx /* tcn_ssl_ctxt_t * */,
                                                   jboolean server) {
    SSL *ssl = NULL;
    int *handshakeCount = NULL;
    tcn_ssl_ctxt_t *c = J2P(ctx, tcn_ssl_ctxt_t *);

    if (c == NULL) {
        tcn_ThrowException(e, "ssl ctx is null");
        return 0;
    }
    if (c->ctx == NULL) {
        tcn_ThrowException(e, "ctx is null");
        return 0;
    }

    UNREFERENCED_STDARGS;

    ssl = SSL_new(c->ctx);
    if (ssl == NULL) {
        tcn_ThrowException(e, "cannot create new ssl");
        return 0;
    }

    // Set the app_data2 before all the others because it may be used in SSL_free.
    SSL_set_app_data2(ssl, c);

    // Initially we will share the configuration from the SSLContext.
    // Set this before other app_data because there is no chance of failure, and if other app_data initialization fails
    // SSL_free maybe called and the state of this variable is assumed to be initalized.
    SSL_set_app_data4(ssl, &c->verify_config);

    // Store the handshakeCount in the SSL instance.
    handshakeCount = (int*) OPENSSL_malloc(sizeof(int));
    if (handshakeCount == NULL) {
        SSL_free(ssl);
        tcn_ThrowException(e, "cannot create handshakeCount user data");
        return 0;
    }

    *handshakeCount = 0;
    SSL_set_app_data3(ssl, handshakeCount);

    // Add callback to keep track of handshakes.
    SSL_CTX_set_info_callback(c->ctx, ssl_info_callback);

    if (server) {
        SSL_set_accept_state(ssl);
    } else {
        SSL_set_connect_state(ssl);
    }

    return P2J(ssl);
}

TCN_IMPLEMENT_CALL(jint, SSL, getError)(TCN_STDARGS,
                                       jlong ssl /* SSL * */,
                                       jint ret) {
    SSL *ssl_ = J2P(ssl, SSL *);

    if (ssl_ == NULL) {
        tcn_ThrowException(e, "ssl is null");
        return 0;
    }

    UNREFERENCED_STDARGS;

    return SSL_get_error(ssl_, ret);
}

// Write wlen bytes from wbuf into bio
TCN_IMPLEMENT_CALL(jint /* status */, SSL, bioWrite)(TCN_STDARGS,
                                                     jlong bioAddress /* BIO* */,
                                                     jlong wbufAddress /* char* */,
                                                     jint wlen /* sizeof(wbuf) */) {
    BIO* bio = J2P(bioAddress, BIO*);
    void* wbuf = J2P(wbufAddress, void*);

    if (bio == NULL) {
        tcn_ThrowException(e, "bio is null");
        return 0;
    }
    if (wbuf == NULL) {
        tcn_ThrowException(e, "wbuf is null");
        return 0;
    }

    UNREFERENCED_STDARGS;

    return BIO_write(bio, wbuf, wlen);
}

TCN_IMPLEMENT_CALL(void, SSL, bioSetByteBuffer)(TCN_STDARGS,
                                                jlong bioAddress /* BIO* */,
                                                jlong bufferAddress /* Address for direct memory */,
                                                jint maxUsableBytes /* max number of bytes to use */,
                                                jboolean isSSLWriteSink) {
    BIO* bio = J2P(bioAddress, BIO*);
    char* buffer = J2P(bufferAddress, char*);
    struct TCN_bio_bytebuffer* bioUserData = NULL;
    TCN_ASSERT(bio != NULL);
    TCN_ASSERT(buffer != NULL);

    bioUserData = (struct TCN_bio_bytebuffer*) BIO_get_data(bio);
    TCN_ASSERT(bioUserData != NULL);

    bioUserData->buffer = buffer;
    bioUserData->bufferLength = maxUsableBytes;
    bioUserData->bufferIsSSLWriteSink = (bool) isSSLWriteSink;
}

TCN_IMPLEMENT_CALL(void, SSL, bioClearByteBuffer)(TCN_STDARGS, jlong bioAddress) {
    BIO* bio = J2P(bioAddress, BIO*);
    struct TCN_bio_bytebuffer* bioUserData = NULL;

    if (bio == NULL || (bioUserData = (struct TCN_bio_bytebuffer*) BIO_get_data(bio)) == NULL) {
        return;
    }

    bioUserData->buffer = NULL;
    bioUserData->bufferLength = 0;
    bioUserData->bufferIsSSLWriteSink = false;
}

TCN_IMPLEMENT_CALL(jint, SSL, bioFlushByteBuffer)(TCN_STDARGS, jlong bioAddress) {
    BIO* bio = J2P(bioAddress, BIO*);
    struct TCN_bio_bytebuffer* bioUserData;

    return (bio == NULL ||
           (bioUserData = (struct TCN_bio_bytebuffer*) BIO_get_data(bio)) == NULL ||
            bioUserData->nonApplicationBufferLength == 0 ||
            bioUserData->buffer == NULL ||
           !bioUserData->bufferIsSSLWriteSink) ? 0 : tcn_flush_sslbuffer_to_bytebuffer(bioUserData);
}

// Write up to wlen bytes of application data to the ssl BIO (encrypt)
TCN_IMPLEMENT_CALL(jint /* status */, SSL, writeToSSL)(TCN_STDARGS,
                                                       jlong ssl /* SSL * */,
                                                       jlong wbuf /* char * */,
                                                       jint wlen /* sizeof(wbuf) */) {
    SSL *ssl_ = J2P(ssl, SSL *);
    void *w = J2P(wbuf, void *);

    if (ssl_ == NULL) {
        tcn_ThrowException(e, "ssl is null");
        return 0;
    }
    if (w == NULL) {
        tcn_ThrowException(e, "wbuf is null");
        return 0;
    }

    UNREFERENCED_STDARGS;

    return SSL_write(ssl_, w, wlen);
}

// Read up to rlen bytes of application data from the given SSL BIO (decrypt)
TCN_IMPLEMENT_CALL(jint /* status */, SSL, readFromSSL)(TCN_STDARGS,
                                                        jlong ssl /* SSL * */,
                                                        jlong rbuf /* char * */,
                                                        jint rlen /* sizeof(rbuf) - 1 */) {
    SSL *ssl_ = J2P(ssl, SSL *);
    void *r = J2P(rbuf, void *);

    if (ssl_ == NULL) {
        tcn_ThrowException(e, "ssl is null");
        return 0;
    }
    if (r == NULL) {
        tcn_ThrowException(e, "rbuf is null");
        return 0;
    }

    UNREFERENCED_STDARGS;

    return SSL_read(ssl_, r, rlen);
}

// Get the shutdown status of the engine
TCN_IMPLEMENT_CALL(jint /* status */, SSL, getShutdown)(TCN_STDARGS,
                                                        jlong ssl /* SSL * */) {
    SSL *ssl_ = J2P(ssl, SSL *);

    if (ssl_ == NULL) {
        tcn_ThrowException(e, "ssl is null");
        return 0;
    }

    UNREFERENCED_STDARGS;

    return SSL_get_shutdown(ssl_);
}

// Called when the peer closes the connection
TCN_IMPLEMENT_CALL(void, SSL, setShutdown)(TCN_STDARGS,
                                           jlong ssl /* SSL * */,
                                           jint mode) {
    SSL *ssl_ = J2P(ssl, SSL *);

    if (ssl_ == NULL) {
        tcn_ThrowException(e, "ssl is null");
        return;
    }

    UNREFERENCED_STDARGS;

    SSL_set_shutdown(ssl_, mode);
}

// Free the SSL * and its associated internal BIO
TCN_IMPLEMENT_CALL(void, SSL, freeSSL)(TCN_STDARGS,
                                       jlong ssl /* SSL * */) {
    int *handshakeCount = NULL;
    tcn_ssl_ctxt_t* c = NULL;
    tcn_ssl_verify_config_t* verify_config = NULL;
    SSL *ssl_ = J2P(ssl, SSL *);
    if (ssl_ == NULL) {
        tcn_ThrowException(e, "ssl is null");
        return;
    }
    c = SSL_get_app_data2(ssl_);
    handshakeCount = SSL_get_app_data3(ssl_);
    verify_config = SSL_get_app_data4(ssl_);

    UNREFERENCED_STDARGS;
    TCN_ASSERT(c != NULL);

    if (handshakeCount != NULL) {
        OPENSSL_free(handshakeCount);
        SSL_set_app_data3(ssl_, NULL);
    }

    // Only free the verify_config if it is not shared with the SSLContext.
    if (verify_config != NULL && verify_config != &c->verify_config) {
        OPENSSL_free(verify_config);
        SSL_set_app_data4(ssl_, &c->verify_config);
    }
    SSL_free(ssl_);
}

TCN_IMPLEMENT_CALL(jlong, SSL, bioNewByteBuffer)(TCN_STDARGS,
                                                 jlong ssl /* SSL* */,
                                                 jint nonApplicationBufferSize) {
    SSL* ssl_ = J2P(ssl, SSL*);
    BIO* bio;
    struct TCN_bio_bytebuffer* bioUserData;

    if (ssl_ == NULL) {
        tcn_ThrowException(e, "ssl is null");
        return 0;
    }

    if (nonApplicationBufferSize <= 0) {
        tcn_ThrowException(e, "nonApplicationBufferSize <= 0");
        return 0;
    }

    bio = BIO_new(BIO_java_bytebuffer());
    if (bio == NULL) {
        tcn_ThrowException(e, "BIO_new failed");
        return 0;
    }

    bioUserData = BIO_get_data(bio);
    if (bioUserData == NULL) {
        BIO_free(bio);
        tcn_ThrowException(e, "BIO_get_data failed");
        return 0;
    }

    bioUserData->nonApplicationBuffer = (char*) OPENSSL_malloc(nonApplicationBufferSize * sizeof(char));
    if (bioUserData->nonApplicationBuffer == NULL) {
        BIO_free(bio);
        tcn_Throw(e, "Failed to allocate internal buffer of size %d", nonApplicationBufferSize);
        return 0;
    }
    bioUserData->nonApplicationBufferSize = nonApplicationBufferSize;

    SSL_set_bio(ssl_, bio, bio);

    return P2J(bio);
}

// Free a BIO * (typically, the network BIO)
TCN_IMPLEMENT_CALL(void, SSL, freeBIO)(TCN_STDARGS,
                                       jlong bio /* BIO * */) {
    BIO *bio_ = J2P(bio, BIO *);

    UNREFERENCED_STDARGS;

    if (bio_ != NULL) {
        BIO_free(bio_);
    }
}

// Send CLOSE_NOTIFY to peer
TCN_IMPLEMENT_CALL(jint /* status */, SSL, shutdownSSL)(TCN_STDARGS,
                                                        jlong ssl /* SSL * */) {
    SSL *ssl_ = J2P(ssl, SSL *);

    if (ssl_ == NULL) {
        tcn_ThrowException(e, "ssl is null");
        return 0;
    }

    UNREFERENCED_STDARGS;

    return SSL_shutdown(ssl_);
}

// Read which cipher was negotiated for the given SSL *.
TCN_IMPLEMENT_CALL(jstring, SSL, getCipherForSSL)(TCN_STDARGS,
                                                  jlong ssl /* SSL * */)
{
    SSL *ssl_ = J2P(ssl, SSL *);

    if (ssl_ == NULL) {
        tcn_ThrowException(e, "ssl is null");
        return NULL;
    }

    UNREFERENCED_STDARGS;

    return AJP_TO_JSTRING(SSL_get_cipher(ssl_));
}

// Read which protocol was negotiated for the given SSL *.
TCN_IMPLEMENT_CALL(jstring, SSL, getVersion)(TCN_STDARGS,
                                                  jlong ssl /* SSL * */)
{
    SSL *ssl_ = J2P(ssl, SSL *);

    if (ssl_ == NULL) {
        tcn_ThrowException(e, "ssl is null");
        return NULL;
    }

    UNREFERENCED_STDARGS;

    return AJP_TO_JSTRING(SSL_get_version(ssl_));
}

// Is the handshake over yet?
TCN_IMPLEMENT_CALL(jint, SSL, isInInit)(TCN_STDARGS,
                                        jlong ssl /* SSL * */) {
    SSL *ssl_ = J2P(ssl, SSL *);

    if (ssl_ == NULL) {
        tcn_ThrowException(e, "ssl is null");
        return 0;
    }

    UNREFERENCED(o);

    return SSL_in_init(ssl_);
}

TCN_IMPLEMENT_CALL(jint, SSL, doHandshake)(TCN_STDARGS,
                                           jlong ssl /* SSL * */) {
    SSL *ssl_ = J2P(ssl, SSL *);

    if (ssl_ == NULL) {
        tcn_ThrowException(e, "ssl is null");
        return 0;
    }

    UNREFERENCED(o);

    return SSL_do_handshake(ssl_);
}

// Read which protocol was negotiated for the given SSL *.
TCN_IMPLEMENT_CALL(jstring, SSL, getNextProtoNegotiated)(TCN_STDARGS,
                                                         jlong ssl /* SSL * */) {
    SSL *ssl_ = J2P(ssl, SSL *);
    const unsigned char *proto;
    unsigned int proto_len;

    if (ssl_ == NULL) {
        tcn_ThrowException(e, "ssl is null");
        return NULL;
    }

    UNREFERENCED(o);

    SSL_get0_next_proto_negotiated(ssl_, &proto, &proto_len);
    return tcn_new_stringn(e, (char*) proto, proto_len);
}

/*** End Twitter API Additions ***/

/*** Apple API Additions ***/

TCN_IMPLEMENT_CALL(jstring, SSL, getAlpnSelected)(TCN_STDARGS,
                                                         jlong ssl /* SSL * */) {
    // Use weak linking with GCC as this will alow us to run the same packaged version with multiple
    // version of openssl.
    #if defined(__GNUC__) || defined(__GNUG__)
        if (!SSL_get0_alpn_selected) {
            UNREFERENCED(o);
            UNREFERENCED(ssl);
            return NULL;
        }
    #endif

    // We can only support it when either use openssl version >= 1.0.2 or GCC as this way we can use weak linking
    #if OPENSSL_VERSION_NUMBER >= 0x10002000L || defined(__GNUC__) || defined(__GNUG__)
        SSL *ssl_ = J2P(ssl, SSL *);
        const unsigned char *proto;
        unsigned int proto_len;

        if (ssl_ == NULL) {
            tcn_ThrowException(e, "ssl is null");
            return NULL;
        }

        UNREFERENCED(o);

        SSL_get0_alpn_selected(ssl_, &proto, &proto_len);
        return tcn_new_stringn(e, (char*) proto, proto_len);
    #else
        UNREFERENCED(o);
        UNREFERENCED(ssl);
        return NULL;
    #endif
}

TCN_IMPLEMENT_CALL(jobjectArray, SSL, getPeerCertChain)(TCN_STDARGS,
                                                  jlong ssl /* SSL * */)
{
    STACK_OF(X509) *sk;
    int len;
    int i;
    X509 *cert;
    int length;
    unsigned char *buf;
    jobjectArray array;
    jbyteArray bArray;
    jclass byteArrayClass = tcn_get_byte_array_class();

    SSL *ssl_ = J2P(ssl, SSL *);

    if (ssl_ == NULL) {
        tcn_ThrowException(e, "ssl is null");
        return NULL;
    }

    UNREFERENCED(o);

    // Get a stack of all certs in the chain.
    sk = SSL_get_peer_cert_chain(ssl_);

    len = sk_X509_num(sk);
    if (len <= 0) {
        // No peer certificate chain as no auth took place yet, or the auth was not successful.
        return NULL;
    }
    // Create the byte[][] array that holds all the certs
    array = (*e)->NewObjectArray(e, len, byteArrayClass, NULL);

    for(i = 0; i < len; i++) {
        cert = sk_X509_value(sk, i);

        buf = NULL;
        length = i2d_X509(cert, &buf);
        if (length < 0) {
            if (buf != NULL) {
                OPENSSL_free(buf);
            }
            // In case of error just return an empty byte[][]
            return (*e)->NewObjectArray(e, 0, byteArrayClass, NULL);
        }
        bArray = (*e)->NewByteArray(e, length);
        (*e)->SetByteArrayRegion(e, bArray, 0, length, (jbyte*) buf);
        (*e)->SetObjectArrayElement(e, array, i, bArray);

        // Delete the local reference as we not know how long the chain is and local references are otherwise
        // only freed once jni method returns.
        (*e)->DeleteLocalRef(e, bArray);

        OPENSSL_free(buf);
    }
    return array;
}

TCN_IMPLEMENT_CALL(jbyteArray, SSL, getPeerCertificate)(TCN_STDARGS,
                                                  jlong ssl /* SSL * */)
{
    X509 *cert;
    int length;
    unsigned char *buf = NULL;
    jbyteArray bArray;

    SSL *ssl_ = J2P(ssl, SSL *);

    if (ssl_ == NULL) {
        tcn_ThrowException(e, "ssl is null");
        return NULL;
    }

    UNREFERENCED(o);

    // Get a stack of all certs in the chain
    cert = SSL_get_peer_certificate(ssl_);
    if (cert == NULL) {
        return NULL;
    }

    length = i2d_X509(cert, &buf);

    bArray = (*e)->NewByteArray(e, length);
    (*e)->SetByteArrayRegion(e, bArray, 0, length, (jbyte*) buf);

    // We need to free the cert as the reference count is incremented by one and it is not destroyed when the
    // session is freed.
    // See https://www.openssl.org/docs/ssl/SSL_get_peer_certificate.html
    X509_free(cert);

    OPENSSL_free(buf);

    return bArray;
}

TCN_IMPLEMENT_CALL(jstring, SSL, getErrorString)(TCN_STDARGS, jlong number)
{
    char buf[ERR_LEN];
    UNREFERENCED(o);
    ERR_error_string(number, buf);
    return tcn_new_string(e, buf);
}

TCN_IMPLEMENT_CALL(jlong, SSL, getTime)(TCN_STDARGS, jlong ssl)
{
    SSL *ssl_ = J2P(ssl, SSL *);
    SSL_SESSION *session = NULL;

    if (ssl_ == NULL) {
        tcn_ThrowException(e, "ssl is null");
        return 0;
    }

    session = SSL_get_session(ssl_);
    if (session == NULL) {
        // BoringSSL does not protect against a NULL session. OpenSSL
        // returns 0 if the session is NULL, so do that here.
        return 0;
    }

    UNREFERENCED(o);

    return SSL_get_time(session);
}


TCN_IMPLEMENT_CALL(jlong, SSL, getTimeout)(TCN_STDARGS, jlong ssl)
{
    SSL *ssl_ = J2P(ssl, SSL *);
    SSL_SESSION *session = NULL;

    if (ssl_ == NULL) {
        tcn_ThrowException(e, "ssl is null");
        return 0;
    }

    session = SSL_get_session(ssl_);
    if (session == NULL) {
        // BoringSSL does not protect against a NULL session. OpenSSL
        // returns 0 if the session is NULL, so do that here.
        return 0;
    }

    UNREFERENCED(o);

    return SSL_get_timeout(session);
}


TCN_IMPLEMENT_CALL(jlong, SSL, setTimeout)(TCN_STDARGS, jlong ssl, jlong seconds)
{
    SSL *ssl_ = J2P(ssl, SSL *);
    SSL_SESSION *session = NULL;

    if (ssl_ == NULL) {
        tcn_ThrowException(e, "ssl is null");
        return 0;
    }

    session = SSL_get_session(ssl_);
    if (session == NULL) {
        // BoringSSL does not protect against a NULL session. OpenSSL
        // returns 0 if the session is NULL, so do that here.
        return 0;
    }

    UNREFERENCED(o);

    return SSL_set_timeout(session, seconds);
}


TCN_IMPLEMENT_CALL(void, SSL, setVerify)(TCN_STDARGS, jlong ssl, jint level, jint depth)
{
    tcn_ssl_verify_config_t* verify_config;
    tcn_ssl_ctxt_t* c;
    SSL *ssl_ = J2P(ssl, SSL *);

    if (ssl_ == NULL) {
        tcn_ThrowException(e, "ssl is null");
        return;
    }

    c = SSL_get_app_data2(ssl_);
    verify_config = SSL_get_app_data4(ssl_);

    UNREFERENCED(o);
    TCN_ASSERT(c != NULL);
    TCN_ASSERT(verify_config != NULL);

    // If we are sharing the configuration from the SSLContext we now need to create a new configuration just for this SSL.
    if (verify_config == &c->verify_config) {
       verify_config = (tcn_ssl_verify_config_t*) OPENSSL_malloc(sizeof(tcn_ssl_verify_config_t));
       if (verify_config == NULL) {
           tcn_ThrowException(e, "failed to allocate tcn_ssl_verify_config_t");
           return;
       }
       // Copy the verify depth form the context in case depth is <0.
       verify_config->verify_depth = c->verify_config.verify_depth;
       SSL_set_app_data4(ssl_, verify_config);
    }

    // No need to specify a callback for SSL_set_verify because we override the default certificate verification via SSL_CTX_set_cert_verify_callback.
    SSL_set_verify(ssl_, tcn_set_verify_config(verify_config, level, depth), NULL);
    SSL_set_verify_depth(ssl_, verify_config->verify_depth);
}

TCN_IMPLEMENT_CALL(void, SSL, setOptions)(TCN_STDARGS, jlong ssl,
                                                 jint opt)
{
    SSL *ssl_ = J2P(ssl, SSL *);

    if (ssl_ == NULL) {
        tcn_ThrowException(e, "ssl is null");
        return;
    }

    UNREFERENCED_STDARGS;

    SSL_set_options(ssl_, opt);
}

TCN_IMPLEMENT_CALL(void, SSL, clearOptions)(TCN_STDARGS, jlong ssl,
                                                 jint opt)
{
    SSL *ssl_ = J2P(ssl, SSL *);

    if (ssl_ == NULL) {
        tcn_ThrowException(e, "ssl is null");
        return;
    }

    UNREFERENCED_STDARGS;

    SSL_clear_options(ssl_, opt);
}

TCN_IMPLEMENT_CALL(jint, SSL, getOptions)(TCN_STDARGS, jlong ssl)
{
    SSL *ssl_ = J2P(ssl, SSL *);

    if (ssl_ == NULL) {
        tcn_ThrowException(e, "ssl is null");
        return 0;
    }

    UNREFERENCED_STDARGS;

    return SSL_get_options(ssl_);
}

TCN_IMPLEMENT_CALL(jobjectArray, SSL, getCiphers)(TCN_STDARGS, jlong ssl)
{
    STACK_OF(SSL_CIPHER) *sk;
    int len;
    jobjectArray array;
    const SSL_CIPHER *cipher;
    const char *name;
    int i;
    jstring c_name;
    SSL *ssl_ = J2P(ssl, SSL *);

    if (ssl_ == NULL) {
        tcn_ThrowException(e, "ssl is null");
        return NULL;
    }

    UNREFERENCED_STDARGS;

    sk = SSL_get_ciphers(ssl_);
    len = sk_SSL_CIPHER_num(sk);

    if (len <= 0) {
        // No peer certificate chain as no auth took place yet, or the auth was not successful.
        return NULL;
    }

    // Create the byte[][] array that holds all the certs
    array = (*e)->NewObjectArray(e, len, tcn_get_string_class(), NULL);

    for (i = 0; i < len; i++) {
        cipher = sk_SSL_CIPHER_value(sk, i);
        name = SSL_CIPHER_get_name(cipher);

        c_name = (*e)->NewStringUTF(e, name);
        (*e)->SetObjectArrayElement(e, array, i, c_name);
    }
    return array;
}

TCN_IMPLEMENT_CALL(jboolean, SSL, setCipherSuites)(TCN_STDARGS, jlong ssl,
                                                         jstring ciphers)
{
    jboolean rv = JNI_TRUE;
    TCN_ALLOC_CSTRING(ciphers);
    SSL *ssl_ = J2P(ssl, SSL *);

    if (ssl_ == NULL) {
        tcn_ThrowException(e, "ssl is null");
        return JNI_FALSE;
    }

    UNREFERENCED(o);

    if (!J2S(ciphers)) {
        return JNI_FALSE;
    }

    if (!SSL_set_cipher_list(ssl_, J2S(ciphers))) {
        char err[ERR_LEN];
        ERR_error_string(ERR_get_error(), err);
        tcn_Throw(e, "Unable to configure permitted SSL ciphers (%s)", err);
        rv = JNI_FALSE;
    }

    TCN_FREE_CSTRING(ciphers);
    return rv;
}

TCN_IMPLEMENT_CALL(jbyteArray, SSL, getSessionId)(TCN_STDARGS, jlong ssl)
{

    unsigned int len;
    const unsigned char *session_id;
    SSL_SESSION *session;
    jbyteArray bArray;
    SSL *ssl_ = J2P(ssl, SSL *);

    if (ssl_ == NULL) {
        tcn_ThrowException(e, "ssl is null");
        return NULL;
    }

    UNREFERENCED(o);

    session = SSL_get_session(ssl_);
    if (session == NULL) {
        return NULL;
    }

    session_id = SSL_SESSION_get_id(session, &len);
    if (len == 0 || session_id == NULL) {
        return NULL;
    }

    bArray = (*e)->NewByteArray(e, len);
    (*e)->SetByteArrayRegion(e, bArray, 0, len, (jbyte*) session_id);
    return bArray;
}

TCN_IMPLEMENT_CALL(jint, SSL, getHandshakeCount)(TCN_STDARGS, jlong ssl)
{
    int *handshakeCount = NULL;
    SSL *ssl_ = J2P(ssl, SSL *);

    if (ssl_ == NULL) {
        tcn_ThrowException(e, "ssl is null");
        return -1;
    }

    UNREFERENCED(o);

    handshakeCount = SSL_get_app_data3(ssl_);
    return handshakeCount != NULL ? *handshakeCount : 0;
}


TCN_IMPLEMENT_CALL(void, SSL, clearError)(TCN_STDARGS)
{
    UNREFERENCED(o);
    ERR_clear_error();
}

TCN_IMPLEMENT_CALL(jint, SSL, renegotiate)(TCN_STDARGS,
                                           jlong ssl /* SSL * */) {
    SSL *ssl_ = J2P(ssl, SSL *);

    if (ssl_ == NULL) {
        tcn_ThrowException(e, "ssl is null");
        return 0;
    }

    UNREFERENCED(o);

    return SSL_renegotiate(ssl_);
}

TCN_IMPLEMENT_CALL(void, SSL, setState)(TCN_STDARGS,
                                           jlong ssl, /* SSL * */
                                           jint state) {
    SSL *ssl_ = J2P(ssl, SSL *);

    if (ssl_ == NULL) {
        tcn_ThrowException(e, "ssl is null");
        return;
    }

    UNREFERENCED(o);

    SSL_set_state(ssl_, state);
}

TCN_IMPLEMENT_CALL(void, SSL, setTlsExtHostName)(TCN_STDARGS, jlong ssl, jstring hostname) {
    TCN_ALLOC_CSTRING(hostname);
    SSL *ssl_ = J2P(ssl, SSL *);

    if (ssl_ == NULL) {
        tcn_ThrowException(e, "ssl is null");
    } else {
        UNREFERENCED(o);

        if (SSL_set_tlsext_host_name(ssl_, J2S(hostname)) != 1) {
            char err[ERR_LEN];
            ERR_error_string(ERR_get_error(), err);
            tcn_Throw(e, "Unable to set TLS servername extension (%s)", err);
        }
    }

    TCN_FREE_CSTRING(hostname);
}

TCN_IMPLEMENT_CALL(void, SSL, setHostNameValidation)(TCN_STDARGS, jlong sslAddress, jint flags, jstring hostnameString) {
    SSL* ssl = J2P(sslAddress, SSL*);

    if (ssl == NULL) {
        tcn_ThrowException(e, "ssl is null");
    } else {
        const char* hostname = hostnameString == NULL ? NULL : (*e)->GetStringUTFChars(e, hostnameString, 0);
#if OPENSSL_VERSION_NUMBER >= 0x10002000L && !defined(LIBRESSL_VERSION_NUMBER)
        X509_VERIFY_PARAM* param = SSL_get0_param(ssl);
        X509_VERIFY_PARAM_set_hostflags(param, flags);
        if (X509_VERIFY_PARAM_set1_host(param, hostname, 0) != 1) {
            char err[ERR_LEN];
            ERR_error_string(ERR_get_error(), err);
            tcn_Throw(e, "X509_VERIFY_PARAM_set1_host error (%s)", err);
        }
#else
        if (hostname != NULL && hostname[0] != '\0') {
            tcn_ThrowException(e, "hostname verification requires OpenSSL 1.0.2+");
        }
#endif
        (*e)->ReleaseStringUTFChars(e, hostnameString, hostname);
    }
}

TCN_IMPLEMENT_CALL(jobjectArray, SSL, authenticationMethods)(TCN_STDARGS, jlong ssl) {
    SSL *ssl_ = J2P(ssl, SSL *);
    const STACK_OF(SSL_CIPHER) *ciphers = NULL;
    int len;
    int i;
    jobjectArray array;

    TCN_ASSERT(ssl_ != NULL);

    UNREFERENCED(o);

    ciphers = SSL_get_ciphers(ssl_);
    len = sk_SSL_CIPHER_num(ciphers);

    array = (*e)->NewObjectArray(e, len, tcn_get_string_class(), NULL);

    for (i = 0; i < len; i++) {
        (*e)->SetObjectArrayElement(e, array, i,
        (*e)->NewStringUTF(e, SSL_cipher_authentication_method((SSL_CIPHER*) sk_value((_STACK*) ciphers, i))));
    }
    return array;
}

TCN_IMPLEMENT_CALL(void, SSL, setCertificateBio)(TCN_STDARGS, jlong ssl,
                                                         jlong cert, jlong key,
                                                         jstring password)
{
    SSL *ssl_ = J2P(ssl, SSL *);
    BIO *cert_bio = J2P(cert, BIO *);
    BIO *key_bio = J2P(key, BIO *);
    EVP_PKEY* pkey = NULL;
    X509* xcert = NULL;
    TCN_ALLOC_CSTRING(password);
    char err[ERR_LEN];

    UNREFERENCED(o);
    TCN_ASSERT(ssl != NULL);

    if (key <= 0) {
        key = cert;
    }

    if (cert <= 0 || key <= 0) {
        tcn_Throw(e, "No Certificate file specified or invalid file format");
        goto cleanup;
    }

    if ((pkey = load_pem_key_bio(cpassword, key_bio)) == NULL) {
        ERR_error_string_n(ERR_get_error(), err, ERR_LEN);
        ERR_clear_error();
        tcn_Throw(e, "Unable to load certificate key (%s)",err);
        goto cleanup;
    }
    if ((xcert = load_pem_cert_bio(cpassword, cert_bio)) == NULL) {
        ERR_error_string_n(ERR_get_error(), err, ERR_LEN);
        ERR_clear_error();
        tcn_Throw(e, "Unable to load certificate (%s) ", err);
        goto cleanup;
    }

    if (SSL_use_certificate(ssl_, xcert) <= 0) {
        ERR_error_string_n(ERR_get_error(), err, ERR_LEN);
        ERR_clear_error();
        tcn_Throw(e, "Error setting certificate (%s)", err);
        goto cleanup;
    }
    if (SSL_use_PrivateKey(ssl_, pkey) <= 0) {
        ERR_error_string_n(ERR_get_error(), err, ERR_LEN);
        ERR_clear_error();
        tcn_Throw(e, "Error setting private key (%s)", err);
        goto cleanup;
    }
    if (SSL_check_private_key(ssl_) <= 0) {
        ERR_error_string_n(ERR_get_error(), err, ERR_LEN);
        ERR_clear_error();

        tcn_Throw(e, "Private key does not match the certificate public key (%s)",
                  err);
        goto cleanup;
    }
cleanup:
    TCN_FREE_CSTRING(password);
    EVP_PKEY_free(pkey); // this function is safe to call with NULL
    X509_free(xcert); // this function is safe to call with NULL
}

TCN_IMPLEMENT_CALL(void, SSL, setCertificateChainBio)(TCN_STDARGS, jlong ssl,
                                                                  jlong chain,
                                                                  jboolean skipfirst)
{
    SSL *ssl_ = J2P(ssl, SSL *);
    BIO *b = J2P(chain, BIO *);
    char err[ERR_LEN];

    UNREFERENCED(o);
    TCN_ASSERT(ssl_ != NULL);
    TCN_ASSERT(b != NULL);

    if (SSL_use_certificate_chain_bio(ssl_, b, skipfirst) < 0)  {
        ERR_error_string_n(ERR_get_error(), err, ERR_LEN);
        ERR_clear_error();
        tcn_Throw(e, "Error setting certificate chain (%s)", err);
    }
}

TCN_IMPLEMENT_CALL(long, SSL, parsePrivateKey)(TCN_STDARGS, jlong privateKeyBio, jstring password)
{
    EVP_PKEY* pkey = NULL;
    BIO *bio = J2P(privateKeyBio, BIO *);
    TCN_ALLOC_CSTRING(password);
    char err[ERR_LEN];

    UNREFERENCED(o);

    if (bio == NULL) {
        tcn_Throw(e, "Unable to load certificate key");
        goto cleanup;
    }

    if ((pkey = load_pem_key_bio(cpassword, bio)) == NULL) {
        ERR_error_string_n(ERR_get_error(), err, ERR_LEN);
        ERR_clear_error();
        tcn_Throw(e, "Unable to load certificate key (%s)",err);
        goto cleanup;
    }

cleanup:
    TCN_FREE_CSTRING(password);
    return P2J(pkey);
}

TCN_IMPLEMENT_CALL(void, SSL, freePrivateKey)(TCN_STDARGS, jlong privateKey)
{
    EVP_PKEY *key = J2P(privateKey, EVP_PKEY *);
    UNREFERENCED(o);
    EVP_PKEY_free(key); // Safe to call with NULL as well.
}

TCN_IMPLEMENT_CALL(long, SSL, parseX509Chain)(TCN_STDARGS, jlong x509ChainBio)
{
    BIO *cert_bio = J2P(x509ChainBio, BIO *);
    X509* cert = NULL;
    STACK_OF(X509) *chain = NULL;
    char err[ERR_LEN];
    unsigned long error;
    int n = 0;

    UNREFERENCED(o);

    if (cert_bio == NULL) {
        tcn_Throw(e, "No Certificate specified or invalid format");
        goto cleanup;
    }

    chain = sk_X509_new_null();
    while ((cert = PEM_read_bio_X509(cert_bio, NULL, NULL, NULL)) != NULL) {
        if (sk_X509_push(chain, cert) <= 0) {
            tcn_Throw(e, "No Certificate specified or invalid format");
            goto cleanup;
        }
        cert = NULL;
        n++;
    }

    // ensure that if we have an error its just for EOL.
    if ((error = ERR_peek_error()) > 0) {
        if (!(ERR_GET_LIB(error) == ERR_LIB_PEM
              && ERR_GET_REASON(error) == PEM_R_NO_START_LINE)) {

            ERR_error_string_n(ERR_get_error(), err, ERR_LEN);
            tcn_Throw(e, "Invalid format (%s)", err);
            goto cleanup;
        }
        ERR_clear_error();
    }

    return P2J(chain);

cleanup:
    ERR_clear_error();
    sk_X509_pop_free(chain, X509_free);
    X509_free(cert);
    return 0;
}

TCN_IMPLEMENT_CALL(void, SSL, freeX509Chain)(TCN_STDARGS, jlong x509Chain)
{
    STACK_OF(X509) *chain = J2P(x509Chain, STACK_OF(X509) *);
    UNREFERENCED(o);
    sk_X509_pop_free(chain, X509_free);
}

