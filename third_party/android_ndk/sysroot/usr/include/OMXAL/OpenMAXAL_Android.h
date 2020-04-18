/*
 * Copyright (C) 2010 The Android Open Source Project
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

#ifndef OPENMAX_AL_ANDROID_H_
#define OPENMAX_AL_ANDROID_H_

#include "OpenMAXAL.h"

#ifdef __cplusplus
extern "C" {
#endif

/*---------------------------------------------------------------------------*/
/* Android common types                                                      */
/*---------------------------------------------------------------------------*/

typedef xa_int64_t             XAAint64;          /* 64 bit signed integer   */

typedef xa_uint64_t            XAAuint64;         /* 64 bit unsigned integer */

/*---------------------------------------------------------------------------*/
/* Android common types                                                      */
/*---------------------------------------------------------------------------*/

#define XA_ANDROID_VIDEOCODEC_VP8            ((XAuint32) 0x00000006)

#define XA_ANDROID_VIDEOPROFILE_VP8_MAIN     ((XAuint32) 0x00000001)

#define XA_ANDROID_VIDEOLEVEL_VP8_VERSION0   ((XAuint32) 0x00000001)
#define XA_ANDROID_VIDEOLEVEL_VP8_VERSION1   ((XAuint32) 0x00000002)
#define XA_ANDROID_VIDEOLEVEL_VP8_VERSION2   ((XAuint32) 0x00000003)
#define XA_ANDROID_VIDEOLEVEL_VP8_VERSION3   ((XAuint32) 0x00000004)

/*---------------------------------------------------------------------------*/
/* Android Buffer Queue Interface                                            */
/*---------------------------------------------------------------------------*/

extern XA_API const XAInterfaceID XA_IID_ANDROIDBUFFERQUEUESOURCE;

struct XAAndroidBufferQueueItf_;
typedef const struct XAAndroidBufferQueueItf_ * const * XAAndroidBufferQueueItf;

#define XA_ANDROID_ITEMKEY_NONE             ((XAuint32) 0x00000000)
#define XA_ANDROID_ITEMKEY_EOS              ((XAuint32) 0x00000001)
#define XA_ANDROID_ITEMKEY_DISCONTINUITY    ((XAuint32) 0x00000002)
#define XA_ANDROID_ITEMKEY_BUFFERQUEUEEVENT ((XAuint32) 0x00000003)
#define XA_ANDROID_ITEMKEY_FORMAT_CHANGE    ((XAuint32) 0x00000004)

// optional data for XA_ANDROID_ITEMKEY_FORMAT_CHANGE, used when only one stream changes format,
//   and the others remain continuous (i.e. no temporal discontinuity is introduced for them)
//   candidate for being exposed in NDK
#define XA_ANDROID_FORMATCHANGE_ITEMDATA_VIDEO  ((XAuint32) 0x00000001)
//   not supported at this stage, for illustration purposes only
//#define XA_ANDROID_FORMATCHANGE_ITEMDATA_AUDIO ((XAuint32) 0x00000002)

#define XA_ANDROIDBUFFERQUEUEEVENT_NONE        ((XAuint32) 0x00000000)
#define XA_ANDROIDBUFFERQUEUEEVENT_PROCESSED   ((XAuint32) 0x00000001)
#if 0   // reserved for future use
#define XA_ANDROIDBUFFERQUEUEEVENT_UNREALIZED  ((XAuint32) 0x00000002)
#define XA_ANDROIDBUFFERQUEUEEVENT_CLEARED     ((XAuint32) 0x00000004)
#define XA_ANDROIDBUFFERQUEUEEVENT_STOPPED     ((XAuint32) 0x00000008)
#define XA_ANDROIDBUFFERQUEUEEVENT_ERROR       ((XAuint32) 0x00000010)
#define XA_ANDROIDBUFFERQUEUEEVENT_CONTENT_END ((XAuint32) 0x00000020)
#endif

typedef struct XAAndroidBufferItem_ {
    XAuint32 itemKey;  // identifies the item
    XAuint32 itemSize;
    XAuint8  itemData[0];
} XAAndroidBufferItem;

typedef XAresult (XAAPIENTRY *xaAndroidBufferQueueCallback)(
    XAAndroidBufferQueueItf caller,/* input */
    void *pCallbackContext,        /* input */
    void *pBufferContext,          /* input */
    void *pBufferData,             /* input */
    XAuint32 dataSize,             /* input */
    XAuint32 dataUsed,             /* input */
    const XAAndroidBufferItem *pItems,/* input */
    XAuint32 itemsLength           /* input */
);

typedef struct XAAndroidBufferQueueState_ {
    XAuint32    count;
    XAuint32    index;
} XAAndroidBufferQueueState;

struct XAAndroidBufferQueueItf_ {
    XAresult (*RegisterCallback) (
        XAAndroidBufferQueueItf self,
        xaAndroidBufferQueueCallback callback,
        void* pCallbackContext
    );

    XAresult (*Clear) (
        XAAndroidBufferQueueItf self
    );

    XAresult (*Enqueue) (
        XAAndroidBufferQueueItf self,
        void *pBufferContext,
        void *pData,
        XAuint32 dataLength,
        const XAAndroidBufferItem *pItems,
        XAuint32 itemsLength
    );

    XAresult (*GetState) (
        XAAndroidBufferQueueItf self,
        XAAndroidBufferQueueState *pState
    );


    XAresult (*SetCallbackEventsMask) (
            XAAndroidBufferQueueItf self,
            XAuint32 eventFlags
    );

    XAresult (*GetCallbackEventsMask) (
            XAAndroidBufferQueueItf self,
            XAuint32 *pEventFlags
    );
};


/*---------------------------------------------------------------------------*/
/* Android Buffer Queue Data Locator                                         */
/*---------------------------------------------------------------------------*/

/** Addendum to Data locator macros  */
#define XA_DATALOCATOR_ANDROIDBUFFERQUEUE       ((XAuint32) 0x800007BE)

/** Android Buffer Queue-based data locator definition,
 *  locatorType must be XA_DATALOCATOR_ANDROIDBUFFERQUEUE */
typedef struct XADataLocator_AndroidBufferQueue_ {
    XAuint32    locatorType;
    XAuint32    numBuffers;
} XADataLocator_AndroidBufferQueue;


/*---------------------------------------------------------------------------*/
/* Android File Descriptor Data Locator                                      */
/*---------------------------------------------------------------------------*/

/** Addendum to Data locator macros  */
#define XA_DATALOCATOR_ANDROIDFD                ((XAuint32) 0x800007BC)

#define XA_DATALOCATOR_ANDROIDFD_USE_FILE_SIZE ((XAAint64) 0xFFFFFFFFFFFFFFFFll)

/** File Descriptor-based data locator definition, locatorType must be XA_DATALOCATOR_ANDROIDFD */
typedef struct XADataLocator_AndroidFD_ {
    XAuint32        locatorType;
    XAint32         fd;
    XAAint64        offset;
    XAAint64        length;
} XADataLocator_AndroidFD;

/**
 * MIME types required for data in Android Buffer Queues
 */
#define XA_ANDROID_MIME_MP2TS              ((XAchar *) "video/mp2ts")

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* OPENMAX_AL_ANDROID_H_ */
