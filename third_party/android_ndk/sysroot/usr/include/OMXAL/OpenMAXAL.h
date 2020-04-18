/*
 * Copyright (c) 2007-2010 The Khronos Group Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and/or associated documentation files (the
 * "Materials "), to deal in the Materials without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Materials, and to
 * permit persons to whom the Materials are furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Materials.
 *
 * THE MATERIALS ARE PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * MATERIALS OR THE USE OR OTHER DEALINGS IN THE MATERIALS.
 *
 * OpenMAXAL.h - OpenMAX AL version 1.0.1
 *
 */

/****************************************************************************/
/* NOTE: This file is a standard OpenMAX AL header file and should not be   */
/* modified in any way.                                                     */
/****************************************************************************/

#ifndef _OPENMAXAL_H_
#define _OPENMAXAL_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "OpenMAXAL_Platform.h"


  /*****************************************************************/
  /* TYPES                                                         */
  /*****************************************************************/

/* remap common types to XA types for clarity */
typedef xa_int8_t   XAint8;   /* 8 bit signed integer    */
typedef xa_uint8_t  XAuint8;  /* 8 bit unsigned integer  */
typedef xa_int16_t  XAint16;  /* 16 bit signed integer   */
typedef xa_uint16_t XAuint16; /* 16 bit unsigned integer */
typedef xa_int32_t  XAint32;  /* 32 bit signed integer   */
typedef xa_uint32_t XAuint32; /* 32 bit unsigned integer */
typedef xa_uint64_t XAuint64; /* 64 bit unsigned integer */

typedef XAuint32    XAboolean;
typedef XAuint8     XAchar;
typedef XAint16     XAmillibel;
typedef XAuint32    XAmillisecond;
typedef XAuint32    XAmilliHertz;
typedef XAint32     XAmillimeter;
typedef XAint32     XAmillidegree;
typedef XAint16     XApermille;
typedef XAuint32    XAmicrosecond;
typedef XAuint64    XAtime;
typedef XAuint32    XAresult;

#define XA_BOOLEAN_FALSE                    ((XAuint32) 0x00000000)
#define XA_BOOLEAN_TRUE                     ((XAuint32) 0x00000001)

#define XA_MILLIBEL_MAX                     ((XAmillibel) 0x7FFF)
#define XA_MILLIBEL_MIN                     ((XAmillibel) (-XA_MILLIBEL_MAX-1))

#define XA_MILLIHERTZ_MAX                   ((XAmilliHertz) 0xFFFFFFFF)

#define XA_MILLIMETER_MAX                   ((XAmillimeter) 0x7FFFFFFF)



  /*****************************************************************/
  /* RESULT CODES                                                  */
  /*****************************************************************/

#define XA_RESULT_SUCCESS                   ((XAuint32) 0x00000000)
#define XA_RESULT_PRECONDITIONS_VIOLATED    ((XAuint32) 0x00000001)
#define XA_RESULT_PARAMETER_INVALID         ((XAuint32) 0x00000002)
#define XA_RESULT_MEMORY_FAILURE            ((XAuint32) 0x00000003)
#define XA_RESULT_RESOURCE_ERROR            ((XAuint32) 0x00000004)
#define XA_RESULT_RESOURCE_LOST             ((XAuint32) 0x00000005)
#define XA_RESULT_IO_ERROR                  ((XAuint32) 0x00000006)
#define XA_RESULT_BUFFER_INSUFFICIENT       ((XAuint32) 0x00000007)
#define XA_RESULT_CONTENT_CORRUPTED         ((XAuint32) 0x00000008)
#define XA_RESULT_CONTENT_UNSUPPORTED       ((XAuint32) 0x00000009)
#define XA_RESULT_CONTENT_NOT_FOUND         ((XAuint32) 0x0000000A)
#define XA_RESULT_PERMISSION_DENIED         ((XAuint32) 0x0000000B)
#define XA_RESULT_FEATURE_UNSUPPORTED       ((XAuint32) 0x0000000C)
#define XA_RESULT_INTERNAL_ERROR            ((XAuint32) 0x0000000D)
#define XA_RESULT_UNKNOWN_ERROR             ((XAuint32) 0x0000000E)
#define XA_RESULT_OPERATION_ABORTED         ((XAuint32) 0x0000000F)
#define XA_RESULT_CONTROL_LOST              ((XAuint32) 0x00000010)



  /*****************************************************************/
  /* INTERFACE ID DEFINITION                                       */
  /*****************************************************************/

/* Interface ID defined as a UUID */
typedef const struct XAInterfaceID_ {
    XAuint32 time_low;
    XAuint16 time_mid;
    XAuint16 time_hi_and_version;
    XAuint16 clock_seq;
    XAuint8  node[6];
} * XAInterfaceID;

/* NULL Interface */
XA_API extern const XAInterfaceID XA_IID_NULL;



  /*****************************************************************/
  /* GENERAL INTERFACES, STRUCTS AND DEFINES                       */
  /*****************************************************************/

/* OBJECT */

#define XA_PRIORITY_LOWEST                      ((XAint32) (-0x7FFFFFFF-1))
#define XA_PRIORITY_VERYLOW                     ((XAint32) -0x60000000)
#define XA_PRIORITY_LOW                         ((XAint32) -0x40000000)
#define XA_PRIORITY_BELOWNORMAL                 ((XAint32) -0x20000000)
#define XA_PRIORITY_NORMAL                      ((XAint32) 0x00000000)
#define XA_PRIORITY_ABOVENORMAL                 ((XAint32) 0x20000000)
#define XA_PRIORITY_HIGH                        ((XAint32) 0x40000000)
#define XA_PRIORITY_VERYHIGH                    ((XAint32) 0x60000000)
#define XA_PRIORITY_HIGHEST                     ((XAint32) 0x7FFFFFFF)

#define XA_OBJECT_EVENT_RUNTIME_ERROR           ((XAuint32) 0x00000001)
#define XA_OBJECT_EVENT_ASYNC_TERMINATION       ((XAuint32) 0x00000002)
#define XA_OBJECT_EVENT_RESOURCES_LOST          ((XAuint32) 0x00000003)
#define XA_OBJECT_EVENT_RESOURCES_AVAILABLE     ((XAuint32) 0x00000004)
#define XA_OBJECT_EVENT_ITF_CONTROL_TAKEN       ((XAuint32) 0x00000005)
#define XA_OBJECT_EVENT_ITF_CONTROL_RETURNED    ((XAuint32) 0x00000006)
#define XA_OBJECT_EVENT_ITF_PARAMETERS_CHANGED  ((XAuint32) 0x00000007)

#define XA_OBJECT_STATE_UNREALIZED              ((XAuint32) 0x00000001)
#define XA_OBJECT_STATE_REALIZED                ((XAuint32) 0x00000002)
#define XA_OBJECT_STATE_SUSPENDED               ((XAuint32) 0x00000003)


XA_API extern const XAInterfaceID XA_IID_OBJECT;

struct XAObjectItf_;
typedef const struct XAObjectItf_ * const * XAObjectItf;

typedef void (XAAPIENTRY * xaObjectCallback) (
    XAObjectItf caller,
    const void * pContext,
    XAuint32 event,
    XAresult result,
    XAuint32 param,
    void * pInterface
);

struct XAObjectItf_ {
    XAresult (*Realize) (
        XAObjectItf self,
        XAboolean async
    );
    XAresult (*Resume) (
        XAObjectItf self,
        XAboolean async
    );
    XAresult (*GetState) (
        XAObjectItf self,
        XAuint32 * pState
    );
    XAresult (*GetInterface) (
        XAObjectItf self,
        const XAInterfaceID iid,
        void * pInterface
    );
    XAresult (*RegisterCallback) (
        XAObjectItf self,
        xaObjectCallback callback,
        void * pContext
    );
    void (*AbortAsyncOperation) (
        XAObjectItf self
    );
    void (*Destroy) (
        XAObjectItf self
    );
    XAresult (*SetPriority) (
        XAObjectItf self,
        XAint32 priority,
        XAboolean preemptable
    );
    XAresult (*GetPriority) (
        XAObjectItf self,
        XAint32 * pPriority,
        XAboolean * pPreemptable
    );
    XAresult (*SetLossOfControlInterfaces) (
        XAObjectItf self,
        XAint16 numInterfaces,
        XAInterfaceID * pInterfaceIDs,
        XAboolean enabled
    );
};

/* CONFIG EXTENSION */

XA_API extern const XAInterfaceID XA_IID_CONFIGEXTENSION;

struct XAConfigExtensionsItf_;
typedef const struct XAConfigExtensionsItf_
    * const * XAConfigExtensionsItf;

struct XAConfigExtensionsItf_ {
    XAresult (*SetConfiguration) (
        XAConfigExtensionsItf self,
        const XAchar * configKey,
        XAuint32 valueSize,
        const void * pConfigValue
    );
    XAresult (*GetConfiguration) (
        XAConfigExtensionsItf self,
        const XAchar * configKey,
        XAuint32 * pValueSize,
        void * pConfigValue
    );
};

/* DYNAMIC INTERFACE MANAGEMENT */

#define XA_DYNAMIC_ITF_EVENT_RUNTIME_ERROR              ((XAuint32) 0x00000001)
#define XA_DYNAMIC_ITF_EVENT_ASYNC_TERMINATION          ((XAuint32) 0x00000002)
#define XA_DYNAMIC_ITF_EVENT_RESOURCES_LOST             ((XAuint32) 0x00000003)
#define XA_DYNAMIC_ITF_EVENT_RESOURCES_LOST_PERMANENTLY ((XAuint32) 0x00000004)
#define XA_DYNAMIC_ITF_EVENT_RESOURCES_AVAILABLE        ((XAuint32) 0x00000005)

XA_API extern const XAInterfaceID XA_IID_DYNAMICINTERFACEMANAGEMENT;

struct XADynamicInterfaceManagementItf_;
typedef const struct XADynamicInterfaceManagementItf_
    * const * XADynamicInterfaceManagementItf;

typedef void (XAAPIENTRY * xaDynamicInterfaceManagementCallback) (
    XADynamicInterfaceManagementItf caller,
    void * pContext,
    XAuint32 event,
    XAresult result,
    const XAInterfaceID iid
);

struct XADynamicInterfaceManagementItf_ {
    XAresult (*AddInterface) (
        XADynamicInterfaceManagementItf self,
        const XAInterfaceID iid,
        XAboolean aysnc
    );
    XAresult (*RemoveInterface) (
        XADynamicInterfaceManagementItf self,
        const XAInterfaceID iid
    );
    XAresult (*ResumeInterface) (
        XADynamicInterfaceManagementItf self,
        const XAInterfaceID iid,
        XAboolean aysnc
    );
    XAresult (*RegisterCallback) (
        XADynamicInterfaceManagementItf self,
        xaDynamicInterfaceManagementCallback callback,
        void * pContext
    );
};

/* DATA SOURCES/SINKS */

#define XA_DATAFORMAT_MIME              ((XAuint32) 0x00000001)
#define XA_DATAFORMAT_PCM               ((XAuint32) 0x00000002)
#define XA_DATAFORMAT_RAWIMAGE          ((XAuint32) 0x00000003)

#define XA_DATALOCATOR_URI              ((XAuint32) 0x00000001)
#define XA_DATALOCATOR_ADDRESS          ((XAuint32) 0x00000002)
#define XA_DATALOCATOR_IODEVICE         ((XAuint32) 0x00000003)
#define XA_DATALOCATOR_OUTPUTMIX        ((XAuint32) 0x00000004)
#define XA_DATALOCATOR_NATIVEDISPLAY    ((XAuint32) 0x00000005)
#define XA_DATALOCATOR_RESERVED6        ((XAuint32) 0x00000006)
#define XA_DATALOCATOR_RESERVED7        ((XAuint32) 0x00000007)

typedef struct XADataSink_ {
    void * pLocator;
    void * pFormat;
} XADataSink;

typedef struct XADataSource_ {
    void * pLocator;
    void * pFormat;
} XADataSource;

#define XA_CONTAINERTYPE_UNSPECIFIED    ((XAuint32) 0x00000001)
#define XA_CONTAINERTYPE_RAW            ((XAuint32) 0x00000002)
#define XA_CONTAINERTYPE_ASF            ((XAuint32) 0x00000003)
#define XA_CONTAINERTYPE_AVI            ((XAuint32) 0x00000004)
#define XA_CONTAINERTYPE_BMP            ((XAuint32) 0x00000005)
#define XA_CONTAINERTYPE_JPG            ((XAuint32) 0x00000006)
#define XA_CONTAINERTYPE_JPG2000        ((XAuint32) 0x00000007)
#define XA_CONTAINERTYPE_M4A            ((XAuint32) 0x00000008)
#define XA_CONTAINERTYPE_MP3            ((XAuint32) 0x00000009)
#define XA_CONTAINERTYPE_MP4            ((XAuint32) 0x0000000A)
#define XA_CONTAINERTYPE_MPEG_ES        ((XAuint32) 0x0000000B)
#define XA_CONTAINERTYPE_MPEG_PS        ((XAuint32) 0x0000000C)
#define XA_CONTAINERTYPE_MPEG_TS        ((XAuint32) 0x0000000D)
#define XA_CONTAINERTYPE_QT             ((XAuint32) 0x0000000E)
#define XA_CONTAINERTYPE_WAV            ((XAuint32) 0x0000000F)
#define XA_CONTAINERTYPE_XMF_0          ((XAuint32) 0x00000010)
#define XA_CONTAINERTYPE_XMF_1          ((XAuint32) 0x00000011)
#define XA_CONTAINERTYPE_XMF_2          ((XAuint32) 0x00000012)
#define XA_CONTAINERTYPE_XMF_3          ((XAuint32) 0x00000013)
#define XA_CONTAINERTYPE_XMF_GENERIC    ((XAuint32) 0x00000014)
#define XA_CONTAINERTYPE_AMR            ((XAuint32) 0x00000015)
#define XA_CONTAINERTYPE_AAC            ((XAuint32) 0x00000016)
#define XA_CONTAINERTYPE_3GPP           ((XAuint32) 0x00000017)
#define XA_CONTAINERTYPE_3GA            ((XAuint32) 0x00000018)
#define XA_CONTAINERTYPE_RM             ((XAuint32) 0x00000019)
#define XA_CONTAINERTYPE_DMF            ((XAuint32) 0x0000001A)
#define XA_CONTAINERTYPE_SMF            ((XAuint32) 0x0000001B)
#define XA_CONTAINERTYPE_MOBILE_DLS     ((XAuint32) 0x0000001C)
#define XA_CONTAINERTYPE_OGG            ((XAuint32) 0x0000001D)

typedef struct XADataFormat_MIME_ {
    XAuint32 formatType;
    XAchar * mimeType;
    XAuint32 containerType;
} XADataFormat_MIME;

#define XA_BYTEORDER_BIGENDIAN          ((XAuint32) 0x00000001)
#define XA_BYTEORDER_LITTLEENDIAN       ((XAuint32) 0x00000002)

#define XA_SAMPLINGRATE_8               ((XAuint32)   8000000)
#define XA_SAMPLINGRATE_11_025          ((XAuint32)  11025000)
#define XA_SAMPLINGRATE_12              ((XAuint32)  12000000)
#define XA_SAMPLINGRATE_16              ((XAuint32)  16000000)
#define XA_SAMPLINGRATE_22_05           ((XAuint32)  22050000)
#define XA_SAMPLINGRATE_24              ((XAuint32)  24000000)
#define XA_SAMPLINGRATE_32              ((XAuint32)  32000000)
#define XA_SAMPLINGRATE_44_1            ((XAuint32)  44100000)
#define XA_SAMPLINGRATE_48              ((XAuint32)  48000000)
#define XA_SAMPLINGRATE_64              ((XAuint32)  64000000)
#define XA_SAMPLINGRATE_88_2            ((XAuint32)  88200000)
#define XA_SAMPLINGRATE_96              ((XAuint32)  96000000)
#define XA_SAMPLINGRATE_192             ((XAuint32) 192000000)

#define XA_SPEAKER_FRONT_LEFT               ((XAuint32) 0x00000001)
#define XA_SPEAKER_FRONT_RIGHT              ((XAuint32) 0x00000002)
#define XA_SPEAKER_FRONT_CENTER             ((XAuint32) 0x00000004)
#define XA_SPEAKER_LOW_FREQUENCY            ((XAuint32) 0x00000008)
#define XA_SPEAKER_BACK_LEFT                ((XAuint32) 0x00000010)
#define XA_SPEAKER_BACK_RIGHT               ((XAuint32) 0x00000020)
#define XA_SPEAKER_FRONT_LEFT_OF_CENTER     ((XAuint32) 0x00000040)
#define XA_SPEAKER_FRONT_RIGHT_OF_CENTER    ((XAuint32) 0x00000080)
#define XA_SPEAKER_BACK_CENTER              ((XAuint32) 0x00000100)
#define XA_SPEAKER_SIDE_LEFT                ((XAuint32) 0x00000200)
#define XA_SPEAKER_SIDE_RIGHT               ((XAuint32) 0x00000400)
#define XA_SPEAKER_TOP_CENTER               ((XAuint32) 0x00000800)
#define XA_SPEAKER_TOP_FRONT_LEFT           ((XAuint32) 0x00001000)
#define XA_SPEAKER_TOP_FRONT_CENTER         ((XAuint32) 0x00002000)
#define XA_SPEAKER_TOP_FRONT_RIGHT          ((XAuint32) 0x00004000)
#define XA_SPEAKER_TOP_BACK_LEFT            ((XAuint32) 0x00008000)
#define XA_SPEAKER_TOP_BACK_CENTER          ((XAuint32) 0x00010000)
#define XA_SPEAKER_TOP_BACK_RIGHT           ((XAuint32) 0x00020000)

