/*
 * Copyright (C) 2014 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/*
 * This file defines an NDK API.
 * Do not remove methods.
 * Do not change method signatures.
 * Do not change the value of constants.
 * Do not change the size of any of the classes defined in here.
 * Do not reference types that are not part of the NDK.
 * Do not #include files that aren't part of the NDK.
 */

#ifndef _NDK_MEDIA_CODEC_H
#define _NDK_MEDIA_CODEC_H

#include <stdint.h>
#include <sys/cdefs.h>

#include "NdkMediaCrypto.h"
#include "NdkMediaError.h"
#include "NdkMediaFormat.h"

__BEGIN_DECLS

struct ANativeWindow;
typedef struct ANativeWindow ANativeWindow;


struct AMediaCodec;
typedef struct AMediaCodec AMediaCodec;

struct AMediaCodecBufferInfo {
    int32_t offset;
    int32_t size;
    int64_t presentationTimeUs;
    uint32_t flags;
};
typedef struct AMediaCodecBufferInfo AMediaCodecBufferInfo;
typedef struct AMediaCodecCryptoInfo AMediaCodecCryptoInfo;

enum {
    AMEDIACODEC_BUFFER_FLAG_END_OF_STREAM = 4,
    AMEDIACODEC_CONFIGURE_FLAG_ENCODE = 1,
    AMEDIACODEC_INFO_OUTPUT_BUFFERS_CHANGED = -3,
    AMEDIACODEC_INFO_OUTPUT_FORMAT_CHANGED = -2,
    AMEDIACODEC_INFO_TRY_AGAIN_LATER = -1
};

#if __ANDROID_API__ >= 21

/**
 * Create codec by name. Use this if you know the exact codec you want to use.
 * When configuring, you will need to specify whether to use the codec as an
 * encoder or decoder.
 */
AMediaCodec* AMediaCodec_createCodecByName(const char *name);

/**
 * Create codec by mime type. Most applications will use this, specifying a
 * mime type obtained from media extractor.
 */
AMediaCodec* AMediaCodec_createDecoderByType(const char *mime_type);

/**
 * Create encoder by name.
 */
AMediaCodec* AMediaCodec_createEncoderByType(const char *mime_type);

/**
 * delete the codec and free its resources
 */
media_status_t AMediaCodec_delete(AMediaCodec*);

/**
 * Configure the codec. For decoding you would typically get the format from an extractor.
 */
media_status_t AMediaCodec_configure(
        AMediaCodec*,
        const AMediaFormat* format,
        ANativeWindow* surface,
        AMediaCrypto *crypto,
        uint32_t flags);

/**
 * Start the codec. A codec must be configured before it can be started, and must be started
 * before buffers can be sent to it.
 */
media_status_t AMediaCodec_start(AMediaCodec*);

/**
 * Stop the codec.
 */
media_status_t AMediaCodec_stop(AMediaCodec*);

/*
 * Flush the codec's input and output. All indices previously returned from calls to
 * AMediaCodec_dequeueInputBuffer and AMediaCodec_dequeueOutputBuffer become invalid.
 */
media_status_t AMediaCodec_flush(AMediaCodec*);

/**
 * Get an input buffer. The specified buffer index must have been previously obtained from
 * dequeueInputBuffer, and not yet queued.
 */
uint8_t* AMediaCodec_getInputBuffer(AMediaCodec*, size_t idx, size_t *out_size);

/**
 * Get an output buffer. The specified buffer index must have been previously obtained from
 * dequeueOutputBuffer, and not yet queued.
 */
uint8_t* AMediaCodec_getOutputBuffer(AMediaCodec*, size_t idx, size_t *out_size);

/**
 * Get the index of the next available input buffer. An app will typically use this with
 * getInputBuffer() to get a pointer to the buffer, then copy the data to be encoded or decoded
 * into the buffer before passing it to the codec.
 */
ssize_t AMediaCodec_dequeueInputBuffer(AMediaCodec*, int64_t timeoutUs);

/*
 * __USE_FILE_OFFSET64 changes the type of off_t in LP32, which changes the ABI
 * of these declarations to  not match the platform. In that case, define these
 * APIs in terms of int32_t instead. Passing an off_t in this situation will
 * result in silent truncation unless the user builds with -Wconversion, but the
 * only alternative it to not expose them at all for this configuration, which
 * makes the whole API unusable.
 *
 * https://github.com/android-ndk/ndk/issues/459
 */
