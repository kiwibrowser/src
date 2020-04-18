/*
 * Copyright (C) 2013 The Android Open Source Project
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

#ifndef RENDER_SCRIPT_DEFINES_H
#define RENDER_SCRIPT_DEFINES_H

#include <stdint.h>
#include <sys/types.h>

#ifdef __cplusplus
extern "C" {
#endif

//////////////////////////////////////////////////////
//

typedef void * RsAsyncVoidPtr;

typedef void * RsAllocation;
typedef void * RsAnimation;
typedef void * RsClosure;
typedef void * RsContext;
typedef void * RsDevice;
typedef void * RsElement;
typedef void * RsFile;
typedef void * RsFont;
typedef void * RsSampler;
typedef void * RsScript;
typedef void * RsScriptKernelID;
typedef void * RsScriptInvokeID;
typedef void * RsScriptFieldID;
typedef void * RsScriptMethodID;
typedef void * RsScriptGroup;
typedef void * RsScriptGroup2;
typedef void * RsMesh;
typedef void * RsPath;
typedef void * RsType;
typedef void * RsObjectBase;

typedef void * RsProgram;
typedef void * RsProgramVertex;
typedef void * RsProgramFragment;
typedef void * RsProgramStore;
typedef void * RsProgramRaster;

typedef void * RsNativeWindow;

typedef void (* RsBitmapCallback_t)(void *);

typedef struct {
    float m[16];
} rs_matrix4x4;

typedef struct {
    float m[9];
} rs_matrix3x3;

typedef struct {
    float m[4];
} rs_matrix2x2;

enum RsDeviceParam {
    RS_DEVICE_PARAM_FORCE_SOFTWARE_GL,
    RS_DEVICE_PARAM_COUNT
};

enum RsContextType {
    RS_CONTEXT_TYPE_NORMAL,
    RS_CONTEXT_TYPE_DEBUG,
    RS_CONTEXT_TYPE_PROFILE
};


enum RsAllocationUsageType {
    RS_ALLOCATION_USAGE_SCRIPT = 0x0001,
    RS_ALLOCATION_USAGE_GRAPHICS_TEXTURE = 0x0002,
    RS_ALLOCATION_USAGE_GRAPHICS_VERTEX = 0x0004,
    RS_ALLOCATION_USAGE_GRAPHICS_CONSTANTS = 0x0008,
    RS_ALLOCATION_USAGE_GRAPHICS_RENDER_TARGET = 0x0010,
    RS_ALLOCATION_USAGE_IO_INPUT = 0x0020,
    RS_ALLOCATION_USAGE_IO_OUTPUT = 0x0040,
    RS_ALLOCATION_USAGE_SHARED = 0x0080,

    RS_ALLOCATION_USAGE_INCREMENTAL_SUPPORT = 0x1000,
    RS_ALLOCATION_USAGE_OEM = 0x8000,
    RS_ALLOCATION_USAGE_ALL = 0x80FF
};

enum RsAllocationMipmapControl {
    RS_ALLOCATION_MIPMAP_NONE = 0,
    RS_ALLOCATION_MIPMAP_FULL = 1,
    RS_ALLOCATION_MIPMAP_ON_SYNC_TO_TEXTURE = 2
};

enum RsAllocationCubemapFace {
    RS_ALLOCATION_CUBEMAP_FACE_POSITIVE_X = 0,
    RS_ALLOCATION_CUBEMAP_FACE_NEGATIVE_X = 1,
    RS_ALLOCATION_CUBEMAP_FACE_POSITIVE_Y = 2,
    RS_ALLOCATION_CUBEMAP_FACE_NEGATIVE_Y = 3,
    RS_ALLOCATION_CUBEMAP_FACE_POSITIVE_Z = 4,
    RS_ALLOCATION_CUBEMAP_FACE_NEGATIVE_Z = 5
};

enum RsDataType {
    RS_TYPE_NONE = 0,
    RS_TYPE_FLOAT_16,
    RS_TYPE_FLOAT_32,
    RS_TYPE_FLOAT_64,
    RS_TYPE_SIGNED_8,
    RS_TYPE_SIGNED_16,
    RS_TYPE_SIGNED_32,
    RS_TYPE_SIGNED_64,
    RS_TYPE_UNSIGNED_8,
    RS_TYPE_UNSIGNED_16,
    RS_TYPE_UNSIGNED_32,
    RS_TYPE_UNSIGNED_64,

    RS_TYPE_BOOLEAN,

    RS_TYPE_UNSIGNED_5_6_5,
    RS_TYPE_UNSIGNED_5_5_5_1,
    RS_TYPE_UNSIGNED_4_4_4_4,

    RS_TYPE_MATRIX_4X4,
    RS_TYPE_MATRIX_3X3,
    RS_TYPE_MATRIX_2X2,

    RS_TYPE_ELEMENT = 1000,
    RS_TYPE_TYPE,
    RS_TYPE_ALLOCATION,
    RS_TYPE_SAMPLER,
    RS_TYPE_SCRIPT,
    RS_TYPE_MESH,
    RS_TYPE_PROGRAM_FRAGMENT,
    RS_TYPE_PROGRAM_VERTEX,
    RS_TYPE_PROGRAM_RASTER,
    RS_TYPE_PROGRAM_STORE,
    RS_TYPE_FONT,

    RS_TYPE_INVALID = 10000,
};

enum RsDataKind {
    RS_KIND_USER,

    RS_KIND_PIXEL_L = 7,
    RS_KIND_PIXEL_A,
    RS_KIND_PIXEL_LA,
    RS_KIND_PIXEL_RGB,
    RS_KIND_PIXEL_RGBA,
    RS_KIND_PIXEL_DEPTH,
    RS_KIND_PIXEL_YUV,

    RS_KIND_INVALID = 100,
};

enum RsYuvFormat {
    RS_YUV_NONE    = 0,
    RS_YUV_YV12    = 0x32315659, // HAL_PIXEL_FORMAT_YV12 in system/graphics.h
    RS_YUV_NV21    = 0x11,       // HAL_PIXEL_FORMAT_YCrCb_420_SP
    RS_YUV_420_888 = 0x23,       // HAL_PIXEL_FORMAT_YCbCr_420_888
};

enum RsSamplerParam {
    RS_SAMPLER_MIN_FILTER,
    RS_SAMPLER_MAG_FILTER,
    RS_SAMPLER_WRAP_S,
    RS_SAMPLER_WRAP_T,
    RS_SAMPLER_WRAP_R,
    RS_SAMPLER_ANISO
};

enum RsSamplerValue {
    RS_SAMPLER_NEAREST,
    RS_SAMPLER_LINEAR,
    RS_SAMPLER_LINEAR_MIP_LINEAR,
    RS_SAMPLER_WRAP,
    RS_SAMPLER_CLAMP,
    RS_SAMPLER_LINEAR_MIP_NEAREST,
    RS_SAMPLER_MIRRORED_REPEAT,

    RS_SAMPLER_INVALID = 100,
};

enum RsDimension {
    RS_DIMENSION_X,
    RS_DIMENSION_Y,
    RS_DIMENSION_Z,
    RS_DIMENSION_LOD,
    RS_DIMENSION_FACE,

    RS_DIMENSION_ARRAY_0 = 100,
    RS_DIMENSION_ARRAY_1,
    RS_DIMENSION_ARRAY_2,
    RS_DIMENSION_ARRAY_3,
    RS_DIMENSION_MAX = RS_DIMENSION_ARRAY_3
};


enum RsError {
    RS_ERROR_NONE = 0,
    RS_ERROR_BAD_SHADER = 1,
    RS_ERROR_BAD_SCRIPT = 2,
    RS_ERROR_BAD_VALUE = 3,
    RS_ERROR_OUT_OF_MEMORY = 4,
    RS_ERROR_DRIVER = 5,

    // Errors that only occur in the debug context.
    RS_ERROR_FATAL_DEBUG = 0x0800,

    RS_ERROR_FATAL_UNKNOWN = 0x1000,
    RS_ERROR_FATAL_DRIVER = 0x1001,
    RS_ERROR_FATAL_PROGRAM_LINK = 0x1002
};

enum RsForEachStrategy {
    RS_FOR_EACH_STRATEGY_SERIAL = 0,
    RS_FOR_EACH_STRATEGY_DONT_CARE = 1,
    RS_FOR_EACH_STRATEGY_DST_LINEAR = 2,
    RS_FOR_EACH_STRATEGY_TILE_SMALL = 3,
    RS_FOR_EACH_STRATEGY_TILE_MEDIUM = 4,
    RS_FOR_EACH_STRATEGY_TILE_LARGE = 5
};

// Script to Script
typedef struct {
    enum RsForEachStrategy strategy;
    uint32_t xStart;
    uint32_t xEnd;
    uint32_t yStart;
    uint32_t yEnd;
    uint32_t zStart;
    uint32_t zEnd;
    uint32_t arrayStart;
    uint32_t arrayEnd;
    uint32_t array2Start;
    uint32_t array2End;
    uint32_t array3Start;
    uint32_t array3End;
    uint32_t array4Start;
    uint32_t array4End;

} RsScriptCall;

enum RsContextFlags {
    RS_CONTEXT_SYNCHRONOUS      = 0x0001,
    RS_CONTEXT_LOW_LATENCY      = 0x0002,
    RS_CONTEXT_LOW_POWER        = 0x0004,
    RS_CONTEXT_WAIT_FOR_ATTACH  = 0x0008
};

enum RsBlasTranspose {
    RsBlasNoTrans=111,
    RsBlasTrans=112,
    RsBlasConjTrans=113
};

enum RsBlasUplo {
    RsBlasUpper=121,
    RsBlasLower=122
};

enum RsBlasDiag {
    RsBlasNonUnit=131,
    RsBlasUnit=132
};

enum RsBlasSide {
    RsBlasLeft=141,
    RsBlasRight=142
};

enum RsBlasFunction {
    RsBlas_nop = 0,
    RsBlas_sdsdot = 1,
    RsBlas_dsdot = 2,
    RsBlas_sdot = 3,
    RsBlas_ddot = 4,
    RsBlas_cdotu_sub = 5,
    RsBlas_cdotc_sub = 6,
    RsBlas_zdotu_sub = 7,
    RsBlas_zdotc_sub = 8,
    RsBlas_snrm2 = 9,
    RsBlas_sasum = 10,
    RsBlas_dnrm2 = 11,
    RsBlas_dasum = 12,
    RsBlas_scnrm2 = 13,
    RsBlas_scasum = 14,
    RsBlas_dznrm2 = 15,
    RsBlas_dzasum = 16,
    RsBlas_isamax = 17,
    RsBlas_idamax = 18,
    RsBlas_icamax = 19,
    RsBlas_izamax = 20,
    RsBlas_sswap = 21,
    RsBlas_scopy = 22,
    RsBlas_saxpy = 23,
    RsBlas_dswap = 24,
    RsBlas_dcopy = 25,
    RsBlas_daxpy = 26,
    RsBlas_cswap = 27,
    RsBlas_ccopy = 28,
    RsBlas_caxpy = 29,
    RsBlas_zswap = 30,
    RsBlas_zcopy = 31,
    RsBlas_zaxpy = 32,
    RsBlas_srotg = 33,
    RsBlas_srotmg = 34,
    RsBlas_srot = 35,
    RsBlas_srotm = 36,
    RsBlas_drotg = 37,
    RsBlas_drotmg = 38,
    RsBlas_drot = 39,
    RsBlas_drotm = 40,
    RsBlas_sscal = 41,
    RsBlas_dscal = 42,
    RsBlas_cscal = 43,
    RsBlas_zscal = 44,
    RsBlas_csscal = 45,
    RsBlas_zdscal = 46,
    RsBlas_sgemv = 47,
    RsBlas_sgbmv = 48,
    RsBlas_strmv = 49,
    RsBlas_stbmv = 50,
    RsBlas_stpmv = 51,
    RsBlas_strsv = 52,
    RsBlas_stbsv = 53,
    RsBlas_stpsv = 54,
    RsBlas_dgemv = 55,
    RsBlas_dgbmv = 56,
    RsBlas_dtrmv = 57,
    RsBlas_dtbmv = 58,
    RsBlas_dtpmv = 59,
    RsBlas_dtrsv = 60,
    RsBlas_dtbsv = 61,
    RsBlas_dtpsv = 62,
    RsBlas_cgemv = 63,
    RsBlas_cgbmv = 64,
    RsBlas_ctrmv = 65,
    RsBlas_ctbmv = 66,
    RsBlas_ctpmv = 67,
    RsBlas_ctrsv = 68,
    RsBlas_ctbsv = 69,
    RsBlas_ctpsv = 70,
    RsBlas_zgemv = 71,
    RsBlas_zgbmv = 72,
    RsBlas_ztrmv = 73,
    RsBlas_ztbmv = 74,
    RsBlas_ztpmv = 75,
    RsBlas_ztrsv = 76,
    RsBlas_ztbsv = 77,
    RsBlas_ztpsv = 78,
    RsBlas_ssymv = 79,
    RsBlas_ssbmv = 80,
    RsBlas_sspmv = 81,
    RsBlas_sger = 82,
    RsBlas_ssyr = 83,
    RsBlas_sspr = 84,
    RsBlas_ssyr2 = 85,
    RsBlas_sspr2 = 86,
    RsBlas_dsymv = 87,
    RsBlas_dsbmv = 88,
    RsBlas_dspmv = 89,
    RsBlas_dger = 90,
    RsBlas_dsyr = 91,
    RsBlas_dspr = 92,
    RsBlas_dsyr2 = 93,
    RsBlas_dspr2 = 94,
    RsBlas_chemv = 95,
    RsBlas_chbmv = 96,
    RsBlas_chpmv = 97,
    RsBlas_cgeru = 98,
    RsBlas_cgerc = 99,
    RsBlas_cher = 100,
    RsBlas_chpr = 101,
    RsBlas_cher2 = 102,
    RsBlas_chpr2 = 103,
    RsBlas_zhemv = 104,
    RsBlas_zhbmv = 105,
    RsBlas_zhpmv = 106,
    RsBlas_zgeru = 107,
    RsBlas_zgerc = 108,
    RsBlas_zher = 109,
    RsBlas_zhpr = 110,
    RsBlas_zher2 = 111,
    RsBlas_zhpr2 = 112,
    RsBlas_sgemm = 113,
    RsBlas_ssymm = 114,
    RsBlas_ssyrk = 115,
    RsBlas_ssyr2k = 116,
    RsBlas_strmm = 117,
    RsBlas_strsm = 118,
    RsBlas_dgemm = 119,
    RsBlas_dsymm = 120,
    RsBlas_dsyrk = 121,
    RsBlas_dsyr2k = 122,
    RsBlas_dtrmm = 123,
    RsBlas_dtrsm = 124,
    RsBlas_cgemm = 125,
    RsBlas_csymm = 126,
    RsBlas_csyrk = 127,
    RsBlas_csyr2k = 128,
    RsBlas_ctrmm = 129,
    RsBlas_ctrsm = 130,
    RsBlas_zgemm = 131,
    RsBlas_zsymm = 132,
    RsBlas_zsyrk = 133,
    RsBlas_zsyr2k = 134,
    RsBlas_ztrmm = 135,
    RsBlas_ztrsm = 136,
    RsBlas_chemm = 137,
    RsBlas_cherk = 138,
    RsBlas_cher2k = 139,
    RsBlas_zhemm = 140,
    RsBlas_zherk = 141,
    RsBlas_zher2k = 142,

    // BLAS extensions start here
    RsBlas_bnnm = 1000,
};

// custom complex types because of NDK support
typedef struct {
    float r;
    float i;
} RsFloatComplex;

typedef struct {
    double r;
    double i;
} RsDoubleComplex;

typedef union {
    float f;
    RsFloatComplex c;
    double d;
    RsDoubleComplex z;
} RsBlasScalar;

typedef struct {
    RsBlasFunction func;
    RsBlasTranspose transA;
    RsBlasTranspose transB;
    RsBlasUplo uplo;
    RsBlasDiag diag;
    RsBlasSide side;
    int M;
    int N;
    int K;
    RsBlasScalar alpha;
    RsBlasScalar beta;
    int incX;
    int incY;
    int KL;
    int KU;
    uint8_t a_offset;
    uint8_t b_offset;
    int32_t c_offset;
    int32_t c_mult_int;
} RsBlasCall;

enum RsGlobalProperty {
    RS_GLOBAL_TYPE     = 0x0000FFFF,
    RS_GLOBAL_CONSTANT = 0x00010000,
    RS_GLOBAL_STATIC   = 0x00020000,
    RS_GLOBAL_POINTER  = 0x00040000
};

// Special symbols embedded into a shared object compiled by bcc.
static const char kRoot[] = "root";
static const char kInit[] = "init";
static const char kRsDtor[] = ".rs.dtor";
static const char kRsInfo[] = ".rs.info";
static const char kRsGlobalEntries[] = ".rs.global_entries";
static const char kRsGlobalNames[] = ".rs.global_names";
static const char kRsGlobalAddresses[] = ".rs.global_addresses";
static const char kRsGlobalSizes[] = ".rs.global_sizes";
static const char kRsGlobalProperties[] = ".rs.global_properties";

static inline uint32_t getGlobalRsType(uint32_t properties) {
    return properties & RS_GLOBAL_TYPE;
}
static inline bool isGlobalConstant(uint32_t properties) {
    return properties & RS_GLOBAL_CONSTANT;
}
static inline bool isGlobalStatic(uint32_t properties) {
    return properties & RS_GLOBAL_STATIC;
}
static inline bool isGlobalPointer(uint32_t properties) {
    return properties & RS_GLOBAL_POINTER;
}

#ifdef __cplusplus
};
#endif

#endif // RENDER_SCRIPT_DEFINES_H