#define XA_PCMSAMPLEFORMAT_FIXED_8          ((XAuint16) 0x0008)
#define XA_PCMSAMPLEFORMAT_FIXED_16         ((XAuint16) 0x0010)
#define XA_PCMSAMPLEFORMAT_FIXED_20         ((XAuint16) 0x0014)
#define XA_PCMSAMPLEFORMAT_FIXED_24         ((XAuint16) 0x0018)
#define XA_PCMSAMPLEFORMAT_FIXED_28         ((XAuint16) 0x001C)
#define XA_PCMSAMPLEFORMAT_FIXED_32         ((XAuint16) 0x0020)

typedef struct XADataFormat_PCM_ {
    XAuint32 formatType;
    XAuint32 numChannels;
    XAuint32 samplesPerSec;
    XAuint32 bitsPerSample;
    XAuint32 containerSize;
    XAuint32 channelMask;
    XAuint32 endianness;
} XADataFormat_PCM;

#define XA_COLORFORMAT_UNUSED                   ((XAuint32) 0x00000000)
#define XA_COLORFORMAT_MONOCHROME               ((XAuint32) 0x00000001)
#define XA_COLORFORMAT_8BITRGB332               ((XAuint32) 0x00000002)
#define XA_COLORFORMAT_12BITRGB444              ((XAuint32) 0x00000003)
#define XA_COLORFORMAT_16BITARGB4444            ((XAuint32) 0x00000004)
#define XA_COLORFORMAT_16BITARGB1555            ((XAuint32) 0x00000005)
#define XA_COLORFORMAT_16BITRGB565              ((XAuint32) 0x00000006)
#define XA_COLORFORMAT_16BITBGR565              ((XAuint32) 0x00000007)
#define XA_COLORFORMAT_18BITRGB666              ((XAuint32) 0x00000008)
#define XA_COLORFORMAT_18BITARGB1665            ((XAuint32) 0x00000009)
#define XA_COLORFORMAT_19BITARGB1666            ((XAuint32) 0x0000000A)
#define XA_COLORFORMAT_24BITRGB888              ((XAuint32) 0x0000000B)
#define XA_COLORFORMAT_24BITBGR888              ((XAuint32) 0x0000000C)
#define XA_COLORFORMAT_24BITARGB1887            ((XAuint32) 0x0000000D)
#define XA_COLORFORMAT_25BITARGB1888            ((XAuint32) 0x0000000E)
#define XA_COLORFORMAT_32BITBGRA8888            ((XAuint32) 0x0000000F)
#define XA_COLORFORMAT_32BITARGB8888            ((XAuint32) 0x00000010)
#define XA_COLORFORMAT_YUV411PLANAR             ((XAuint32) 0x00000011)
#define XA_COLORFORMAT_YUV420PLANAR             ((XAuint32) 0x00000013)
#define XA_COLORFORMAT_YUV420SEMIPLANAR         ((XAuint32) 0x00000015)
#define XA_COLORFORMAT_YUV422PLANAR             ((XAuint32) 0x00000016)
#define XA_COLORFORMAT_YUV422SEMIPLANAR         ((XAuint32) 0x00000018)
#define XA_COLORFORMAT_YCBYCR                   ((XAuint32) 0x00000019)
#define XA_COLORFORMAT_YCRYCB                   ((XAuint32) 0x0000001A)
#define XA_COLORFORMAT_CBYCRY                   ((XAuint32) 0x0000001B)
#define XA_COLORFORMAT_CRYCBY                   ((XAuint32) 0x0000001C)
#define XA_COLORFORMAT_YUV444INTERLEAVED        ((XAuint32) 0x0000001D)
#define XA_COLORFORMAT_RAWBAYER8BIT             ((XAuint32) 0x0000001E)
#define XA_COLORFORMAT_RAWBAYER10BIT            ((XAuint32) 0x0000001F)
#define XA_COLORFORMAT_RAWBAYER8BITCOMPRESSED   ((XAuint32) 0x00000020)
#define XA_COLORFORMAT_L2                       ((XAuint32) 0x00000021)
#define XA_COLORFORMAT_L4                       ((XAuint32) 0x00000022)
#define XA_COLORFORMAT_L8                       ((XAuint32) 0x00000023)
#define XA_COLORFORMAT_L16                      ((XAuint32) 0x00000024)
#define XA_COLORFORMAT_L24                      ((XAuint32) 0x00000025)
#define XA_COLORFORMAT_L32                      ((XAuint32) 0x00000026)
#define XA_COLORFORMAT_18BITBGR666              ((XAuint32) 0x00000029)
#define XA_COLORFORMAT_24BITARGB6666            ((XAuint32) 0x0000002A)
#define XA_COLORFORMAT_24BITABGR6666            ((XAuint32) 0x0000002B)

typedef struct XADataFormat_RawImage_ {
    XAuint32 formatType;
    XAuint32 colorFormat;
    XAuint32 height;
    XAuint32 width;
    XAuint32 stride;
} XADataFormat_RawImage;

typedef struct XADataLocator_Address_ {
    XAuint32 locatorType;
    void * pAddress;
    XAuint32 length;
} XADataLocator_Address;

#define XA_IODEVICE_AUDIOINPUT          ((XAuint32) 0x00000001)
#define XA_IODEVICE_LEDARRAY            ((XAuint32) 0x00000002)
#define XA_IODEVICE_VIBRA               ((XAuint32) 0x00000003)
#define XA_IODEVICE_CAMERA              ((XAuint32) 0x00000004)
#define XA_IODEVICE_RADIO               ((XAuint32) 0x00000005)

typedef struct XADataLocator_IODevice_ {
    XAuint32 locatorType;
    XAuint32 deviceType;
    XAuint32 deviceID;
    XAObjectItf device;
} XADataLocator_IODevice;

typedef void * XANativeHandle;

typedef struct XADataLocator_NativeDisplay_{
    XAuint32 locatorType;
    XANativeHandle hWindow;
    XANativeHandle hDisplay;
} XADataLocator_NativeDisplay;

typedef struct XADataLocator_OutputMix {
    XAuint32 locatorType;
    XAObjectItf outputMix;
} XADataLocator_OutputMix;

typedef struct XADataLocator_URI_ {
    XAuint32 locatorType;
    XAchar * URI;
} XADataLocator_URI;


/* ENGINE */

#define XA_DEFAULTDEVICEID_AUDIOINPUT   ((XAuint32) 0xFFFFFFFF)
#define XA_DEFAULTDEVICEID_AUDIOOUTPUT  ((XAuint32) 0xFFFFFFFE)
#define XA_DEFAULTDEVICEID_LED          ((XAuint32) 0xFFFFFFFD)
#define XA_DEFAULTDEVICEID_VIBRA        ((XAuint32) 0xFFFFFFFC)
#define XA_DEFAULTDEVICEID_CAMERA       ((XAuint32) 0xFFFFFFFB)

#define XA_ENGINEOPTION_THREADSAFE      ((XAuint32) 0x00000001)
#define XA_ENGINEOPTION_LOSSOFCONTROL   ((XAuint32) 0x00000002)

#define XA_OBJECTID_ENGINE              ((XAuint32) 0x00000001)
#define XA_OBJECTID_LEDDEVICE           ((XAuint32) 0x00000002)
#define XA_OBJECTID_VIBRADEVICE         ((XAuint32) 0x00000003)
#define XA_OBJECTID_MEDIAPLAYER         ((XAuint32) 0x00000004)
#define XA_OBJECTID_MEDIARECORDER       ((XAuint32) 0x00000005)
#define XA_OBJECTID_RADIODEVICE         ((XAuint32) 0x00000006)
#define XA_OBJECTID_OUTPUTMIX           ((XAuint32) 0x00000007)
#define XA_OBJECTID_METADATAEXTRACTOR   ((XAuint32) 0x00000008)
#define XA_OBJECTID_CAMERADEVICE        ((XAuint32) 0x00000009)

#define XA_PROFILES_MEDIA_PLAYER            ((XAint16) 0x0001)
#define XA_PROFILES_MEDIA_PLAYER_RECORDER   ((XAint16) 0x0002)
#define XA_PROFILES_PLUS_MIDI               ((XAint16) 0x0004)

typedef struct XAEngineOption_ {
    XAuint32 feature;
    XAuint32 data;
} XAEngineOption;

XA_API XAresult XAAPIENTRY xaCreateEngine(
    XAObjectItf * pEngine,
    XAuint32 numOptions,
    const XAEngineOption * pEngineOptions,
    XAuint32 numInterfaces,
    const XAInterfaceID * pInterfaceIds,
    const XAboolean * pInterfaceRequired
);

XA_API XAresult XAAPIENTRY xaQueryNumSupportedEngineInterfaces(
    XAuint32 * pNumSupportedInterfaces
);

XA_API XAresult XAAPIENTRY xaQuerySupportedEngineInterfaces(
    XAuint32 index,
    XAInterfaceID * pInterfaceId
);

typedef struct XALEDDescriptor_ {
    XAuint8 ledCount;
    XAuint8 primaryLED;
    XAuint32 colorMask;
} XALEDDescriptor;

typedef struct XAVibraDescriptor_ {
    XAboolean supportsFrequency;
    XAboolean supportsIntensity;
    XAmilliHertz minFrequency;
    XAmilliHertz maxFrequency;
} XAVibraDescriptor;


XA_API extern const XAInterfaceID XA_IID_ENGINE;

struct XAEngineItf_;
typedef const struct XAEngineItf_ * const * XAEngineItf;

struct XAEngineItf_ {
    XAresult (*CreateCameraDevice) (
        XAEngineItf self,
        XAObjectItf * pDevice,
        XAuint32 deviceID,
        XAuint32 numInterfaces,
        const XAInterfaceID * pInterfaceIds,
        const XAboolean * pInterfaceRequired
    );
    XAresult (*CreateRadioDevice) (
        XAEngineItf self,
        XAObjectItf * pDevice,
        XAuint32 numInterfaces,
        const XAInterfaceID * pInterfaceIds,
        const XAboolean * pInterfaceRequired
    );
    XAresult (*CreateLEDDevice) (
        XAEngineItf self,
        XAObjectItf * pDevice,
        XAuint32 deviceID,
        XAuint32 numInterfaces,
        const XAInterfaceID * pInterfaceIds,
        const XAboolean * pInterfaceRequired
    );
       XAresult (*CreateVibraDevice) (
        XAEngineItf self,
        XAObjectItf * pDevice,
        XAuint32 deviceID,
        XAuint32 numInterfaces,
        const XAInterfaceID * pInterfaceIds,
        const XAboolean * pInterfaceRequired
    );
    XAresult (*CreateMediaPlayer) (
        XAEngineItf self,
        XAObjectItf * pPlayer,
        XADataSource * pDataSrc,
        XADataSource * pBankSrc,
        XADataSink * pAudioSnk,
        XADataSink * pImageVideoSnk,
        XADataSink * pVibra,
        XADataSink * pLEDArray,
        XAuint32 numInterfaces,
        const XAInterfaceID * pInterfaceIds,
        const XAboolean * pInterfaceRequired
    );
    XAresult (*CreateMediaRecorder) (
        XAEngineItf self,
        XAObjectItf * pRecorder,
        XADataSource * pAudioSrc,
        XADataSource * pImageVideoSrc,
        XADataSink * pDataSnk,
        XAuint32 numInterfaces,
        const XAInterfaceID * pInterfaceIds,
        const XAboolean * pInterfaceRequired
    );
    XAresult (*CreateOutputMix) (
        XAEngineItf self,
        XAObjectItf * pMix,
        XAuint32 numInterfaces,
        const XAInterfaceID * pInterfaceIds,
        const XAboolean * pInterfaceRequired
    );
    XAresult (*CreateMetadataExtractor) (
        XAEngineItf self,
        XAObjectItf * pMetadataExtractor,
        XADataSource * pDataSource,
        XAuint32 numInterfaces,
        const XAInterfaceID * pInterfaceIds,
        const XAboolean * pInterfaceRequired
    );
    XAresult (*CreateExtensionObject) (
        XAEngineItf self,
        XAObjectItf * pObject,
        void * pParameters,
        XAuint32 objectID,
        XAuint32 numInterfaces,
        const XAInterfaceID * pInterfaceIds,
        const XAboolean * pInterfaceRequired
    );
    XAresult (*GetImplementationInfo) (
        XAEngineItf self,
        XAuint32 * pMajor,
        XAuint32 * pMinor,
        XAuint32 * pStep,
        const XAchar * pImplementationText
    );
    XAresult (*QuerySupportedProfiles) (
        XAEngineItf self,
        XAint16 * pProfilesSupported
    );
    XAresult (*QueryNumSupportedInterfaces) (
        XAEngineItf self,
        XAuint32 objectID,
        XAuint32 * pNumSupportedInterfaces
    );
    XAresult (*QuerySupportedInterfaces) (
        XAEngineItf self,
        XAuint32 objectID,
        XAuint32 index,
        XAInterfaceID * pInterfaceId
    );
    XAresult (*QueryNumSupportedExtensions) (
        XAEngineItf self,
        XAuint32 * pNumExtensions
    );
    XAresult (*QuerySupportedExtension) (
        XAEngineItf self,
        XAuint32 index,
        XAchar * pExtensionName,
        XAint16 * pNameLength
    );
    XAresult (*IsExtensionSupported) (
        XAEngineItf self,
        const XAchar * pExtensionName,
        XAboolean * pSupported
    );
    XAresult (*QueryLEDCapabilities) (
        XAEngineItf self,
        XAuint32 *pIndex,
        XAuint32 * pLEDDeviceID,
        XALEDDescriptor * pDescriptor
    );
    XAresult (*QueryVibraCapabilities) (
        XAEngineItf self,
        XAuint32 *pIndex,
        XAuint32 * pVibraDeviceID,
        XAVibraDescriptor * pDescriptor
    );
};

/* THREAD SYNC */

XA_API extern const XAInterfaceID XA_IID_THREADSYNC;

struct XAThreadSyncItf_;
typedef const struct XAThreadSyncItf_ * const * XAThreadSyncItf;

struct XAThreadSyncItf_ {
    XAresult (*EnterCriticalSection) (
        XAThreadSyncItf self
    );
    XAresult (*ExitCriticalSection) (
        XAThreadSyncItf self
    );
};



  /*****************************************************************/
  /* PLAYBACK RELATED INTERFACES, STRUCTS AND DEFINES              */
  /*****************************************************************/

/* PLAY */

#define XA_TIME_UNKNOWN                     ((XAuint32) 0xFFFFFFFF)

#define XA_PLAYEVENT_HEADATEND              ((XAuint32) 0x00000001)
#define XA_PLAYEVENT_HEADATMARKER           ((XAuint32) 0x00000002)
#define XA_PLAYEVENT_HEADATNEWPOS           ((XAuint32) 0x00000004)
#define XA_PLAYEVENT_HEADMOVING             ((XAuint32) 0x00000008)
#define XA_PLAYEVENT_HEADSTALLED            ((XAuint32) 0x00000010)

#define XA_PLAYSTATE_STOPPED                ((XAuint32) 0x00000001)
#define XA_PLAYSTATE_PAUSED                 ((XAuint32) 0x00000002)
#define XA_PLAYSTATE_PLAYING                ((XAuint32) 0x00000003)

#define XA_PREFETCHEVENT_STATUSCHANGE       ((XAuint32) 0x00000001)
#define XA_PREFETCHEVENT_FILLLEVELCHANGE    ((XAuint32) 0x00000002)

#define XA_PREFETCHSTATUS_UNDERFLOW         ((XAuint32) 0x00000001)
#define XA_PREFETCHSTATUS_SUFFICIENTDATA    ((XAuint32) 0x00000002)
#define XA_PREFETCHSTATUS_OVERFLOW          ((XAuint32) 0x00000003)

#define XA_SEEKMODE_FAST                    ((XAuint32) 0x0001)
#define XA_SEEKMODE_ACCURATE                ((XAuint32) 0x0002)

XA_API extern const XAInterfaceID XA_IID_PLAY;

struct XAPlayItf_;
typedef const struct XAPlayItf_ * const * XAPlayItf;

typedef void (XAAPIENTRY * xaPlayCallback) (
    XAPlayItf caller,
    void * pContext,
    XAuint32 event
);

struct XAPlayItf_ {
    XAresult (*SetPlayState) (
        XAPlayItf self,
        XAuint32 state
    );
    XAresult (*GetPlayState) (
        XAPlayItf self,
        XAuint32 * pState
    );
    XAresult (*GetDuration) (
        XAPlayItf self,
        XAmillisecond * pMsec
    );
    XAresult (*GetPosition) (
        XAPlayItf self,
        XAmillisecond * pMsec
    );
    XAresult (*RegisterCallback) (
        XAPlayItf self,
        xaPlayCallback callback,
        void * pContext
    );
    XAresult (*SetCallbackEventsMask) (
        XAPlayItf self,
        XAuint32 eventFlags
    );
    XAresult (*GetCallbackEventsMask) (
        XAPlayItf self,
        XAuint32 * pEventFlags
    );
    XAresult (*SetMarkerPosition) (
        XAPlayItf self,
        XAmillisecond mSec
    );
    XAresult (*ClearMarkerPosition) (
        XAPlayItf self
    );
    XAresult (*GetMarkerPosition) (
        XAPlayItf self,
        XAmillisecond * pMsec
    );
    XAresult (*SetPositionUpdatePeriod) (
        XAPlayItf self,
        XAmillisecond mSec
    );
    XAresult (*GetPositionUpdatePeriod) (
        XAPlayItf self,
        XAmillisecond * pMsec
    );
};