#if defined(__USE_FILE_OFFSET64) && !defined(__LP64__)
#define _off_t_compat int32_t
#else
#define _off_t_compat off_t
#endif  /* defined(__USE_FILE_OFFSET64) && !defined(__LP64__) */

#if (defined(__cplusplus) && __cplusplus >= 201103L) || \
    __STDC_VERSION__ >= 201112L
#include <assert.h>
static_assert(sizeof(_off_t_compat) == sizeof(long),
              "_off_t_compat does not match the NDK ABI. See "
              "https://github.com/android-ndk/ndk/issues/459.");
#endif

/**
 * Send the specified buffer to the codec for processing.
 */
media_status_t AMediaCodec_queueInputBuffer(AMediaCodec*, size_t idx,
                                            _off_t_compat offset, size_t size,
                                            uint64_t time, uint32_t flags);

/**
 * Send the specified buffer to the codec for processing.
 */
media_status_t AMediaCodec_queueSecureInputBuffer(AMediaCodec*, size_t idx,
                                                  _off_t_compat offset,
                                                  AMediaCodecCryptoInfo*,
                                                  uint64_t time, uint32_t flags);

#undef _off_t_compat

/**
 * Get the index of the next available buffer of processed data.
 */
ssize_t AMediaCodec_dequeueOutputBuffer(AMediaCodec*, AMediaCodecBufferInfo *info,
        int64_t timeoutUs);
AMediaFormat* AMediaCodec_getOutputFormat(AMediaCodec*);

/**
 * If you are done with a buffer, use this call to return the buffer to
 * the codec. If you previously specified a surface when configuring this
 * video decoder you can optionally render the buffer.
 */
media_status_t AMediaCodec_releaseOutputBuffer(AMediaCodec*, size_t idx, bool render);

/**
 * Dynamically sets the output surface of a codec.
 *
 *  This can only be used if the codec was configured with an output surface.  The
 *  new output surface should have a compatible usage type to the original output surface.
 *  E.g. codecs may not support switching from a SurfaceTexture (GPU readable) output
 *  to ImageReader (software readable) output.
 *
 * For more details, see the Java documentation for MediaCodec.setOutputSurface.
 */
media_status_t AMediaCodec_setOutputSurface(AMediaCodec*, ANativeWindow* surface);

/**
 * If you are done with a buffer, use this call to update its surface timestamp
 * and return it to the codec to render it on the output surface. If you
 * have not specified an output surface when configuring this video codec,
 * this call will simply return the buffer to the codec.
 *
 * For more details, see the Java documentation for MediaCodec.releaseOutputBuffer.
 */
media_status_t AMediaCodec_releaseOutputBufferAtTime(
        AMediaCodec *mData, size_t idx, int64_t timestampNs);

#if __ANDROID_API__ >= 26

/**
 * Creates a Surface that can be used as the input to encoder, in place of input buffers
 *
 * This can only be called after the codec has been configured via
 * AMediaCodec_configure(..); and before AMediaCodec_start() has been called.
 *
 * The application is responsible for releasing the surface by calling
 * ANativeWindow_release() when done.
 *
 * For more details, see the Java documentation for MediaCodec.createInputSurface.
 */
media_status_t AMediaCodec_createInputSurface(
        AMediaCodec *mData, ANativeWindow **surface);

/**
 * Creates a persistent Surface that can be used as the input to encoder
 *
 * Persistent surface can be reused by MediaCodec instances and can be set
 * on a new instance via AMediaCodec_setInputSurface().
 * A persistent surface can be connected to at most one instance of MediaCodec
 * at any point in time.
 *
 * The application is responsible for releasing the surface by calling
 * ANativeWindow_release() when done.
 *
 * For more details, see the Java documentation for MediaCodec.createPersistentInputSurface.
 */
media_status_t AMediaCodec_createPersistentInputSurface(
        ANativeWindow **surface);

/**
 * Set a persistent-surface that can be used as the input to encoder, in place of input buffers
 *
 * The surface provided *must* be a persistent surface created via
 * AMediaCodec_createPersistentInputSurface()
 * This can only be called after the codec has been configured by calling
 * AMediaCodec_configure(..); and before AMediaCodec_start() has been called.
 *
 * For more details, see the Java documentation for MediaCodec.setInputSurface.
 */
media_status_t AMediaCodec_setInputSurface(
        AMediaCodec *mData, ANativeWindow *surface);

/**
 * Signal additional parameters to the codec instance.
 *
 * Parameters can be communicated only when the codec is running, i.e
 * after AMediaCodec_start() has been called.
 *
 * NOTE: Some of these parameter changes may silently fail to apply.
 */
media_status_t AMediaCodec_setParameters(
        AMediaCodec *mData, const AMediaFormat* params);

/**
 * Signals end-of-stream on input. Equivalent to submitting an empty buffer with
 * AMEDIACODEC_BUFFER_FLAG_END_OF_STREAM set.
 *
 * Returns AMEDIA_ERROR_INVALID_OPERATION when used with an encoder not in executing state
 * or not receiving input from a Surface created by AMediaCodec_createInputSurface or
 * AMediaCodec_createPersistentInputSurface.
 *
 * Returns the previous codec error if one exists.
 *
 * Returns AMEDIA_OK when completed succesfully.
 *
 * For more details, see the Java documentation for MediaCodec.signalEndOfInputStream.
 */
media_status_t AMediaCodec_signalEndOfInputStream(AMediaCodec *mData);

#endif /* __ANDROID_API__ >= 26 */

typedef enum {
    AMEDIACODECRYPTOINFO_MODE_CLEAR = 0,
    AMEDIACODECRYPTOINFO_MODE_AES_CTR = 1,
    AMEDIACODECRYPTOINFO_MODE_AES_WV = 2,
    AMEDIACODECRYPTOINFO_MODE_AES_CBC = 3
} cryptoinfo_mode_t;

typedef struct {
    int32_t encryptBlocks;
    int32_t skipBlocks;
} cryptoinfo_pattern_t;

/**
 * Create an AMediaCodecCryptoInfo from scratch. Use this if you need to use custom
 * crypto info, rather than one obtained from AMediaExtractor.
 *
 * AMediaCodecCryptoInfo describes the structure of an (at least
 * partially) encrypted input sample.
 * A buffer's data is considered to be partitioned into "subsamples",
 * each subsample starts with a (potentially empty) run of plain,
 * unencrypted bytes followed by a (also potentially empty) run of
 * encrypted bytes.
 * numBytesOfClearData can be null to indicate that all data is encrypted.
 * This information encapsulates per-sample metadata as outlined in
 * ISO/IEC FDIS 23001-7:2011 "Common encryption in ISO base media file format files".
 */
AMediaCodecCryptoInfo *AMediaCodecCryptoInfo_new(
        int numsubsamples,
        uint8_t key[16],
        uint8_t iv[16],
        cryptoinfo_mode_t mode,
        size_t *clearbytes,
        size_t *encryptedbytes);

/**
 * delete an AMediaCodecCryptoInfo created previously with AMediaCodecCryptoInfo_new, or
 * obtained from AMediaExtractor
 */
media_status_t AMediaCodecCryptoInfo_delete(AMediaCodecCryptoInfo*);

/**
 * Set the crypto pattern on an AMediaCryptoInfo object
 */
void AMediaCodecCryptoInfo_setPattern(
        AMediaCodecCryptoInfo *info,
        cryptoinfo_pattern_t *pattern);

/**
 * The number of subsamples that make up the buffer's contents.
 */
size_t AMediaCodecCryptoInfo_getNumSubSamples(AMediaCodecCryptoInfo*);

/**
 * A 16-byte opaque key
 */
media_status_t AMediaCodecCryptoInfo_getKey(AMediaCodecCryptoInfo*, uint8_t *dst);

/**
 * A 16-byte initialization vector
 */
media_status_t AMediaCodecCryptoInfo_getIV(AMediaCodecCryptoInfo*, uint8_t *dst);

/**
 * The type of encryption that has been applied,
 * one of AMEDIACODECRYPTOINFO_MODE_CLEAR or AMEDIACODECRYPTOINFO_MODE_AES_CTR.
 */
cryptoinfo_mode_t AMediaCodecCryptoInfo_getMode(AMediaCodecCryptoInfo*);

/**
 * The number of leading unencrypted bytes in each subsample.
 */
media_status_t AMediaCodecCryptoInfo_getClearBytes(AMediaCodecCryptoInfo*, size_t *dst);

/**
 * The number of trailing encrypted bytes in each subsample.
 */
media_status_t AMediaCodecCryptoInfo_getEncryptedBytes(AMediaCodecCryptoInfo*, size_t *dst);

#endif /* __ANDROID_API__ >= 21 */

__END_DECLS

#endif //_NDK_MEDIA_CODEC_H