/* PLAYBACK RATE */

#define XA_RATEPROP_STAGGEREDVIDEO      ((XAuint32) 0x00000001)
#define XA_RATEPROP_SMOOTHVIDEO         ((XAuint32) 0x00000002)
#define XA_RATEPROP_SILENTAUDIO         ((XAuint32) 0x00000100)
#define XA_RATEPROP_STAGGEREDAUDIO      ((XAuint32) 0x00000200)
#define XA_RATEPROP_NOPITCHCORAUDIO     ((XAuint32) 0x00000400)
#define XA_RATEPROP_PITCHCORAUDIO       ((XAuint32) 0x00000800)

XA_API extern const XAInterfaceID XA_IID_PLAYBACKRATE;

struct XAPlaybackRateItf_;
typedef const struct XAPlaybackRateItf_ * const * XAPlaybackRateItf;

struct XAPlaybackRateItf_ {
    XAresult (*SetRate) (
        XAPlaybackRateItf self,
        XApermille rate
    );
    XAresult (*GetRate) (
        XAPlaybackRateItf self,
        XApermille * pRate
    );
    XAresult (*SetPropertyConstraints) (
        XAPlaybackRateItf self,
        XAuint32 constraints
    );
    XAresult (*GetProperties) (
        XAPlaybackRateItf self,
        XAuint32 * pProperties
    );
    XAresult (*GetCapabilitiesOfRate) (
        XAPlaybackRateItf self,
        XApermille rate,
        XAuint32 * pCapabilities
    );
    XAresult (*GetRateRange) (
        XAPlaybackRateItf self,
        XAuint8 index,
        XApermille * pMinRate,
        XApermille * pMaxRate,
        XApermille * pStepSize,
        XAuint32 * pCapabilities
    );
};

/* PREFETCH STATUS */

XA_API extern const XAInterfaceID XA_IID_PREFETCHSTATUS;

struct XAPrefetchStatusItf_;
typedef const struct XAPrefetchStatusItf_
    * const * XAPrefetchStatusItf;

typedef void (XAAPIENTRY * xaPrefetchCallback) (
    XAPrefetchStatusItf caller,
    void * pContext,
    XAuint32 event
);

struct XAPrefetchStatusItf_ {
    XAresult (*GetPrefetchStatus) (
        XAPrefetchStatusItf self,
        XAuint32 * pStatus
    );
    XAresult (*GetFillLevel) (
        XAPrefetchStatusItf self,
        XApermille * pLevel
    );
    XAresult (*RegisterCallback) (
        XAPrefetchStatusItf self,
        xaPrefetchCallback callback,
        void * pContext
    );
    XAresult (*SetCallbackEventsMask) (
        XAPrefetchStatusItf self,
        XAuint32 eventFlags
    );
    XAresult (*GetCallbackEventsMask) (
        XAPrefetchStatusItf self,
        XAuint32 * pEventFlags
    );
    XAresult (*SetFillUpdatePeriod) (
        XAPrefetchStatusItf self,
        XApermille period
    );
    XAresult (*GetFillUpdatePeriod) (
        XAPrefetchStatusItf self,
        XApermille * pPeriod
    );
};

/* SEEK */

XA_API extern const XAInterfaceID XA_IID_SEEK;

struct XASeekItf_;
typedef const struct XASeekItf_ * const * XASeekItf;

struct XASeekItf_ {
    XAresult (*SetPosition) (
        XASeekItf self,
        XAmillisecond pos,
        XAuint32 seekMode
    );
    XAresult (*SetLoop) (
        XASeekItf self,
        XAboolean loopEnable,
        XAmillisecond startPos,
        XAmillisecond endPos
    );
    XAresult (*GetLoop) (
        XASeekItf self,
        XAboolean * pLoopEnabled,
        XAmillisecond * pStartPos,
        XAmillisecond * pEndPos
    );
};

/* VOLUME */

XA_API extern const XAInterfaceID XA_IID_VOLUME;

struct XAVolumeItf_;
typedef const struct XAVolumeItf_ * const * XAVolumeItf;

struct XAVolumeItf_ {
    XAresult (*SetVolumeLevel) (
        XAVolumeItf self,
        XAmillibel level
    );
    XAresult (*GetVolumeLevel) (
        XAVolumeItf self,
        XAmillibel * pLevel
    );
    XAresult (*GetMaxVolumeLevel) (
        XAVolumeItf self,
        XAmillibel * pMaxLevel
    );
    XAresult (*SetMute) (
        XAVolumeItf self,
        XAboolean mute
    );
    XAresult (*GetMute) (
        XAVolumeItf self,
        XAboolean * pMute
    );
    XAresult (*EnableStereoPosition) (
        XAVolumeItf self,
        XAboolean enable
    );
    XAresult (*IsEnabledStereoPosition) (
        XAVolumeItf self,
        XAboolean * pEnable
    );
    XAresult (*SetStereoPosition) (
        XAVolumeItf self,
        XApermille stereoPosition
    );
    XAresult (*GetStereoPosition) (
        XAVolumeItf self,
        XApermille * pStereoPosition
    );
};

/* IMAGE CONTROL */

XA_API extern const XAInterfaceID XA_IID_IMAGECONTROLS;

struct XAImageControlsItf_;
typedef const struct XAImageControlsItf_ * const * XAImageControlsItf;

struct XAImageControlsItf_ {
    XAresult (*SetBrightness) (
        XAImageControlsItf self,
        XAuint32 brightness
    );
    XAresult (*GetBrightness) (
        XAImageControlsItf self,
        XAuint32 * pBrightness
    );
    XAresult (*SetContrast) (
        XAImageControlsItf self,
        XAint32 contrast
    );
    XAresult (*GetContrast) (
        XAImageControlsItf self,
        XAint32 * pContrast
    );
    XAresult (*SetGamma) (
        XAImageControlsItf self,
        XApermille gamma
    );
    XAresult (*GetGamma) (
        XAImageControlsItf self,
        XApermille * pGamma
    );
    XAresult (*GetSupportedGammaSettings) (
        XAImageControlsItf self,
        XApermille * pMinValue,
        XApermille * pMaxValue,
        XAuint32 * pNumSettings,
        XApermille ** ppSettings
    );
};

/* IMAGE EFFECT */

#define XA_IMAGEEFFECT_MONOCHROME       ((XAuint32) 0x00000001)
#define XA_IMAGEEFFECT_NEGATIVE         ((XAuint32) 0x00000002)
#define XA_IMAGEEFFECT_SEPIA            ((XAuint32) 0x00000003)
#define XA_IMAGEEFFECT_EMBOSS           ((XAuint32) 0x00000004)
#define XA_IMAGEEFFECT_PAINTBRUSH       ((XAuint32) 0x00000005)
#define XA_IMAGEEFFECT_SOLARIZE         ((XAuint32) 0x00000006)
#define XA_IMAGEEFFECT_CARTOON          ((XAuint32) 0x00000007)

XA_API extern const XAInterfaceID XA_IID_IMAGEEFFECTS;

struct XAImageEffectsItf_;
typedef const struct XAImageEffectsItf_ * const * XAImageEffectsItf;

struct XAImageEffectsItf_ {
    XAresult (*QuerySupportedImageEffects) (
        XAImageEffectsItf self,
        XAuint32 index,
        XAuint32 * pImageEffectId
    );
    XAresult (*EnableImageEffect) (
        XAImageEffectsItf self,
        XAuint32 imageEffectID
    );
    XAresult (*DisableImageEffect) (
        XAImageEffectsItf self,
        XAuint32 imageEffectID
    );
    XAresult (*IsImageEffectEnabled) (
        XAImageEffectsItf self,
        XAuint32 imageEffectID,
        XAboolean * pEnabled
    );
};

/* VIDEO POST PROCESSING */

#define XA_VIDEOMIRROR_NONE             ((XAuint32) 0x00000001)
#define XA_VIDEOMIRROR_VERTICAL         ((XAuint32) 0x00000002)
#define XA_VIDEOMIRROR_HORIZONTAL       ((XAuint32) 0x00000003)
#define XA_VIDEOMIRROR_BOTH             ((XAuint32) 0x00000004)

#define XA_VIDEOSCALE_STRETCH           ((XAuint32) 0x00000001)
#define XA_VIDEOSCALE_FIT               ((XAuint32) 0x00000002)
#define XA_VIDEOSCALE_CROP              ((XAuint32) 0x00000003)

#define XA_RENDERINGHINT_NONE           ((XAuint32) 0x00000000)
#define XA_RENDERINGHINT_ANTIALIASING   ((XAuint32) 0x00000001)

typedef struct XARectangle_ {
    XAuint32 left;
    XAuint32 top;
    XAuint32 width;
    XAuint32 height;
} XARectangle;

XA_API extern const XAInterfaceID XA_IID_VIDEOPOSTPROCESSING;

struct XAVideoPostProcessingItf_;
typedef const struct XAVideoPostProcessingItf_ * const * XAVideoPostProcessingItf;

struct XAVideoPostProcessingItf_ {
    XAresult (*SetRotation) (
        XAVideoPostProcessingItf self,
        XAmillidegree rotation
    );
    XAresult (*IsArbitraryRotationSupported) (
        XAVideoPostProcessingItf self,
        XAboolean *pSupported
    );
    XAresult (*SetScaleOptions) (
        XAVideoPostProcessingItf self,
        XAuint32 scaleOptions,
        XAuint32 backgroundColor,
        XAuint32 renderingHints
    );
    XAresult (*SetSourceRectangle) (
        XAVideoPostProcessingItf self,
        const XARectangle *pSrcRect
    );
    XAresult (*SetDestinationRectangle) (
        XAVideoPostProcessingItf self,
        const XARectangle *pDestRect
    );
    XAresult (*SetMirror) (
        XAVideoPostProcessingItf self,
        XAuint32 mirror
    );
    XAresult (*Commit) (
        XAVideoPostProcessingItf self
    );
};



  /*****************************************************************/
  /* CAPTURING INTERFACES, STRUCTS AND DEFINES                     */
  /*****************************************************************/

/* RECORD */

#define XA_RECORDEVENT_HEADATLIMIT          ((XAuint32) 0x00000001)
#define XA_RECORDEVENT_HEADATMARKER         ((XAuint32) 0x00000002)
#define XA_RECORDEVENT_HEADATNEWPOS         ((XAuint32) 0x00000004)
#define XA_RECORDEVENT_HEADMOVING           ((XAuint32) 0x00000008)
#define XA_RECORDEVENT_HEADSTALLED          ((XAuint32) 0x00000010)
#define XA_RECORDEVENT_BUFFER_FULL          ((XAuint32) 0x00000020)

#define XA_RECORDSTATE_STOPPED          ((XAuint32) 0x00000001)
#define XA_RECORDSTATE_PAUSED           ((XAuint32) 0x00000002)
#define XA_RECORDSTATE_RECORDING        ((XAuint32) 0x00000003)

XA_API extern const XAInterfaceID XA_IID_RECORD;

struct XARecordItf_;
typedef const struct XARecordItf_ * const * XARecordItf;

typedef void (XAAPIENTRY * xaRecordCallback) (
    XARecordItf caller,
    void * pContext,
    XAuint32 event
);

struct XARecordItf_ {
    XAresult (*SetRecordState) (
        XARecordItf self,
        XAuint32 state
    );
    XAresult (*GetRecordState) (
        XARecordItf self,
        XAuint32 * pState
    );
    XAresult (*SetDurationLimit) (
        XARecordItf self,
        XAmillisecond msec
    );
    XAresult (*GetPosition) (
        XARecordItf self,
        XAmillisecond * pMsec
    );
    XAresult (*RegisterCallback) (
        XARecordItf self,
        xaRecordCallback callback,
        void * pContext
    );
    XAresult (*SetCallbackEventsMask) (
        XARecordItf self,
        XAuint32 eventFlags
    );
    XAresult (*GetCallbackEventsMask) (
        XARecordItf self,
        XAuint32 * pEventFlags
    );
    XAresult (*SetMarkerPosition) (
        XARecordItf self,
        XAmillisecond mSec
    );
    XAresult (*ClearMarkerPosition) (
        XARecordItf self
    );
    XAresult (*GetMarkerPosition) (
        XARecordItf self,
        XAmillisecond * pMsec
    );
    XAresult (*SetPositionUpdatePeriod) (
        XARecordItf self,
        XAmillisecond mSec
    );
    XAresult (*GetPositionUpdatePeriod) (
        XARecordItf self,
        XAmillisecond * pMsec
    );
};

/* SNAPSHOT */

XA_API extern const XAInterfaceID XA_IID_SNAPSHOT;

struct XASnapshotItf_;
typedef const struct XASnapshotItf_ * const * XASnapshotItf;

typedef void (XAAPIENTRY * xaSnapshotInitiatedCallback) (
    XASnapshotItf caller,
    void * context
);

typedef void (XAAPIENTRY * xaSnapshotTakenCallback) (
    XASnapshotItf caller,
    void * context,
    XAuint32 numberOfPicsTaken,
    const XADataSink * image
);

struct XASnapshotItf_ {
    XAresult (*InitiateSnapshot) (
        XASnapshotItf self,
        XAuint32 numberOfPictures,
        XAuint32 fps,
        XAboolean freezeViewFinder,
        XADataSink sink,
        xaSnapshotInitiatedCallback initiatedCallback,
        xaSnapshotTakenCallback takenCallback,
        void * pContext
    );
    XAresult (*TakeSnapshot) (
        XASnapshotItf self
    );
    XAresult (*CancelSnapshot) (
        XASnapshotItf self
    );
    XAresult (*ReleaseBuffers) (
        XASnapshotItf self,
        XADataSink * image
    );
    XAresult (*GetMaxPicsPerBurst) (
        XASnapshotItf self,
        XAuint32 * maxNumberOfPictures
    );
    XAresult (*GetBurstFPSRange) (
        XASnapshotItf self,
        XAuint32 * minFPS,
        XAuint32 * maxFPS
    );
    XAresult (*SetShutterFeedback) (
        XASnapshotItf self,
        XAboolean enabled
    );
    XAresult (*GetShutterFeedback) (
        XASnapshotItf self,
        XAboolean * enabled
    );
};



  /*****************************************************************/
  /* METADATA RELATED INTERFACES, STRUCTS AND DEFINES              */
  /*****************************************************************/

/* METADATA (EXTRACTION, INSERTION, TRAVERSAL) */

#define XA_NODE_PARENT                  ((XAuint32) 0xFFFFFFFF)

#define XA_ROOT_NODE_ID                 ((XAint32) 0x7FFFFFFF)

#define XA_NODETYPE_UNSPECIFIED         ((XAuint32) 0x00000001)
#define XA_NODETYPE_AUDIO               ((XAuint32) 0x00000002)
#define XA_NODETYPE_VIDEO               ((XAuint32) 0x00000003)
#define XA_NODETYPE_IMAGE               ((XAuint32) 0x00000004)

#define XA_CHARACTERENCODING_UNKNOWN            ((XAuint32) 0x00000000)
#define XA_CHARACTERENCODING_BINARY             ((XAuint32) 0x00000001)
#define XA_CHARACTERENCODING_ASCII              ((XAuint32) 0x00000002)
#define XA_CHARACTERENCODING_BIG5               ((XAuint32) 0x00000003)
#define XA_CHARACTERENCODING_CODEPAGE1252       ((XAuint32) 0x00000004)
#define XA_CHARACTERENCODING_GB2312             ((XAuint32) 0x00000005)
#define XA_CHARACTERENCODING_HZGB2312           ((XAuint32) 0x00000006)
#define XA_CHARACTERENCODING_GB12345            ((XAuint32) 0x00000007)
#define XA_CHARACTERENCODING_GB18030            ((XAuint32) 0x00000008)
#define XA_CHARACTERENCODING_GBK                ((XAuint32) 0x00000009)
#define XA_CHARACTERENCODING_IMAPUTF7           ((XAuint32) 0x0000000A)
#define XA_CHARACTERENCODING_ISO2022JP          ((XAuint32) 0x0000000B)
#define XA_CHARACTERENCODING_ISO2022JP1         ((XAuint32) 0x0000000B)
#define XA_CHARACTERENCODING_ISO88591           ((XAuint32) 0x0000000C)
#define XA_CHARACTERENCODING_ISO885910          ((XAuint32) 0x0000000D)
#define XA_CHARACTERENCODING_ISO885913          ((XAuint32) 0x0000000E)
#define XA_CHARACTERENCODING_ISO885914          ((XAuint32) 0x0000000F)
#define XA_CHARACTERENCODING_ISO885915          ((XAuint32) 0x00000010)
#define XA_CHARACTERENCODING_ISO88592           ((XAuint32) 0x00000011)
#define XA_CHARACTERENCODING_ISO88593           ((XAuint32) 0x00000012)
#define XA_CHARACTERENCODING_ISO88594           ((XAuint32) 0x00000013)
#define XA_CHARACTERENCODING_ISO88595           ((XAuint32) 0x00000014)
#define XA_CHARACTERENCODING_ISO88596           ((XAuint32) 0x00000015)
#define XA_CHARACTERENCODING_ISO88597           ((XAuint32) 0x00000016)
#define XA_CHARACTERENCODING_ISO88598           ((XAuint32) 0x00000017)
#define XA_CHARACTERENCODING_ISO88599           ((XAuint32) 0x00000018)
#define XA_CHARACTERENCODING_ISOEUCJP           ((XAuint32) 0x00000019)
#define XA_CHARACTERENCODING_SHIFTJIS           ((XAuint32) 0x0000001A)
#define XA_CHARACTERENCODING_SMS7BIT            ((XAuint32) 0x0000001B)
#define XA_CHARACTERENCODING_UTF7               ((XAuint32) 0x0000001C)
#define XA_CHARACTERENCODING_UTF8               ((XAuint32) 0x0000001D)
#define XA_CHARACTERENCODING_JAVACONFORMANTUTF8 ((XAuint32) 0x0000001E)
#define XA_CHARACTERENCODING_UTF16BE            ((XAuint32) 0x0000001F)
#define XA_CHARACTERENCODING_UTF16LE            ((XAuint32) 0x00000020)

#define XA_METADATA_FILTER_KEY          ((XAuint8) 0x01)
#define XA_METADATA_FILTER_LANG         ((XAuint8) 0x02)
#define XA_METADATA_FILTER_ENCODING     ((XAuint8) 0x04)

#define XA_METADATATRAVERSALMODE_ALL    ((XAuint32) 0x00000001)
#define XA_METADATATRAVERSALMODE_NODE   ((XAuint32) 0x00000002)

#ifndef _KHRONOS_KEYS_
#define _KHRONOS_KEYS_
#define KHRONOS_TITLE                   "KhronosTitle"
#define KHRONOS_ALBUM                   "KhronosAlbum"
#define KHRONOS_TRACK_NUMBER            "KhronosTrackNumber"
#define KHRONOS_ARTIST                  "KhronosArtist"
#define KHRONOS_GENRE                   "KhronosGenre"
#define KHRONOS_YEAR                    "KhronosYear"
#define KHRONOS_COMMENT                 "KhronosComment"
#define KHRONOS_ARTIST_URL              "KhronosArtistURL"
#define KHRONOS_CONTENT_URL             "KhronosContentURL"
#define KHRONOS_RATING                  "KhronosRating"
#define KHRONOS_ALBUM_ART               "KhronosAlbumArt"
#define KHRONOS_COPYRIGHT               "KhronosCopyright"
#endif /* _KHRONOS_KEYS_ */


typedef struct XAMetadataInfo_ {
    XAuint32 size;
    XAuint32 encoding;
    const XAchar langCountry[16];
    XAuint8 data[1];
} XAMetadataInfo;

XA_API extern const XAInterfaceID XA_IID_METADATAEXTRACTION;

struct XAMetadataExtractionItf_;
typedef const struct XAMetadataExtractionItf_
    * const * XAMetadataExtractionItf;

struct XAMetadataExtractionItf_ {
    XAresult (*GetItemCount) (
        XAMetadataExtractionItf self,
        XAuint32 * pItemCount
    );
    XAresult (*GetKeySize) (
        XAMetadataExtractionItf self,
        XAuint32 index,
        XAuint32 * pKeySize
    );
    XAresult (*GetKey) (
        XAMetadataExtractionItf self,
        XAuint32 index,
        XAuint32 keySize,
        XAMetadataInfo * pKey
    );
    XAresult (*GetValueSize) (
        XAMetadataExtractionItf self,
        XAuint32 index,
        XAuint32 * pValueSize
    );
    XAresult (*GetValue) (
        XAMetadataExtractionItf self,
        XAuint32 index,
        XAuint32 valueSize,
        XAMetadataInfo * pValue
    );
    XAresult (*AddKeyFilter) (
        XAMetadataExtractionItf self,
        XAuint32 keySize,
        const void * pKey,
        XAuint32 keyEncoding,
        const XAchar * pValueLangCountry,
        XAuint32 valueEncoding,
        XAuint8 filterMask
    );
    XAresult (*ClearKeyFilter) (
        XAMetadataExtractionItf self
    );
};


XA_API extern const XAInterfaceID XA_IID_METADATAINSERTION;

struct XAMetadataInsertionItf_;
typedef const struct XAMetadataInsertionItf_
    * const * XAMetadataInsertionItf;

typedef void (XAAPIENTRY * xaMetadataInsertionCallback) (
    XAMetadataInsertionItf caller,
    void * pContext,
    XAMetadataInfo * pKey,
    XAMetadataInfo * pValue,
    XAint32 nodeID,
    XAboolean result
);

struct XAMetadataInsertionItf_ {
    XAresult (*CreateChildNode) (
        XAMetadataInsertionItf self,
        XAint32 parentNodeID,
        XAuint32 type,
        XAchar * mimeType,
        XAint32 * pChildNodeID
    );
    XAresult (*GetSupportedKeysCount) (
        XAMetadataInsertionItf self,
        XAint32 nodeID,
        XAboolean * pFreeKeys,
        XAuint32 * pKeyCount,
        XAuint32 * pEncodingCount
    );
    XAresult (*GetKeySize) (
        XAMetadataInsertionItf self,
        XAint32 nodeID,
        XAuint32 keyIndex,
        XAuint32 * pKeySize
    );
    XAresult (*GetKey) (
        XAMetadataInsertionItf self,
        XAint32 nodeID,
        XAuint32 keyIndex,
        XAuint32 keySize,
        XAMetadataInfo * pKey
    );
    XAresult (*GetFreeKeysEncoding) (
        XAMetadataInsertionItf self,
        XAint32 nodeID,
        XAuint32 encodingIndex,
        XAuint32 * pEncoding
    );
    XAresult (*InsertMetadataItem) (
        XAMetadataInsertionItf self,
        XAint32 nodeID,
        XAMetadataInfo * pKey,
        XAMetadataInfo * pValue,
        XAboolean overwrite
    );
    XAresult (*RegisterCallback) (
        XAMetadataInsertionItf self,
        xaMetadataInsertionCallback callback,
        void * pContext
    );
};


XA_API extern const XAInterfaceID XA_IID_METADATATRAVERSAL;

struct XAMetadataTraversalItf_;
typedef const struct XAMetadataTraversalItf_
    *  const *  XAMetadataTraversalItf;

struct XAMetadataTraversalItf_ {
    XAresult (*SetMode) (
        XAMetadataTraversalItf self,
        XAuint32 mode
    );
    XAresult (*GetChildCount) (
        XAMetadataTraversalItf self,
        XAuint32 * pCount
    );
    XAresult (*GetChildMIMETypeSize) (
        XAMetadataTraversalItf self,
        XAuint32 index,
        XAuint32 * pSize
    );
    XAresult (*GetChildInfo) (
        XAMetadataTraversalItf self,
        XAuint32 index,
        XAint32 * pNodeID,
        XAuint32 * pType,
        XAuint32 size,
        XAchar * pMimeType
    );
    XAresult (*SetActiveNode) (
        XAMetadataTraversalItf self,
        XAuint32 index
    );
};

/* DYNAMIC SOURCE */

XA_API extern const XAInterfaceID XA_IID_DYNAMICSOURCE;

struct XADynamicSourceItf_;
typedef const struct XADynamicSourceItf_ * const * XADynamicSourceItf;

struct XADynamicSourceItf_ {
    XAresult (*SetSource) (
        XADynamicSourceItf self,
        XADataSource * pDataSource
    );
};



  /*****************************************************************/
  /*  I/O DEVICES RELATED INTERFACES, STRUCTS AND DEFINES          */
  /*****************************************************************/

/* CAMERA AND CAMERA CAPABILITIES */

#define XA_CAMERA_APERTUREMODE_MANUAL               ((XAuint32) 0x00000001)
#define XA_CAMERA_APERTUREMODE_AUTO                 ((XAuint32) 0x00000002)

#define XA_CAMERA_AUTOEXPOSURESTATUS_SUCCESS        ((XAuint32) 0x00000001)
#define XA_CAMERA_AUTOEXPOSURESTATUS_UNDEREXPOSURE  ((XAuint32) 0x00000002)
#define XA_CAMERA_AUTOEXPOSURESTATUS_OVEREXPOSURE   ((XAuint32) 0x00000003)

#define XA_CAMERACBEVENT_ROTATION                   ((XAuint32) 0x00000001)
#define XA_CAMERACBEVENT_FLASHREADY                 ((XAuint32) 0x00000002)
#define XA_CAMERACBEVENT_FOCUSSTATUS                ((XAuint32) 0x00000003)
#define XA_CAMERACBEVENT_EXPOSURESTATUS             ((XAuint32) 0x00000004)
#define XA_CAMERACBEVENT_WHITEBALANCELOCKED         ((XAuint32) 0x00000005)
#define XA_CAMERACBEVENT_ZOOMSTATUS                 ((XAuint32) 0x00000006)

#define XA_CAMERACAP_FLASH                          ((XAuint32) 0x00000001)
#define XA_CAMERACAP_AUTOFOCUS                      ((XAuint32) 0x00000002)
#define XA_CAMERACAP_CONTINUOUSAUTOFOCUS            ((XAuint32) 0x00000004)
#define XA_CAMERACAP_MANUALFOCUS                    ((XAuint32) 0x00000008)
#define XA_CAMERACAP_AUTOEXPOSURE                   ((XAuint32) 0x00000010)
#define XA_CAMERACAP_MANUALEXPOSURE                 ((XAuint32) 0x00000020)
#define XA_CAMERACAP_AUTOISOSENSITIVITY             ((XAuint32) 0x00000040)
#define XA_CAMERACAP_MANUALISOSENSITIVITY           ((XAuint32) 0x00000080)
#define XA_CAMERACAP_AUTOAPERTURE                   ((XAuint32) 0x00000100)
#define XA_CAMERACAP_MANUALAPERTURE                 ((XAuint32) 0x00000200)
#define XA_CAMERACAP_AUTOSHUTTERSPEED               ((XAuint32) 0x00000400)
#define XA_CAMERACAP_MANUALSHUTTERSPEED             ((XAuint32) 0x00000800)
#define XA_CAMERACAP_AUTOWHITEBALANCE               ((XAuint32) 0x00001000)
#define XA_CAMERACAP_MANUALWHITEBALANCE             ((XAuint32) 0x00002000)
#define XA_CAMERACAP_OPTICALZOOM                    ((XAuint32) 0x00004000)
#define XA_CAMERACAP_DIGITALZOOM                    ((XAuint32) 0x00008000)
#define XA_CAMERACAP_METERING                       ((XAuint32) 0x00010000)
#define XA_CAMERACAP_BRIGHTNESS                     ((XAuint32) 0x00020000)
#define XA_CAMERACAP_CONTRAST                       ((XAuint32) 0x00040000)
#define XA_CAMERACAP_GAMMA                          ((XAuint32) 0x00080000)


#define XA_CAMERA_EXPOSUREMODE_MANUAL               ((XAuint32) 0x00000001)
#define XA_CAMERA_EXPOSUREMODE_AUTO                 ((XAuint32) 0x00000002)
#define XA_CAMERA_EXPOSUREMODE_NIGHT                ((XAuint32) 0x00000004)
#define XA_CAMERA_EXPOSUREMODE_BACKLIGHT            ((XAuint32) 0x00000008)
#define XA_CAMERA_EXPOSUREMODE_SPOTLIGHT            ((XAuint32) 0x00000010)
#define XA_CAMERA_EXPOSUREMODE_SPORTS               ((XAuint32) 0x00000020)
#define XA_CAMERA_EXPOSUREMODE_SNOW                 ((XAuint32) 0x00000040)
#define XA_CAMERA_EXPOSUREMODE_BEACH                ((XAuint32) 0x00000080)
#define XA_CAMERA_EXPOSUREMODE_LARGEAPERTURE        ((XAuint32) 0x00000100)
#define XA_CAMERA_EXPOSUREMODE_SMALLAPERTURE        ((XAuint32) 0x00000200)
#define XA_CAMERA_EXPOSUREMODE_PORTRAIT             ((XAuint32) 0x0000400)
#define XA_CAMERA_EXPOSUREMODE_NIGHTPORTRAIT        ((XAuint32) 0x00000800)

#define XA_CAMERA_FLASHMODE_OFF                     ((XAuint32) 0x00000001)
#define XA_CAMERA_FLASHMODE_ON                      ((XAuint32) 0x00000002)
#define XA_CAMERA_FLASHMODE_AUTO                    ((XAuint32) 0x00000004)
#define XA_CAMERA_FLASHMODE_REDEYEREDUCTION         ((XAuint32) 0x00000008)
#define XA_CAMERA_FLASHMODE_REDEYEREDUCTION_AUTO    ((XAuint32) 0x00000010)
#define XA_CAMERA_FLASHMODE_FILLIN                  ((XAuint32) 0x00000020)
#define XA_CAMERA_FLASHMODE_TORCH                   ((XAuint32) 0x00000040)

#define XA_CAMERA_FOCUSMODE_MANUAL                  ((XAuint32) 0x00000001)
#define XA_CAMERA_FOCUSMODE_AUTO                    ((XAuint32) 0x00000002)
#define XA_CAMERA_FOCUSMODE_CENTROID                ((XAuint32) 0x00000004)
#define XA_CAMERA_FOCUSMODE_CONTINUOUS_AUTO         ((XAuint32) 0x00000008)
#define XA_CAMERA_FOCUSMODE_CONTINUOUS_CENTROID     ((XAuint32) 0x00000010)

#define XA_CAMERA_FOCUSMODESTATUS_OFF               ((XAuint32) 0x00000001)
#define XA_CAMERA_FOCUSMODESTATUS_REQUEST           ((XAuint32) 0x00000002)
#define XA_CAMERA_FOCUSMODESTATUS_REACHED           ((XAuint32) 0x00000003)
#define XA_CAMERA_FOCUSMODESTATUS_UNABLETOREACH     ((XAuint32) 0x00000004)
#define XA_CAMERA_FOCUSMODESTATUS_LOST              ((XAuint32) 0x00000005)

#define XA_CAMERA_ISOSENSITIVITYMODE_MANUAL         ((XAuint32) 0x00000001)
#define XA_CAMERA_ISOSENSITIVITYMODE_AUTO           ((XAuint32) 0x00000002)

#define XA_CAMERA_LOCK_AUTOFOCUS                    ((XAuint32) 0x00000001)
#define XA_CAMERA_LOCK_AUTOEXPOSURE                 ((XAuint32) 0x00000002)
#define XA_CAMERA_LOCK_AUTOWHITEBALANCE             ((XAuint32) 0x00000004)

#define XA_CAMERA_METERINGMODE_AVERAGE              ((XAuint32) 0x00000001)
#define XA_CAMERA_METERINGMODE_SPOT                 ((XAuint32) 0x00000002)
#define XA_CAMERA_METERINGMODE_MATRIX               ((XAuint32) 0x00000004)

#define XA_CAMERA_SHUTTERSPEEDMODE_MANUAL           ((XAuint32) 0x00000001)
#define XA_CAMERA_SHUTTERSPEEDMODE_AUTO             ((XAuint32) 0x00000002)

#define XA_CAMERA_WHITEBALANCEMODE_MANUAL           ((XAuint32) 0x00000001)
#define XA_CAMERA_WHITEBALANCEMODE_AUTO             ((XAuint32) 0x00000002)
#define XA_CAMERA_WHITEBALANCEMODE_SUNLIGHT         ((XAuint32) 0x00000004)
#define XA_CAMERA_WHITEBALANCEMODE_CLOUDY           ((XAuint32) 0x00000008)
#define XA_CAMERA_WHITEBALANCEMODE_SHADE            ((XAuint32) 0x00000010)
#define XA_CAMERA_WHITEBALANCEMODE_TUNGSTEN         ((XAuint32) 0x00000020)
#define XA_CAMERA_WHITEBALANCEMODE_FLUORESCENT      ((XAuint32) 0x00000040)
#define XA_CAMERA_WHITEBALANCEMODE_INCANDESCENT     ((XAuint32) 0x00000080)
#define XA_CAMERA_WHITEBALANCEMODE_FLASH            ((XAuint32) 0x00000100)
#define XA_CAMERA_WHITEBALANCEMODE_SUNSET           ((XAuint32) 0x00000200)

#define XA_CAMERA_ZOOM_SLOW                         ((XAuint32) 50)
#define XA_CAMERA_ZOOM_NORMAL                       ((XAuint32) 100)
#define XA_CAMERA_ZOOM_FAST                         ((XAuint32) 200)
#define XA_CAMERA_ZOOM_FASTEST                      ((XAuint32) 0xFFFFFFFF)

#define XA_FOCUSPOINTS_ONE                          ((XAuint32) 0x00000001)
#define XA_FOCUSPOINTS_THREE_3X1                    ((XAuint32) 0x00000002)
#define XA_FOCUSPOINTS_FIVE_CROSS                   ((XAuint32) 0x00000003)
#define XA_FOCUSPOINTS_SEVEN_CROSS                  ((XAuint32) 0x00000004)
#define XA_FOCUSPOINTS_NINE_SQUARE                  ((XAuint32) 0x00000005)
#define XA_FOCUSPOINTS_ELEVEN_CROSS                 ((XAuint32) 0x00000006)
#define XA_FOCUSPOINTS_TWELVE_3X4                   ((XAuint32) 0x00000007)
#define XA_FOCUSPOINTS_TWELVE_4X3                   ((XAuint32) 0x00000008)
#define XA_FOCUSPOINTS_SIXTEEN_SQUARE               ((XAuint32) 0x00000009)
#define XA_FOCUSPOINTS_CUSTOM                       ((XAuint32) 0x0000000A)

typedef struct XAFocusPointPosition_ {
    XAuint32 left;
    XAuint32 top;
    XAuint32 width;
    XAuint32 height;
} XAFocusPointPosition;

#define XA_ORIENTATION_UNKNOWN                      ((XAuint32) 0x00000001)
#define XA_ORIENTATION_OUTWARDS                     ((XAuint32) 0x00000002)
#define XA_ORIENTATION_INWARDS                      ((XAuint32) 0x00000003)

typedef struct XACameraDescriptor_ {
    XAchar * name;
    XAuint32 maxWidth;
    XAuint32 maxHeight;
    XAuint32 orientation;
    XAuint32 featuresSupported;
    XAuint32 exposureModesSupported;
    XAuint32 flashModesSupported;
    XAuint32 focusModesSupported;
    XAuint32 meteringModesSupported;
    XAuint32 whiteBalanceModesSupported;
} XACameraDescriptor;

XA_API extern const XAInterfaceID XA_IID_CAMERACAPABILITIES;

struct XACameraCapabilitiesItf_;
typedef const struct XACameraCapabilitiesItf_
    * const * XACameraCapabilitiesItf;

struct XACameraCapabilitiesItf_ {
    XAresult (*GetCameraCapabilities) (
        XACameraCapabilitiesItf self,
        XAuint32 *pIndex,
        XAuint32 * pCameraDeviceID,
        XACameraDescriptor * pDescriptor
    );
    XAresult (*QueryFocusRegionPatterns) (
        XACameraCapabilitiesItf self,
        XAuint32 cameraDeviceID,
        XAuint32 * pPatternID,
        XAuint32 * pFocusPattern,
        XAuint32 * pCustomPoints1,
        XAuint32 * pCustomPoints2
    );
    XAresult (*GetSupportedAutoLocks) (
        XACameraCapabilitiesItf self,
        XAuint32 cameraDeviceID,
        XAuint32 * pNumCombinations,
        XAuint32 ** ppLocks
    );
    XAresult (*GetSupportedFocusManualSettings) (
        XACameraCapabilitiesItf self,
        XAuint32 cameraDeviceID,
        XAboolean macroEnabled,
        XAmillimeter * pMinValue,
        XAmillimeter * pMaxValue,
        XAuint32 * pNumSettings,
        XAmillimeter ** ppSettings
    );
    XAresult (*GetSupportedISOSensitivitySettings) (
        XACameraCapabilitiesItf self,
        XAuint32 cameraDeviceID,
        XAuint32 * pMinValue,
        XAuint32 * pMaxValue,
        XAuint32 * pNumSettings,
        XAuint32 ** ppSettings
    );
    XAresult (*GetSupportedApertureManualSettings) (
        XACameraCapabilitiesItf self,
        XAuint32 cameraDeviceID,
        XAuint32 * pMinValue,
        XAuint32 * pMaxValue,
        XAuint32 * pNumSettings,
        XAuint32 ** ppSettings
    );
    XAresult (*GetSupportedShutterSpeedManualSettings) (
        XACameraCapabilitiesItf self,
        XAuint32 cameraDeviceID,
        XAmicrosecond * pMinValue,
        XAmicrosecond * pMaxValue,
        XAuint32 * pNumSettings,
        XAmicrosecond ** ppSettings
    );
    XAresult (*GetSupportedWhiteBalanceManualSettings) (
        XACameraCapabilitiesItf self,
        XAuint32 cameraDeviceID,
        XAuint32 * pMinValue,
        XAuint32 * pMaxValue,
        XAuint32 * pNumSettings,
        XAuint32 ** ppSettings
    );
    XAresult (*GetSupportedZoomSettings) (
        XACameraCapabilitiesItf self,
        XAuint32 cameraDeviceID,
        XAboolean digitalEnabled,
        XAboolean macroEnabled,
        XApermille * pMaxValue,
        XAuint32 * pNumSettings,
        XApermille ** ppSettings,
        XAboolean * pSpeedSupported

    );
};

XA_API extern const XAInterfaceID XA_IID_CAMERA;

struct XACameraItf_;
typedef const struct XACameraItf_ * const * XACameraItf;

typedef void (XAAPIENTRY * xaCameraCallback) (
    XACameraItf caller,
    void * pContext,
    XAuint32 eventId,
    XAuint32 eventData
);

struct XACameraItf_ {
    XAresult (*RegisterCallback) (
        XACameraItf self,
        xaCameraCallback callback,
        void * pContext
    );
    XAresult (*SetFlashMode) (
        XACameraItf self,
        XAuint32 flashMode
    );
    XAresult (*GetFlashMode) (
        XACameraItf self,
        XAuint32 * pFlashMode
    );
    XAresult (*IsFlashReady) (
        XACameraItf self,
        XAboolean * pReady
    );
    XAresult (*SetFocusMode) (
        XACameraItf self,
        XAuint32 focusMode,
        XAmillimeter manualSetting,
        XAboolean macroEnabled
    );
    XAresult (*GetFocusMode) (
        XACameraItf self,
        XAuint32 * pFocusMode,
        XAmillimeter * pManualSetting,
        XAboolean * pMacroEnabled
    );
    XAresult (*SetFocusRegionPattern) (
        XACameraItf self,
        XAuint32 focusPattern,
        XAuint32 activePoints1,
        XAuint32 activePoints2
    );
    XAresult (*GetFocusRegionPattern) (
        XACameraItf self,
        XAuint32 * pFocusPattern,
        XAuint32 * pActivePoints1,
        XAuint32 * pActivePoints2
    );
    XAresult (*GetFocusRegionPositions) (
        XACameraItf self,
        XAuint32 * pNumPositionEntries,
        XAFocusPointPosition * pFocusPosition
    );
    XAresult (*GetFocusModeStatus) (
        XACameraItf self,
        XAuint32 * pFocusStatus,
        XAuint32 * pRegionStatus1,
        XAuint32 * pRegionStatus2
    );
    XAresult (*SetMeteringMode) (
        XACameraItf self,
        XAuint32 meteringMode
    );
    XAresult (*GetMeteringMode) (
        XACameraItf self,
        XAuint32 * pMeteringMode
    );
    XAresult (*SetExposureMode) (
        XACameraItf self,
        XAuint32 exposure,
        XAuint32 compensation
    );
    XAresult (*GetExposureMode) (
        XACameraItf self,
        XAuint32 * pExposure,
        XAuint32 * pCompensation
    );
    XAresult (*SetISOSensitivity) (
        XACameraItf self,
        XAuint32 isoSensitivity,
        XAuint32 manualSetting
    );
    XAresult (*GetISOSensitivity) (
        XACameraItf self,
        XAuint32 * pIsoSensitivity,
        XAuint32 * pManualSetting
    );
    XAresult (*SetAperture) (
        XACameraItf self,
        XAuint32 aperture,
        XAuint32 manualSetting
    );
    XAresult (*GetAperture) (
        XACameraItf self,
        XAuint32 * pAperture,
        XAuint32 * pManualSetting
    );
    XAresult (*SetShutterSpeed) (
        XACameraItf self,
        XAuint32 shutterSpeed,
        XAmicrosecond manualSetting
    );
    XAresult (*GetShutterSpeed) (
        XACameraItf self,
        XAuint32 * pShutterSpeed,
        XAmicrosecond * pManualSetting
    );
    XAresult (*SetWhiteBalance) (
        XACameraItf self,
        XAuint32 whiteBalance,
        XAuint32 manualSetting
    );
    XAresult (*GetWhiteBalance) (
        XACameraItf self,
        XAuint32 * pWhiteBalance,
        XAuint32 * pManualSetting
    );
    XAresult (*SetAutoLocks) (
        XACameraItf self,
        XAuint32 locks
    );
    XAresult (*GetAutoLocks) (
        XACameraItf self,
        XAuint32 * locks
    );
    XAresult (*SetZoom) (
        XACameraItf self,
        XApermille zoom,
        XAboolean digitalEnabled,
        XAuint32 speed,
        XAboolean async
    );
    XAresult (*GetZoom) (
        XACameraItf self,
        XApermille * pZoom,
        XAboolean * pDigital
    );
};

/* AUDIO I/O DEVICE CAPABILITIES */

#define XA_DEVCONNECTION_INTEGRATED                 ((XAint16) 0x0001)
#define XA_DEVCONNECTION_ATTACHED_WIRED             ((XAint16) 0x0100)
#define XA_DEVCONNECTION_ATTACHED_WIRELESS          ((XAint16) 0x0200)
#define XA_DEVCONNECTION_NETWORK                    ((XAint16) 0x0400)

#define XA_DEVLOCATION_HANDSET                      ((XAint16) 0x0001)
#define XA_DEVLOCATION_HEADSET                      ((XAint16) 0x0002)
#define XA_DEVLOCATION_CARKIT                       ((XAint16) 0x0003)
#define XA_DEVLOCATION_DOCK                         ((XAint16) 0x0004)
#define XA_DEVLOCATION_REMOTE                       ((XAint16) 0x0005)

#define XA_DEVSCOPE_UNKNOWN                         ((XAint16) 0x0001)
#define XA_DEVSCOPE_ENVIRONMENT                     ((XAint16) 0x0002)
#define XA_DEVSCOPE_USER                            ((XAint16) 0x0003)

typedef struct XAAudioInputDescriptor_ {
    XAchar * deviceName;
    XAint16 deviceConnection;
    XAint16 deviceScope;
    XAint16 deviceLocation;
    XAboolean isForTelephony;
    XAmilliHertz minSampleRate;
    XAmilliHertz maxSampleRate;
    XAboolean isFreqRangeContinuous;
    XAmilliHertz * samplingRatesSupported;
    XAint16 numOfSamplingRatesSupported;
    XAint16 maxChannels;
} XAAudioInputDescriptor;

typedef struct XAAudioOutputDescriptor_ {
    XAchar *pDeviceName;
    XAint16 deviceConnection;
    XAint16 deviceScope;
    XAint16 deviceLocation;
    XAboolean isForTelephony;
    XAmilliHertz minSampleRate;
    XAmilliHertz maxSampleRate;
    XAboolean isFreqRangeContinuous;
    XAmilliHertz *samplingRatesSupported;
    XAint16 numOfSamplingRatesSupported;
    XAint16 maxChannels;
} XAAudioOutputDescriptor;

XA_API extern const XAInterfaceID XA_IID_AUDIOIODEVICECAPABILITIES;

struct XAAudioIODeviceCapabilitiesItf_;
typedef const struct XAAudioIODeviceCapabilitiesItf_
    * const * XAAudioIODeviceCapabilitiesItf;

typedef void (XAAPIENTRY * xaAvailableAudioInputsChangedCallback) (
    XAAudioIODeviceCapabilitiesItf caller,
    void * pContext,
    XAuint32 deviceID,
    XAint32 numInputs,
    XAboolean isNew
);

typedef void (XAAPIENTRY * xaAvailableAudioOutputsChangedCallback) (
    XAAudioIODeviceCapabilitiesItf caller,
    void * pContext,
    XAuint32 deviceID,
    XAint32 numOutputs,
    XAboolean isNew
);

typedef void (XAAPIENTRY * xaDefaultDeviceIDMapChangedCallback) (
    XAAudioIODeviceCapabilitiesItf caller,
    void * pContext,
	XAboolean isOutput,
    XAint32 numDevices
);

struct XAAudioIODeviceCapabilitiesItf_ {
    XAresult (*GetAvailableAudioInputs) (
        XAAudioIODeviceCapabilitiesItf self,
        XAint32 * pNumInputs,
        XAuint32 * pInputDeviceIDs
    );
    XAresult (*QueryAudioInputCapabilities) (
        XAAudioIODeviceCapabilitiesItf self,
        XAuint32 deviceID,
        XAAudioInputDescriptor * pDescriptor
    );
    XAresult (*RegisterAvailableAudioInputsChangedCallback) (
        XAAudioIODeviceCapabilitiesItf self,
        xaAvailableAudioInputsChangedCallback callback,
        void * pContext
    );
    XAresult (*GetAvailableAudioOutputs) (
        XAAudioIODeviceCapabilitiesItf self,
        XAint32 * pNumOutputs,
        XAuint32 * pOutputDeviceIDs
    );
    XAresult (*QueryAudioOutputCapabilities) (
        XAAudioIODeviceCapabilitiesItf self,
        XAuint32 deviceID,
        XAAudioOutputDescriptor * pDescriptor
    );
    XAresult (*RegisterAvailableAudioOutputsChangedCallback) (
        XAAudioIODeviceCapabilitiesItf self,
        xaAvailableAudioOutputsChangedCallback callback,
        void * pContext
    );
    XAresult (*RegisterDefaultDeviceIDMapChangedCallback) (
        XAAudioIODeviceCapabilitiesItf self,
        xaDefaultDeviceIDMapChangedCallback callback,
        void * pContext
    );
    XAresult (*GetAssociatedAudioInputs) (
        XAAudioIODeviceCapabilitiesItf self,
        XAuint32 deviceID,
        XAint32 * pNumAudioInputs,
        XAuint32 * pAudioInputDeviceIDs
    );
    XAresult (*GetAssociatedAudioOutputs) (
        XAAudioIODeviceCapabilitiesItf self,
        XAuint32 deviceID,
        XAint32 * pNumAudioOutputs,
        XAuint32 * pAudioOutputDeviceIDs
    );
    XAresult (*GetDefaultAudioDevices) (
        XAAudioIODeviceCapabilitiesItf self,
        XAuint32 defaultDeviceID,
        XAint32 *pNumAudioDevices,
        XAuint32 *pAudioDeviceIDs
    );
    XAresult (*QuerySampleFormatsSupported) (
        XAAudioIODeviceCapabilitiesItf self,
        XAuint32 deviceID,
        XAmilliHertz samplingRate,
        XAint32 *pSampleFormats,
        XAint32 *pNumOfSampleFormats
    );
};

/* DEVICE VOLUME */

XA_API extern const XAInterfaceID XA_IID_DEVICEVOLUME;

struct XADeviceVolumeItf_;
typedef const struct XADeviceVolumeItf_ * const * XADeviceVolumeItf;

struct XADeviceVolumeItf_ {
    XAresult (*GetVolumeScale) (
        XADeviceVolumeItf self,
        XAuint32 deviceID,
        XAint32 * pMinValue,
        XAint32 * pMaxValue,
        XAboolean * pIsMillibelScale
    );
    XAresult (*SetVolume) (
        XADeviceVolumeItf self,
        XAuint32 deviceID,
        XAint32 volume
    );
    XAresult (*GetVolume) (
        XADeviceVolumeItf self,
        XAuint32 deviceID,
        XAint32 * pVolume
    );
};

/* EQUALIZER */

#define XA_EQUALIZER_UNDEFINED    ((XAuint16) 0xFFFF)

XA_API extern const XAInterfaceID XA_IID_EQUALIZER;

struct XAEqualizerItf_;
typedef const struct XAEqualizerItf_ * const * XAEqualizerItf;

struct XAEqualizerItf_ {
    XAresult (*SetEnabled) (
        XAEqualizerItf self,
        XAboolean enabled
    );
    XAresult (*IsEnabled) (
        XAEqualizerItf self,
        XAboolean * pEnabled
    );
    XAresult (*GetNumberOfBands) (
        XAEqualizerItf self,
        XAuint16 * pNumBands
    );
    XAresult (*GetBandLevelRange) (
        XAEqualizerItf self,
        XAmillibel * pMin,
        XAmillibel * pMax
    );
    XAresult (*SetBandLevel) (
        XAEqualizerItf self,
        XAuint16 band,
        XAmillibel level
    );
    XAresult (*GetBandLevel) (
        XAEqualizerItf self,
        XAuint16 band,
        XAmillibel * pLevel
    );
    XAresult (*GetCenterFreq) (
        XAEqualizerItf self,
        XAuint16 band,
        XAmilliHertz * pCenter
    );
    XAresult (*GetBandFreqRange) (
        XAEqualizerItf self,
        XAuint16 band,
        XAmilliHertz * pMin,
        XAmilliHertz * pMax
    );
    XAresult (*GetBand) (
        XAEqualizerItf self,
        XAmilliHertz frequency,
        XAuint16 * pBand
    );
    XAresult (*GetCurrentPreset) (
        XAEqualizerItf self,
        XAuint16 * pPreset
    );
    XAresult (*UsePreset) (
        XAEqualizerItf self,
        XAuint16 index
    );
    XAresult (*GetNumberOfPresets) (
        XAEqualizerItf self,
        XAuint16 * pNumPresets
    );
    XAresult (*GetPresetName) (
        XAEqualizerItf self,
        XAuint16 index,
        const XAchar ** ppName
    );
};

/* OUTPUT MIX */

XA_API extern const XAInterfaceID XA_IID_OUTPUTMIX;

struct XAOutputMixItf_;
typedef const struct XAOutputMixItf_ * const * XAOutputMixItf;

typedef void (XAAPIENTRY * xaMixDeviceChangeCallback) (
    XAOutputMixItf caller,
    void * pContext
);

struct XAOutputMixItf_ {
    XAresult (*GetDestinationOutputDeviceIDs) (
        XAOutputMixItf self,
        XAint32 * pNumDevices,
        XAuint32 * pDeviceIDs
    );
    XAresult (*RegisterDeviceChangeCallback) (
        XAOutputMixItf self,
        xaMixDeviceChangeCallback callback,
        void * pContext
    );
    XAresult (*ReRoute) (
        XAOutputMixItf self,
        XAint32 numOutputDevices,
        XAuint32 * pOutputDeviceIDs
    );
};

/* RADIO */

#define XA_FREQRANGE_FMEUROAMERICA                  ((XAuint8) 0x01)
#define XA_FREQRANGE_FMJAPAN                        ((XAuint8) 0x02)
#define XA_FREQRANGE_AMLW                           ((XAuint8) 0x03)
#define XA_FREQRANGE_AMMW                           ((XAuint8) 0x04)
#define XA_FREQRANGE_AMSW                           ((XAuint8) 0x05)

#define XA_RADIO_EVENT_ANTENNA_STATUS_CHANGED       ((XAuint32) 0x00000001)
#define XA_RADIO_EVENT_FREQUENCY_CHANGED            ((XAuint32) 0x00000002)
#define XA_RADIO_EVENT_FREQUENCY_RANGE_CHANGED      ((XAuint32) 0x00000003)
#define XA_RADIO_EVENT_PRESET_CHANGED               ((XAuint32) 0x00000004)
#define XA_RADIO_EVENT_SEEK_COMPLETED               ((XAuint32) 0x00000005)

#define XA_STEREOMODE_MONO                          ((XAuint32) 0x00000000)
#define XA_STEREOMODE_STEREO                        ((XAuint32) 0x00000001)
#define XA_STEREOMODE_AUTO                          ((XAuint32) 0x00000002)

XA_API extern const XAInterfaceID XA_IID_RADIO;

struct XARadioItf_;
typedef const struct XARadioItf_ * const * XARadioItf;

typedef void (XAAPIENTRY * xaRadioCallback) (
    XARadioItf caller,
    void * pContext,
    XAuint32 event,
    XAuint32 eventIntData,
    XAboolean eventBooleanData
);

struct XARadioItf_ {
    XAresult (*SetFreqRange) (
        XARadioItf self,
        XAuint8 range
    );
    XAresult (*GetFreqRange) (
        XARadioItf self,
        XAuint8 * pRange
    );
    XAresult (*IsFreqRangeSupported) (
        XARadioItf self,
        XAuint8 range,
        XAboolean * pSupported
    );
    XAresult (*GetFreqRangeProperties) (
        XARadioItf self,
        XAuint8 range,
        XAuint32 * pMinFreq,
        XAuint32 * pMaxFreq,
        XAuint32 * pFreqInterval
    );
    XAresult (*SetFrequency) (
        XARadioItf self,
        XAuint32 freq
    );
    XAresult (*CancelSetFrequency) (
        XARadioItf self
    );
    XAresult (*GetFrequency) (
        XARadioItf self,
        XAuint32 * pFreq
    );
    XAresult (*SetSquelch) (
        XARadioItf self,
        XAboolean squelch
    );
    XAresult (*GetSquelch) (
        XARadioItf self,
        XAboolean * pSquelch
    );
    XAresult (*SetStereoMode) (
        XARadioItf self,
        XAuint32 mode
    );
    XAresult (*GetStereoMode) (
        XARadioItf self,
        XAuint32 * pMode
    );
    XAresult (*GetSignalStrength) (
        XARadioItf self,
        XAuint32 * pStrength
    );
    XAresult (*Seek) (
        XARadioItf self,
        XAboolean upwards
    );
    XAresult (*StopSeeking) (
        XARadioItf self
    );
    XAresult (*GetNumberOfPresets) (
        XARadioItf self,
        XAuint32 * pNumPresets
    );
    XAresult (*SetPreset) (
        XARadioItf self,
        XAuint32 preset,
        XAuint32 freq,
        XAuint8 range,
        XAuint32 mode,
        const XAchar * pName
    );
    XAresult (*GetPreset) (
        XARadioItf self,
        XAuint32 preset,
        XAuint32 * pFreq,
        XAuint8 * pRange,
        XAuint32 * pMode,
        XAchar * pName,
        XAuint16 * pNameLength
    );
    XAresult (*RegisterRadioCallback) (
        XARadioItf self,
        xaRadioCallback callback,
        void * pContext
    );
};

/* RDS */

#define XA_RDS_EVENT_NEW_PI                         ((XAuint16) 0x0001)
#define XA_RDS_EVENT_NEW_PTY                        ((XAuint16) 0x0002)
#define XA_RDS_EVENT_NEW_PS                         ((XAuint16) 0x0004)
#define XA_RDS_EVENT_NEW_RT                         ((XAuint16) 0x0008)
#define XA_RDS_EVENT_NEW_RT_PLUS                    ((XAuint16) 0x0010)
#define XA_RDS_EVENT_NEW_CT                         ((XAuint16) 0x0020)
#define XA_RDS_EVENT_NEW_TA                         ((XAuint16) 0x0040)
#define XA_RDS_EVENT_NEW_TP                         ((XAuint16) 0x0080)
#define XA_RDS_EVENT_NEW_ALARM                      ((XAuint16) 0x0100)

#define XA_RDSPROGRAMMETYPE_RDSPTY_NONE \
    ((XAuint32) 0x00000000)
#define XA_RDSPROGRAMMETYPE_RDSPTY_NEWS \
    ((XAuint32) 0x00000001)
#define XA_RDSPROGRAMMETYPE_RDSPTY_CURRENTAFFAIRS \
    ((XAuint32) 0x00000002)
#define XA_RDSPROGRAMMETYPE_RDSPTY_INFORMATION \
    ((XAuint32) 0x00000003)
#define XA_RDSPROGRAMMETYPE_RDSPTY_SPORT \
    ((XAuint32) 0x00000004)
#define XA_RDSPROGRAMMETYPE_RDSPTY_EDUCATION \
    ((XAuint32) 0x00000005)
#define XA_RDSPROGRAMMETYPE_RDSPTY_DRAMA \
    ((XAuint32) 0x00000006)
#define XA_RDSPROGRAMMETYPE_RDSPTY_CULTURE \
    ((XAuint32) 0x00000007)
#define XA_RDSPROGRAMMETYPE_RDSPTY_SCIENCE \
    ((XAuint32) 0x00000008)
#define XA_RDSPROGRAMMETYPE_RDSPTY_VARIEDSPEECH \
    ((XAuint32) 0x00000009)
#define XA_RDSPROGRAMMETYPE_RDSPTY_POPMUSIC \
    ((XAuint32) 0x0000000A)
#define XA_RDSPROGRAMMETYPE_RDSPTY_ROCKMUSIC \
    ((XAuint32) 0x0000000B)
#define XA_RDSPROGRAMMETYPE_RDSPTY_EASYLISTENING \
    ((XAuint32) 0x0000000C)
#define XA_RDSPROGRAMMETYPE_RDSPTY_LIGHTCLASSICAL \
    ((XAuint32) 0x0000000D)
#define XA_RDSPROGRAMMETYPE_RDSPTY_SERIOUSCLASSICAL \
    ((XAuint32) 0x0000000E)
#define XA_RDSPROGRAMMETYPE_RDSPTY_OTHERMUSIC \
    ((XAuint32) 0x0000000F)
#define XA_RDSPROGRAMMETYPE_RDSPTY_WEATHER \
    ((XAuint32) 0x00000010)
#define XA_RDSPROGRAMMETYPE_RDSPTY_FINANCE \
    ((XAuint32) 0x00000011)
#define XA_RDSPROGRAMMETYPE_RDSPTY_CHILDRENSPROGRAMMES \
    ((XAuint32) 0x00000012)
#define XA_RDSPROGRAMMETYPE_RDSPTY_SOCIALAFFAIRS \
    ((XAuint32) 0x00000013)
#define XA_RDSPROGRAMMETYPE_RDSPTY_RELIGION \
    ((XAuint32) 0x00000014)
#define XA_RDSPROGRAMMETYPE_RDSPTY_PHONEIN \
    ((XAuint32) 0x00000015)
#define XA_RDSPROGRAMMETYPE_RDSPTY_TRAVEL \
    ((XAuint32) 0x00000016)
#define XA_RDSPROGRAMMETYPE_RDSPTY_LEISURE \
    ((XAuint32) 0x00000017)
#define XA_RDSPROGRAMMETYPE_RDSPTY_JAZZMUSIC \
    ((XAuint32) 0x00000018)
#define XA_RDSPROGRAMMETYPE_RDSPTY_COUNTRYMUSIC \
    ((XAuint32) 0x00000019)
#define XA_RDSPROGRAMMETYPE_RDSPTY_NATIONALMUSIC \
    ((XAuint32) 0x0000001A)
#define XA_RDSPROGRAMMETYPE_RDSPTY_OLDIESMUSIC \
    ((XAuint32) 0x0000001B)
#define XA_RDSPROGRAMMETYPE_RDSPTY_FOLKMUSIC \
    ((XAuint32) 0x0000001C)
#define XA_RDSPROGRAMMETYPE_RDSPTY_DOCUMENTARY \
    ((XAuint32) 0x0000001D)
#define XA_RDSPROGRAMMETYPE_RDSPTY_ALARMTEST \
    ((XAuint32) 0x0000001E)
#define XA_RDSPROGRAMMETYPE_RDSPTY_ALARM \
    ((XAuint32) 0x0000001F)

#define XA_RDSPROGRAMMETYPE_RBDSPTY_NONE \
    ((XAuint32) 0x00000000)
#define XA_RDSPROGRAMMETYPE_RBDSPTY_NEWS \
    ((XAuint32) 0x00000001)
#define XA_RDSPROGRAMMETYPE_RBDSPTY_INFORMATION \
    ((XAuint32) 0x00000002)
#define XA_RDSPROGRAMMETYPE_RBDSPTY_SPORTS \
    ((XAuint32) 0x00000003)
#define XA_RDSPROGRAMMETYPE_RBDSPTY_TALK \
    ((XAuint32) 0x00000004)
#define XA_RDSPROGRAMMETYPE_RBDSPTY_ROCK \
    ((XAuint32) 0x00000005)
#define XA_RDSPROGRAMMETYPE_RBDSPTY_CLASSICROCK \
    ((XAuint32) 0x00000006)
#define XA_RDSPROGRAMMETYPE_RBDSPTY_ADULTHITS \
    ((XAuint32) 0x00000007)
#define XA_RDSPROGRAMMETYPE_RBDSPTY_SOFTROCK \
    ((XAuint32) 0x00000008)
#define XA_RDSPROGRAMMETYPE_RBDSPTY_TOP40 \
    ((XAuint32) 0x00000009)
#define XA_RDSPROGRAMMETYPE_RBDSPTY_COUNTRY \
    ((XAuint32) 0x0000000A)
#define XA_RDSPROGRAMMETYPE_RBDSPTY_OLDIES \
    ((XAuint32) 0x0000000B)
#define XA_RDSPROGRAMMETYPE_RBDSPTY_SOFT \
    ((XAuint32) 0x0000000C)
#define XA_RDSPROGRAMMETYPE_RBDSPTY_NOSTALGIA \
    ((XAuint32) 0x0000000D)
#define XA_RDSPROGRAMMETYPE_RBDSPTY_JAZZ \
    ((XAuint32) 0x0000000E)
#define XA_RDSPROGRAMMETYPE_RBDSPTY_CLASSICAL \
    ((XAuint32) 0x0000000F)
#define XA_RDSPROGRAMMETYPE_RBDSPTY_RHYTHMANDBLUES \
    ((XAuint32) 0x00000010)
#define XA_RDSPROGRAMMETYPE_RBDSPTY_SOFTRHYTHMANDBLUES \
    ((XAuint32) 0x00000011)
#define XA_RDSPROGRAMMETYPE_RBDSPTY_LANGUAGE \
    ((XAuint32) 0x00000012)
#define XA_RDSPROGRAMMETYPE_RBDSPTY_RELIGIOUSMUSIC \
    ((XAuint32) 0x00000013)
#define XA_RDSPROGRAMMETYPE_RBDSPTY_RELIGIOUSTALK \
    ((XAuint32) 0x00000014)
#define XA_RDSPROGRAMMETYPE_RBDSPTY_PERSONALITY \
    ((XAuint32) 0x00000015)
#define XA_RDSPROGRAMMETYPE_RBDSPTY_PUBLIC \
    ((XAuint32) 0x00000016)
#define XA_RDSPROGRAMMETYPE_RBDSPTY_COLLEGE \
    ((XAuint32) 0x00000017)
#define XA_RDSPROGRAMMETYPE_RBDSPTY_UNASSIGNED1 \
    ((XAuint32) 0x00000018)
#define XA_RDSPROGRAMMETYPE_RBDSPTY_UNASSIGNED2 \
    ((XAuint32) 0x00000019)
#define XA_RDSPROGRAMMETYPE_RBDSPTY_UNASSIGNED3 \
    ((XAuint32) 0x0000001A)
#define XA_RDSPROGRAMMETYPE_RBDSPTY_UNASSIGNED4 \
    ((XAuint32) 0x0000001B)
#define XA_RDSPROGRAMMETYPE_RBDSPTY_UNASSIGNED5 \
    ((XAuint32) 0x0000001C)
#define XA_RDSPROGRAMMETYPE_RBDSPTY_WEATHER \
    ((XAuint32) 0x0000001D)
#define XA_RDSPROGRAMMETYPE_RBDSPTY_EMERGENCYTEST \
    ((XAuint32) 0x0000001E)
#define XA_RDSPROGRAMMETYPE_RBDSPTY_EMERGENCY \
    ((XAuint32) 0x0000001F)

#define XA_RDSRTPLUS_ITEMTITLE              ((XAuint8) 0x01)
#define XA_RDSRTPLUS_ITEMALBUM              ((XAuint8) 0x02)
#define XA_RDSRTPLUS_ITEMTRACKNUMBER        ((XAuint8) 0x03)
#define XA_RDSRTPLUS_ITEMARTIST             ((XAuint8) 0x04)
#define XA_RDSRTPLUS_ITEMCOMPOSITION        ((XAuint8) 0x05)
#define XA_RDSRTPLUS_ITEMMOVEMENT           ((XAuint8) 0x06)
#define XA_RDSRTPLUS_ITEMCONDUCTOR          ((XAuint8) 0x07)
#define XA_RDSRTPLUS_ITEMCOMPOSER           ((XAuint8) 0x08)
#define XA_RDSRTPLUS_ITEMBAND               ((XAuint8) 0x09)
#define XA_RDSRTPLUS_ITEMCOMMENT            ((XAuint8) 0x0A)
#define XA_RDSRTPLUS_ITEMGENRE              ((XAuint8) 0x0B)
#define XA_RDSRTPLUS_INFONEWS               ((XAuint8) 0x0C)
#define XA_RDSRTPLUS_INFONEWSLOCAL          ((XAuint8) 0x0D)
#define XA_RDSRTPLUS_INFOSTOCKMARKET        ((XAuint8) 0x0E)
#define XA_RDSRTPLUS_INFOSPORT              ((XAuint8) 0x0F)
#define XA_RDSRTPLUS_INFOLOTTERY            ((XAuint8) 0x10)
#define XA_RDSRTPLUS_INFOHOROSCOPE          ((XAuint8) 0x11)
#define XA_RDSRTPLUS_INFODAILYDIVERSION     ((XAuint8) 0x12)
#define XA_RDSRTPLUS_INFOHEALTH             ((XAuint8) 0x13)
#define XA_RDSRTPLUS_INFOEVENT              ((XAuint8) 0x14)
#define XA_RDSRTPLUS_INFOSZENE              ((XAuint8) 0x15)
#define XA_RDSRTPLUS_INFOCINEMA             ((XAuint8) 0x16)
#define XA_RDSRTPLUS_INFOTV                 ((XAuint8) 0x17)
#define XA_RDSRTPLUS_INFODATETIME           ((XAuint8) 0x18)
#define XA_RDSRTPLUS_INFOWEATHER            ((XAuint8) 0x19)
#define XA_RDSRTPLUS_INFOTRAFFIC            ((XAuint8) 0x1A)
#define XA_RDSRTPLUS_INFOALARM              ((XAuint8) 0x1B)
#define XA_RDSRTPLUS_INFOADVISERTISEMENT    ((XAuint8) 0x1C)
#define XA_RDSRTPLUS_INFOURL                ((XAuint8) 0x1D)
#define XA_RDSRTPLUS_INFOOTHER              ((XAuint8) 0x1E)
#define XA_RDSRTPLUS_STATIONNAMESHORT       ((XAuint8) 0x1F)
#define XA_RDSRTPLUS_STATIONNAMELONG        ((XAuint8) 0x20)
#define XA_RDSRTPLUS_PROGRAMNOW             ((XAuint8) 0x21)
#define XA_RDSRTPLUS_PROGRAMNEXT            ((XAuint8) 0x22)
#define XA_RDSRTPLUS_PROGRAMPART            ((XAuint8) 0x23)
#define XA_RDSRTPLUS_PROGRAMHOST            ((XAuint8) 0x24)
#define XA_RDSRTPLUS_PROFRAMEDITORIALSTAFF  ((XAuint8) 0x25)
#define XA_RDSRTPLUS_PROGRAMFREQUENCY       ((XAuint8) 0x26)
#define XA_RDSRTPLUS_PROGRAMHOMEPAGE        ((XAuint8) 0x27)
#define XA_RDSRTPLUS_PROGRAMSUBCHANNEL      ((XAuint8) 0x28)
#define XA_RDSRTPLUS_PHONEHOTLINE           ((XAuint8) 0x29)
#define XA_RDSRTPLUS_PHONESTUDIO            ((XAuint8) 0x2A)
#define XA_RDSRTPLUS_PHONEOTHER             ((XAuint8) 0x2B)
#define XA_RDSRTPLUS_SMSSTUDIO              ((XAuint8) 0x2C)
#define XA_RDSRTPLUS_SMSOTHER               ((XAuint8) 0x2D)
#define XA_RDSRTPLUS_EMAILHOTLINE           ((XAuint8) 0x2E)
#define XA_RDSRTPLUS_EMAILSTUDIO            ((XAuint8) 0x2F)
#define XA_RDSRTPLUS_EMAILOTHER             ((XAuint8) 0x30)
#define XA_RDSRTPLUS_MMSOTHER               ((XAuint8) 0x31)
#define XA_RDSRTPLUS_CHAT                   ((XAuint8) 0x32)
#define XA_RDSRTPLUS_CHATCENTER             ((XAuint8) 0x33)
#define XA_RDSRTPLUS_VOTEQUESTION           ((XAuint8) 0x34)
#define XA_RDSRTPLUS_VOTECENTER             ((XAuint8) 0x35)
#define XA_RDSRTPLUS_OPENCLASS45            ((XAuint8) 0x36)
#define XA_RDSRTPLUS_OPENCLASS55            ((XAuint8) 0x37)
#define XA_RDSRTPLUS_OPENCLASS56            ((XAuint8) 0x38)
#define XA_RDSRTPLUS_OPENCLASS57            ((XAuint8) 0x39)
#define XA_RDSRTPLUS_OPENCLASS58            ((XAuint8) 0x3A)
#define XA_RDSRTPLUS_PLACE                  ((XAuint8) 0x3B)
#define XA_RDSRTPLUS_APPOINTMENT            ((XAuint8) 0x3C)
#define XA_RDSRTPLUS_IDENTIFIER             ((XAuint8) 0x3D)
#define XA_RDSRTPLUS_PURCHASE               ((XAuint8) 0x3E)
#define XA_RDSRTPLUS_GETDATA                ((XAuint8) 0x3F)

XA_API extern const XAInterfaceID XA_IID_RDS;

struct XARDSItf_;
typedef const struct XARDSItf_ * const * XARDSItf;

typedef void (XAAPIENTRY * xaGetODAGroupCallback) (
    XARadioItf caller,
    void * pContext,
    XAboolean success,
    XAint16 group,
    XAuint16 message
);

typedef void (XAAPIENTRY * xaNewODADataCallback) (
    XARDSItf caller,
    void * pContext,
    XAint16 group,
    XAuint64 data
);

typedef void (XAAPIENTRY * xaRDSCallback) (
    XARDSItf caller,
    void * pContext,
    XAuint16 event,
    XAuint8 eventData
);

struct XARDSItf_ {
    XAresult (*QueryRDSSignal) (
        XARDSItf self,
        XAboolean * isSignal
    );
    XAresult (*GetProgrammeServiceName) (
        XARDSItf self,
        XAchar * ps
    );
    XAresult (*GetRadioText) (
        XARDSItf self,
        XAchar * rt
    );
    XAresult (*GetRadioTextPlus) (
        XARDSItf self,
        XAuint8 contentType,
        XAchar * informationElement,
        XAchar * descriptor,
        XAuint8 * descriptorContentType
    );
    XAresult (*GetProgrammeType) (
        XARDSItf self,
        XAuint32 * pty
    );
    XAresult (*GetProgrammeTypeString) (
        XARDSItf self,
        XAboolean isLengthMax16,
        XAchar * pty
    );
    XAresult (*GetProgrammeIdentificationCode) (
        XARDSItf self,
        XAint16 * pi
    );
    XAresult (*GetClockTime) (
        XARDSItf self,
        XAtime * dateAndTime
    );
    XAresult (*GetTrafficAnnouncement) (
        XARDSItf self,
        XAboolean * ta
    );
    XAresult (*GetTrafficProgramme) (
        XARDSItf self,
        XAboolean * tp
    );
    XAresult (*SeekByProgrammeType) (
        XARDSItf self,
        XAuint32 pty,
        XAboolean upwards
    );
    XAresult (*SeekTrafficAnnouncement) (
        XARDSItf self,
        XAboolean upwards
    );
    XAresult (*SeekTrafficProgramme) (
        XARDSItf self,
        XAboolean upwards
    );
    XAresult (*SetAutomaticSwitching) (
        XARDSItf self,
        XAboolean automatic
    );
    XAresult (*GetAutomaticSwitching) (
        XARDSItf self,
        XAboolean * automatic
    );
    XAresult (*SetAutomaticTrafficAnnouncement) (
        XARDSItf self,
        XAboolean automatic
    );
    XAresult (*GetAutomaticTrafficAnnouncement) (
        XARDSItf self,
        XAboolean * automatic
    );
    XAresult (*GetODAGroup) (
        XARDSItf self,
        XAuint16 AID,
        xaGetODAGroupCallback callback,
        void * pContext
    );
    XAresult (*SubscribeODAGroup) (
        XARDSItf self,
        XAint16 group,
        XAboolean useErrorCorrection
    );
    XAresult (*UnsubscribeODAGroup) (
        XARDSItf self,
        XAint16 group
    );
    XAresult (*ListODAGroupSubscriptions) (
        XARDSItf self,
        XAint16* pGroups,
        XAuint32* pLength
    );
    XAresult (*RegisterRDSCallback) (
        XARDSItf self,
        xaRDSCallback callback,
        void * pContext
    );
    XAresult (*RegisterODADataCallback) (
        XARDSItf self,
        xaNewODADataCallback callback,
        void * pContext
    );
};

/* VIBRA */

XA_API extern const XAInterfaceID XA_IID_VIBRA;

struct XAVibraItf_;
typedef const struct XAVibraItf_ * const * XAVibraItf;

struct XAVibraItf_ {
    XAresult (*Vibrate) (
        XAVibraItf self,
        XAboolean vibrate
    );
    XAresult (*IsVibrating) (
        XAVibraItf self,
        XAboolean * pVibrating
    );
    XAresult (*SetFrequency) (
        XAVibraItf self,
        XAmilliHertz frequency
    );
    XAresult (*GetFrequency) (
        XAVibraItf self,
        XAmilliHertz * pFrequency
    );
    XAresult (*SetIntensity) (
        XAVibraItf self,
        XApermille intensity
    );
    XAresult (*GetIntensity) (
        XAVibraItf self,
        XApermille * pIntensity
    );
};

/* LED ARRAY */

typedef struct XAHSL_ {
    XAmillidegree hue;
    XApermille saturation;
    XApermille lightness;
} XAHSL;

XA_API extern const XAInterfaceID XA_IID_LED;

struct XALEDArrayItf_;
typedef const struct XALEDArrayItf_ * const * XALEDArrayItf;

struct XALEDArrayItf_ {
    XAresult (*ActivateLEDArray) (
        XALEDArrayItf self,
        XAuint32 lightMask
    );
    XAresult (*IsLEDArrayActivated) (
        XALEDArrayItf self,
        XAuint32 * pLightMask
    );
    XAresult (*SetColor) (
        XALEDArrayItf self,
        XAuint8 index,
        const XAHSL * pColor
    );
    XAresult (*GetColor) (
        XALEDArrayItf self,
        XAuint8 index,
        XAHSL * pColor
    );
};



  /*****************************************************************/
  /* CODEC RELATED INTERFACES, STRUCTS AND DEFINES                 */
  /*****************************************************************/

/* AUDIO ENCODER AND AUDIO ENCODER/DECODER CAPABILITIES */

#define XA_RATECONTROLMODE_CONSTANTBITRATE  ((XAuint32) 0x00000001)
#define XA_RATECONTROLMODE_VARIABLEBITRATE  ((XAuint32) 0x00000002)

#define XA_AUDIOCODEC_PCM                   ((XAuint32) 0x00000001)
#define XA_AUDIOCODEC_MP3                   ((XAuint32) 0x00000002)
#define XA_AUDIOCODEC_AMR                   ((XAuint32) 0x00000003)
#define XA_AUDIOCODEC_AMRWB                 ((XAuint32) 0x00000004)
#define XA_AUDIOCODEC_AMRWBPLUS             ((XAuint32) 0x00000005)
#define XA_AUDIOCODEC_AAC                   ((XAuint32) 0x00000006)
#define XA_AUDIOCODEC_WMA                   ((XAuint32) 0x00000007)
#define XA_AUDIOCODEC_REAL                  ((XAuint32) 0x00000008)
#define XA_AUDIOCODEC_VORBIS                ((XAuint32) 0x00000009)

#define XA_AUDIOPROFILE_PCM                 ((XAuint32) 0x00000001)

#define XA_AUDIOPROFILE_MPEG1_L3            ((XAuint32) 0x00000001)
#define XA_AUDIOPROFILE_MPEG2_L3            ((XAuint32) 0x00000002)
#define XA_AUDIOPROFILE_MPEG25_L3           ((XAuint32) 0x00000003)

#define XA_AUDIOCHANMODE_MP3_MONO           ((XAuint32) 0x00000001)
#define XA_AUDIOCHANMODE_MP3_STEREO         ((XAuint32) 0x00000002)
#define XA_AUDIOCHANMODE_MP3_JOINTSTEREO    ((XAuint32) 0x00000003)
#define XA_AUDIOCHANMODE_MP3_DUAL           ((XAuint32) 0x00000004)

#define XA_AUDIOPROFILE_AMR                 ((XAuint32) 0x00000001)

#define XA_AUDIOSTREAMFORMAT_CONFORMANCE    ((XAuint32) 0x00000001)
#define XA_AUDIOSTREAMFORMAT_IF1            ((XAuint32) 0x00000002)
#define XA_AUDIOSTREAMFORMAT_IF2            ((XAuint32) 0x00000003)
#define XA_AUDIOSTREAMFORMAT_FSF            ((XAuint32) 0x00000004)
#define XA_AUDIOSTREAMFORMAT_RTPPAYLOAD     ((XAuint32) 0x00000005)
#define XA_AUDIOSTREAMFORMAT_ITU            ((XAuint32) 0x00000006)

#define XA_AUDIOPROFILE_AMRWB               ((XAuint32) 0x00000001)

#define XA_AUDIOPROFILE_AMRWBPLUS           ((XAuint32) 0x00000001)

#define XA_AUDIOPROFILE_AAC_AAC             ((XAuint32) 0x00000001)

#define XA_AUDIOMODE_AAC_MAIN               ((XAuint32) 0x00000001)
#define XA_AUDIOMODE_AAC_LC                 ((XAuint32) 0x00000002)
#define XA_AUDIOMODE_AAC_SSR                ((XAuint32) 0x00000003)
#define XA_AUDIOMODE_AAC_LTP                ((XAuint32) 0x00000004)
#define XA_AUDIOMODE_AAC_HE                 ((XAuint32) 0x00000005)
#define XA_AUDIOMODE_AAC_SCALABLE           ((XAuint32) 0x00000006)
#define XA_AUDIOMODE_AAC_ERLC               ((XAuint32) 0x00000007)
#define XA_AUDIOMODE_AAC_LD                 ((XAuint32) 0x00000008)
#define XA_AUDIOMODE_AAC_HE_PS              ((XAuint32) 0x00000009)
#define XA_AUDIOMODE_AAC_HE_MPS             ((XAuint32) 0x0000000A)

#define XA_AUDIOSTREAMFORMAT_MP2ADTS        ((XAuint32) 0x00000001)
#define XA_AUDIOSTREAMFORMAT_MP4ADTS        ((XAuint32) 0x00000002)
#define XA_AUDIOSTREAMFORMAT_MP4LOAS        ((XAuint32) 0x00000003)
#define XA_AUDIOSTREAMFORMAT_MP4LATM        ((XAuint32) 0x00000004)
#define XA_AUDIOSTREAMFORMAT_ADIF           ((XAuint32) 0x00000005)
#define XA_AUDIOSTREAMFORMAT_MP4FF          ((XAuint32) 0x00000006)
#define XA_AUDIOSTREAMFORMAT_RAW            ((XAuint32) 0x00000007)

#define XA_AUDIOPROFILE_WMA7                ((XAuint32) 0x00000001)
#define XA_AUDIOPROFILE_WMA8                ((XAuint32) 0x00000002)
#define XA_AUDIOPROFILE_WMA9                ((XAuint32) 0x00000003)
#define XA_AUDIOPROFILE_WMA10               ((XAuint32) 0x00000004)

#define XA_AUDIOMODE_WMA_LEVEL1             ((XAuint32) 0x00000001)
#define XA_AUDIOMODE_WMA_LEVEL2             ((XAuint32) 0x00000002)
#define XA_AUDIOMODE_WMA_LEVEL3             ((XAuint32) 0x00000003)
#define XA_AUDIOMODE_WMA_LEVEL4             ((XAuint32) 0x00000004)
#define XA_AUDIOMODE_WMAPRO_LEVELM0         ((XAuint32) 0x00000005)
#define XA_AUDIOMODE_WMAPRO_LEVELM1         ((XAuint32) 0x00000006)
#define XA_AUDIOMODE_WMAPRO_LEVELM2         ((XAuint32) 0x00000007)
#define XA_AUDIOMODE_WMAPRO_LEVELM3         ((XAuint32) 0x00000008)

#define XA_AUDIOPROFILE_REALAUDIO           ((XAuint32) 0x00000001)

#define XA_AUDIOMODE_REALAUDIO_G2           ((XAuint32) 0x00000001)
#define XA_AUDIOMODE_REALAUDIO_8            ((XAuint32) 0x00000002)
#define XA_AUDIOMODE_REALAUDIO_10           ((XAuint32) 0x00000003)
#define XA_AUDIOMODE_REALAUDIO_SURROUND     ((XAuint32) 0x00000004)

#define XA_AUDIOPROFILE_VORBIS              ((XAuint32) 0x00000001)

#define XA_AUDIOMODE_VORBIS                 ((XAuint32) 0x00000001)


typedef struct XAAudioCodecDescriptor_ {
    XAuint32 maxChannels;
    XAuint32 minBitsPerSample;
    XAuint32 maxBitsPerSample;
    XAmilliHertz minSampleRate;
    XAmilliHertz maxSampleRate;
    XAboolean isFreqRangeContinuous;
    XAmilliHertz * pSampleRatesSupported;
    XAuint32 numSampleRatesSupported;
    XAuint32 minBitRate;
    XAuint32 maxBitRate;
    XAboolean isBitrateRangeContinuous;
    XAuint32 * pBitratesSupported;
    XAuint32 numBitratesSupported;
    XAuint32 profileSetting;
    XAuint32 modeSetting;
} XAAudioCodecDescriptor;

typedef struct XAAudioEncoderSettings_ {
    XAuint32 encoderId;
    XAuint32 channelsIn;
    XAuint32 channelsOut;
    XAmilliHertz sampleRate;
    XAuint32 bitRate;
    XAuint32 bitsPerSample;
    XAuint32 rateControl;
    XAuint32 profileSetting;
    XAuint32 levelSetting;
    XAuint32 channelMode;
    XAuint32 streamFormat;
    XAuint32 encodeOptions;
    XAuint32 blockAlignment;
} XAAudioEncoderSettings;

XA_API extern const XAInterfaceID XA_IID_AUDIODECODERCAPABILITIES;

struct XAAudioDecoderCapabilitiesItf_;
typedef const struct XAAudioDecoderCapabilitiesItf_
    * const * XAAudioDecoderCapabilitiesItf;

struct XAAudioDecoderCapabilitiesItf_ {
    XAresult (*GetAudioDecoders) (
        XAAudioDecoderCapabilitiesItf self,
        XAuint32 * pNumDecoders,
        XAuint32 * pDecoderIds
    );
    XAresult (*GetAudioDecoderCapabilities) (
        XAAudioDecoderCapabilitiesItf self,
        XAuint32 decoderId,
        XAuint32 * pIndex,
        XAAudioCodecDescriptor * pDescriptor
    );
};

XA_API extern const XAInterfaceID XA_IID_AUDIOENCODER;

struct XAAudioEncoderItf_;
typedef const struct XAAudioEncoderItf_ * const * XAAudioEncoderItf;

struct XAAudioEncoderItf_ {
    XAresult (*SetEncoderSettings) (
        XAAudioEncoderItf self,
        XAAudioEncoderSettings * pSettings
    );
    XAresult (*GetEncoderSettings) (
        XAAudioEncoderItf self,
        XAAudioEncoderSettings * pSettings
    );
};

XA_API extern const XAInterfaceID XA_IID_AUDIOENCODERCAPABILITIES;

struct XAAudioEncoderCapabilitiesItf_;
typedef const struct XAAudioEncoderCapabilitiesItf_
    * const * XAAudioEncoderCapabilitiesItf;

struct XAAudioEncoderCapabilitiesItf_ {
    XAresult (*GetAudioEncoders) (
        XAAudioEncoderCapabilitiesItf self,
        XAuint32 * pNumEncoders,
        XAuint32 * pEncoderIds
    );
    XAresult (*GetAudioEncoderCapabilities) (
        XAAudioEncoderCapabilitiesItf self,
        XAuint32 encoderId,
        XAuint32 * pIndex,
        XAAudioCodecDescriptor * pDescriptor
    );
};

/* IMAGE ENCODER AND IMAGE ENCODER/DECODER CAPABILITIES */

#define XA_IMAGECODEC_JPEG              ((XAuint32) 0x00000001)
#define XA_IMAGECODEC_GIF               ((XAuint32) 0x00000002)
#define XA_IMAGECODEC_BMP               ((XAuint32) 0x00000003)
#define XA_IMAGECODEC_PNG               ((XAuint32) 0x00000004)
#define XA_IMAGECODEC_TIFF              ((XAuint32) 0x00000005)
#define XA_IMAGECODEC_RAW               ((XAuint32) 0x00000006)

typedef struct XAImageCodecDescriptor_ {
    XAuint32 codecId;
    XAuint32 maxWidth;
    XAuint32 maxHeight;
} XAImageCodecDescriptor;

typedef struct XAImageSettings_ {
    XAuint32 encoderId;
    XAuint32 width;
    XAuint32 height;
    XApermille compressionLevel;
    XAuint32 colorFormat;
} XAImageSettings;

XA_API extern const XAInterfaceID XA_IID_IMAGEENCODERCAPABILITIES;

struct XAImageEncoderCapabilitiesItf_;
typedef const struct XAImageEncoderCapabilitiesItf_
    * const * XAImageEncoderCapabilitiesItf;

struct XAImageEncoderCapabilitiesItf_ {
    XAresult (*GetImageEncoderCapabilities) (
        XAImageEncoderCapabilitiesItf self,
        XAuint32 * pEncoderId,
        XAImageCodecDescriptor * pDescriptor
    );
    XAresult (*QueryColorFormats) (
        const XAImageEncoderCapabilitiesItf self,
        XAuint32 * pIndex,
        XAuint32 * pColorFormat
    );
};

XA_API extern const XAInterfaceID XA_IID_IMAGEDECODERCAPABILITIES;

struct XAImageDecoderCapabilitiesItf_;
typedef const struct XAImageDecoderCapabilitiesItf_
    * const * XAImageDecoderCapabilitiesItf;

struct XAImageDecoderCapabilitiesItf_ {
    XAresult (*GetImageDecoderCapabilities) (
        XAImageDecoderCapabilitiesItf self,
        XAuint32 * pDecoderId,
        XAImageCodecDescriptor * pDescriptor
    );
    XAresult (*QueryColorFormats) (
        const XAImageDecoderCapabilitiesItf self,
        XAuint32 * pIndex,
        XAuint32 * pColorFormat
    );
};

XA_API extern const XAInterfaceID XA_IID_IMAGEENCODER;

struct XAImageEncoderItf_;
typedef const struct XAImageEncoderItf_ * const * XAImageEncoderItf;

struct XAImageEncoderItf_ {
    XAresult (*SetImageSettings) (
        XAImageEncoderItf self,
        const XAImageSettings * pSettings
    );
    XAresult (*GetImageSettings) (
        XAImageEncoderItf self,
        XAImageSettings * pSettings
    );
    XAresult (*GetSizeEstimate) (
        XAImageEncoderItf self,
        XAuint32 * pSize
    );
};

/* VIDEO ENCODER AND VIDEO ENCODER/DECODER CAPABILITIES */

#define XA_VIDEOCODEC_MPEG2                     ((XAuint32) 0x00000001)
#define XA_VIDEOCODEC_H263                      ((XAuint32) 0x00000002)
#define XA_VIDEOCODEC_MPEG4                     ((XAuint32) 0x00000003)
#define XA_VIDEOCODEC_AVC                       ((XAuint32) 0x00000004)
#define XA_VIDEOCODEC_VC1                       ((XAuint32) 0x00000005)

#define XA_VIDEOPROFILE_MPEG2_SIMPLE            ((XAuint32) 0x00000001)
#define XA_VIDEOPROFILE_MPEG2_MAIN              ((XAuint32) 0x00000002)
#define XA_VIDEOPROFILE_MPEG2_422               ((XAuint32) 0x00000003)
#define XA_VIDEOPROFILE_MPEG2_SNR               ((XAuint32) 0x00000004)
#define XA_VIDEOPROFILE_MPEG2_SPATIAL           ((XAuint32) 0x00000005)
#define XA_VIDEOPROFILE_MPEG2_HIGH              ((XAuint32) 0x00000006)

#define XA_VIDEOLEVEL_MPEG2_LL                  ((XAuint32) 0x00000001)
#define XA_VIDEOLEVEL_MPEG2_ML                  ((XAuint32) 0x00000002)
#define XA_VIDEOLEVEL_MPEG2_H14                 ((XAuint32) 0x00000003)
#define XA_VIDEOLEVEL_MPEG2_HL                  ((XAuint32) 0x00000004)

#define XA_VIDEOPROFILE_H263_BASELINE           ((XAuint32) 0x00000001)
#define XA_VIDEOPROFILE_H263_H320CODING         ((XAuint32) 0x00000002)
#define XA_VIDEOPROFILE_H263_BACKWARDCOMPATIBLE ((XAuint32) 0x00000003)
#define XA_VIDEOPROFILE_H263_ISWV2              ((XAuint32) 0x00000004)
#define XA_VIDEOPROFILE_H263_ISWV3              ((XAuint32) 0x00000005)
#define XA_VIDEOPROFILE_H263_HIGHCOMPRESSION    ((XAuint32) 0x00000006)
#define XA_VIDEOPROFILE_H263_INTERNET           ((XAuint32) 0x00000007)
#define XA_VIDEOPROFILE_H263_INTERLACE          ((XAuint32) 0x00000008)
#define XA_VIDEOPROFILE_H263_HIGHLATENCY        ((XAuint32) 0x00000009)

#define XA_VIDEOLEVEL_H263_10                   ((XAuint32) 0x00000001)
#define XA_VIDEOLEVEL_H263_20                   ((XAuint32) 0x00000002)
#define XA_VIDEOLEVEL_H263_30                   ((XAuint32) 0x00000003)
#define XA_VIDEOLEVEL_H263_40                   ((XAuint32) 0x00000004)
#define XA_VIDEOLEVEL_H263_45                   ((XAuint32) 0x00000005)
#define XA_VIDEOLEVEL_H263_50                   ((XAuint32) 0x00000006)
#define XA_VIDEOLEVEL_H263_60                   ((XAuint32) 0x00000007)
#define XA_VIDEOLEVEL_H263_70                   ((XAuint32) 0x00000008)

#define XA_VIDEOPROFILE_MPEG4_SIMPLE            ((XAuint32) 0x00000001)
#define XA_VIDEOPROFILE_MPEG4_SIMPLESCALABLE    ((XAuint32) 0x00000002)
#define XA_VIDEOPROFILE_MPEG4_CORE              ((XAuint32) 0x00000003)
#define XA_VIDEOPROFILE_MPEG4_MAIN              ((XAuint32) 0x00000004)
#define XA_VIDEOPROFILE_MPEG4_NBIT              ((XAuint32) 0x00000005)
#define XA_VIDEOPROFILE_MPEG4_SCALABLETEXTURE   ((XAuint32) 0x00000006)
#define XA_VIDEOPROFILE_MPEG4_SIMPLEFACE        ((XAuint32) 0x00000007)
#define XA_VIDEOPROFILE_MPEG4_SIMPLEFBA         ((XAuint32) 0x00000008)
#define XA_VIDEOPROFILE_MPEG4_BASICANIMATED     ((XAuint32) 0x00000009)
#define XA_VIDEOPROFILE_MPEG4_HYBRID            ((XAuint32) 0x0000000A)
#define XA_VIDEOPROFILE_MPEG4_ADVANCEDREALTIME  ((XAuint32) 0x0000000B)
#define XA_VIDEOPROFILE_MPEG4_CORESCALABLE      ((XAuint32) 0x0000000C)
#define XA_VIDEOPROFILE_MPEG4_ADVANCEDCODING    ((XAuint32) 0x0000000D)
#define XA_VIDEOPROFILE_MPEG4_ADVANCEDCORE      ((XAuint32) 0x0000000E)
#define XA_VIDEOPROFILE_MPEG4_ADVANCEDSCALABLE  ((XAuint32) 0x0000000F)

#define XA_VIDEOLEVEL_MPEG4_0                   ((XAuint32) 0x00000001)
#define XA_VIDEOLEVEL_MPEG4_0b                  ((XAuint32) 0x00000002)
#define XA_VIDEOLEVEL_MPEG4_1                   ((XAuint32) 0x00000003)
#define XA_VIDEOLEVEL_MPEG4_2                   ((XAuint32) 0x00000004)
#define XA_VIDEOLEVEL_MPEG4_3                   ((XAuint32) 0x00000005)
#define XA_VIDEOLEVEL_MPEG4_4                   ((XAuint32) 0x00000006)
#define XA_VIDEOLEVEL_MPEG4_4a                  ((XAuint32) 0x00000007)
#define XA_VIDEOLEVEL_MPEG4_5                   ((XAuint32) 0x00000008)

#define XA_VIDEOPROFILE_AVC_BASELINE            ((XAuint32) 0x00000001)
#define XA_VIDEOPROFILE_AVC_MAIN                ((XAuint32) 0x00000002)
#define XA_VIDEOPROFILE_AVC_EXTENDED            ((XAuint32) 0x00000003)
#define XA_VIDEOPROFILE_AVC_HIGH                ((XAuint32) 0x00000004)
#define XA_VIDEOPROFILE_AVC_HIGH10              ((XAuint32) 0x00000005)
#define XA_VIDEOPROFILE_AVC_HIGH422             ((XAuint32) 0x00000006)
#define XA_VIDEOPROFILE_AVC_HIGH444             ((XAuint32) 0x00000007)

#define XA_VIDEOLEVEL_AVC_1                     ((XAuint32) 0x00000001)
#define XA_VIDEOLEVEL_AVC_1B                    ((XAuint32) 0x00000002)
#define XA_VIDEOLEVEL_AVC_11                    ((XAuint32) 0x00000003)
#define XA_VIDEOLEVEL_AVC_12                    ((XAuint32) 0x00000004)
#define XA_VIDEOLEVEL_AVC_13                    ((XAuint32) 0x00000005)
#define XA_VIDEOLEVEL_AVC_2                     ((XAuint32) 0x00000006)
#define XA_VIDEOLEVEL_AVC_21                    ((XAuint32) 0x00000007)
#define XA_VIDEOLEVEL_AVC_22                    ((XAuint32) 0x00000008)
#define XA_VIDEOLEVEL_AVC_3                     ((XAuint32) 0x00000009)
#define XA_VIDEOLEVEL_AVC_31                    ((XAuint32) 0x0000000A)
#define XA_VIDEOLEVEL_AVC_32                    ((XAuint32) 0x0000000B)
#define XA_VIDEOLEVEL_AVC_4                     ((XAuint32) 0x0000000C)
#define XA_VIDEOLEVEL_AVC_41                    ((XAuint32) 0x0000000D)
#define XA_VIDEOLEVEL_AVC_42                    ((XAuint32) 0x0000000E)
#define XA_VIDEOLEVEL_AVC_5                     ((XAuint32) 0x0000000F)
#define XA_VIDEOLEVEL_AVC_51                    ((XAuint32) 0x00000010)

#define XA_VIDEOLEVEL_VC1_SIMPLE                ((XAuint32) 0x00000001)
#define XA_VIDEOLEVEL_VC1_MAIN                  ((XAuint32) 0x00000002)
#define XA_VIDEOLEVEL_VC1_ADVANCED              ((XAuint32) 0x00000003)

#define XA_VIDEOLEVEL_VC1_LOW                   ((XAuint32) 0x00000001)
#define XA_VIDEOLEVEL_VC1_MEDIUM                ((XAuint32) 0x00000002)
#define XA_VIDEOLEVEL_VC1_HIGH                  ((XAuint32) 0x00000003)
#define XA_VIDEOLEVEL_VC1_L0                    ((XAuint32) 0x00000004)
#define XA_VIDEOLEVEL_VC1_L1                    ((XAuint32) 0x00000005)
#define XA_VIDEOLEVEL_VC1_L2                    ((XAuint32) 0x00000006)
#define XA_VIDEOLEVEL_VC1_L3                    ((XAuint32) 0x00000007)
#define XA_VIDEOLEVEL_VC1_L4                    ((XAuint32) 0x00000008)

typedef struct XAVideoCodecDescriptor_ {
    XAuint32 codecId;
    XAuint32 maxWidth;
    XAuint32 maxHeight;
    XAuint32 maxFrameRate;
    XAuint32 maxBitRate;
    XAuint32 rateControlSupported;
    XAuint32 profileSetting;
    XAuint32 levelSetting;
} XAVideoCodecDescriptor;

typedef struct XAVideoSettings_ {
    XAuint32 encoderId;
    XAuint32 width;
    XAuint32 height;
    XAuint32 frameRate;
    XAuint32 bitRate;
    XAuint32 rateControl;
    XAuint32 profileSetting;
    XAuint32 levelSetting;
    XAuint32 keyFrameInterval;
} XAVideoSettings;

XA_API extern const XAInterfaceID XA_IID_VIDEODECODERCAPABILITIES;

struct XAVideoDecoderCapabilitiesItf_;
typedef const struct XAVideoDecoderCapabilitiesItf_
    * const * XAVideoDecoderCapabilitiesItf;

struct XAVideoDecoderCapabilitiesItf_ {
    XAresult (*GetVideoDecoders) (
        XAVideoDecoderCapabilitiesItf self,
        XAuint32 * pNumDecoders,
        XAuint32 * pDecoderIds
    );
    XAresult (*GetVideoDecoderCapabilities) (
        XAVideoDecoderCapabilitiesItf self,
        XAuint32 decoderId,
        XAuint32 * pIndex,
        XAVideoCodecDescriptor * pDescriptor
    );
};

XA_API extern const XAInterfaceID XA_IID_VIDEOENCODER;

XA_API extern const XAInterfaceID XA_IID_VIDEOENCODERCAPABILITIES;

struct XAVideoEncoderCapabilitiesItf_;
typedef const struct XAVideoEncoderCapabilitiesItf_
    * const * XAVideoEncoderCapabilitiesItf;

struct XAVideoEncoderCapabilitiesItf_ {
    XAresult (*GetVideoEncoders) (
        XAVideoEncoderCapabilitiesItf self,
        XAuint32 * pNumEncoders,
        XAuint32 * pEncoderIds
    );
    XAresult (*GetVideoEncoderCapabilities) (
        XAVideoEncoderCapabilitiesItf self,
        XAuint32 encoderId,
        XAuint32 * pIndex,
        XAVideoCodecDescriptor * pDescriptor
    );
};

struct XAVideoEncoderItf_;
typedef const struct XAVideoEncoderItf_ * const * XAVideoEncoderItf;

struct XAVideoEncoderItf_ {
    XAresult (*SetVideoSettings) (
        XAVideoEncoderItf self,
        XAVideoSettings * pSettings
    );
    XAresult (*GetVideoSettings) (
        XAVideoEncoderItf self,
        XAVideoSettings * pSettings
    );
};

/* STREAM INFORMATION */

#define XA_DOMAINTYPE_AUDIO     0x00000001
#define XA_DOMAINTYPE_VIDEO     0x00000002
#define XA_DOMAINTYPE_IMAGE     0x00000003
#define XA_DOMAINTYPE_TIMEDTEXT 0x00000004
#define XA_DOMAINTYPE_MIDI      0x00000005
#define XA_DOMAINTYPE_VENDOR    0xFFFFFFFE
#define XA_DOMAINTYPE_UNKNOWN   0xFFFFFFFF

#define XA_MIDIBANK_DEVICE      0x00000001
#define XA_MIDIBANK_CUSTOM      0x00000002

#define XA_MIDI_UNKNOWN         0xFFFFFFFF

#define XA_STREAMCBEVENT_PROPERTYCHANGE     ((XAuint32) 0x00000001)

typedef struct XAMediaContainerInformation_ {
    XAuint32 containerType;
    XAmillisecond mediaDuration;
    XAuint32 numStreams;
} XAMediaContainerInformation;

typedef struct XAVideoStreamInformation_ {
    XAuint32 codecId;
    XAuint32 width;
    XAuint32 height;
    XAuint32 frameRate;
    XAuint32 bitRate;
    XAmillisecond duration;
} XAVideoStreamInformation;

typedef struct XAAudioStreamInformation_ {
    XAuint32 codecId;
    XAuint32 channels;
    XAmilliHertz sampleRate;
    XAuint32 bitRate;
    XAchar langCountry[16];
    XAmillisecond duration;
} XAAudioStreamInformation;

typedef struct XAImageStreamInformation_ {
    XAuint32 codecId;
    XAuint32 width;
    XAuint32 height;
    XAmillisecond presentationDuration;
} XAImageStreamInformation;

typedef struct XATimedTextStreamInformation_ {
    XAuint16 layer;
    XAuint32 width;
    XAuint32 height;
    XAuint16 tx;
    XAuint16 ty;
    XAuint32 bitrate;
    XAchar langCountry[16];
    XAmillisecond duration;
} XATimedTextStreamInformation;

typedef struct XAMIDIStreamInformation_ {
    XAuint32 channels;
    XAuint32 tracks;
    XAuint32 bankType;
    XAchar langCountry[16];
    XAmillisecond duration;
} XAMIDIStreamInformation;

typedef struct XAVendorStreamInformation_ {
    void *VendorStreamInfo;
} XAVendorStreamInformation;

XA_API extern const XAInterfaceID XA_IID_STREAMINFORMATION;

struct XAStreamInformationItf_;
typedef const struct XAStreamInformationItf_ * const * XAStreamInformationItf;

typedef void (XAAPIENTRY * xaStreamEventChangeCallback) (
    XAStreamInformationItf caller,
    XAuint32 eventId,
    XAuint32 streamIndex,
    void * pEventData,
    void * pContext
);

struct XAStreamInformationItf_ {
    XAresult (*QueryMediaContainerInformation) (
        XAStreamInformationItf self,
        XAMediaContainerInformation * info
    );
    XAresult (*QueryStreamType) (
        XAStreamInformationItf self,
        XAuint32 streamIndex,
        XAuint32 *domain
    );
    XAresult (*QueryStreamInformation) (
        XAStreamInformationItf self,
        XAuint32 streamIndex,
        void * info
    );
    XAresult (*QueryStreamName) (
        XAStreamInformationItf self,
        XAuint32 streamIndex,
        XAuint16 * pNameSize,
        XAchar * pName
    );
    XAresult (*RegisterStreamChangeCallback) (
        XAStreamInformationItf self,
        xaStreamEventChangeCallback callback,
        void * pContext
    );
    XAresult (*QueryActiveStreams) (
        XAStreamInformationItf self,
        XAuint32 *numStreams,
        XAboolean *activeStreams
    );
    XAresult (*SetActiveStream) (
        XAStreamInformationItf self,
        XAuint32   streamNum,
        XAboolean  active,
        XAboolean  commitNow
    );
};

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* _OPENMAXAL_H_ */
