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

#ifndef ANDROID_RSCPPSTRUCTS_H
#define ANDROID_RSCPPSTRUCTS_H

#include "rsDefines.h"
#include "util/RefBase.h"

#include <pthread.h>


/**
 * Every row in an RS allocation is guaranteed to be aligned by this amount, and
 * every row in a user-backed allocation must be aligned by this amount.
 */
#define RS_CPU_ALLOCATION_ALIGNMENT 16

struct dispatchTable;

namespace android {
class Surface;

namespace RSC {


typedef void (*ErrorHandlerFunc_t)(uint32_t errorNum, const char *errorText);
typedef void (*MessageHandlerFunc_t)(uint32_t msgNum, const void *msgData, size_t msgLen);

class RS;
class BaseObj;
class Element;
class Type;
class Allocation;
class Script;
class ScriptC;
class Sampler;

/**
 * Possible error codes used by RenderScript. Once a status other than RS_SUCCESS
 * is returned, the RenderScript context is considered dead and cannot perform any
 * additional work.
 */
 enum RSError {
     RS_SUCCESS = 0,                 ///< No error
     RS_ERROR_INVALID_PARAMETER = 1, ///< An invalid parameter was passed to a function
     RS_ERROR_RUNTIME_ERROR = 2,     ///< The RenderScript driver returned an error; this is
                                     ///< often indicative of a kernel that crashed
     RS_ERROR_INVALID_ELEMENT = 3,   ///< An invalid Element was passed to a function
     RS_ERROR_MAX = 9999

 };

 /**
  * Flags that can control RenderScript behavior on a per-context level.
  */
 enum RSInitFlags {
     RS_INIT_SYNCHRONOUS = 1, ///< All RenderScript calls will be synchronous. May reduce latency.
     RS_INIT_LOW_LATENCY = 2, ///< Prefer low latency devices over potentially higher throughput devices.
     // Bitflag 4 is reserved for the context flag low power
     RS_INIT_WAIT_FOR_ATTACH = 8,   ///< Kernel execution will hold to give time for a debugger to be attached
     RS_INIT_MAX = 16
 };


class Byte2 {
 public:
  int8_t x, y;

  Byte2(int8_t initX, int8_t initY)
    : x(initX), y(initY) {}
  Byte2() : x(0), y(0) {}
};

class Byte3 {
 public:
  int8_t x, y, z;

  Byte3(int8_t initX, int8_t initY, int8_t initZ)
    : x(initX), y(initY), z(initZ) {}
  Byte3() : x(0), y(0), z(0) {}
};

class Byte4 {
 public:
  int8_t x, y, z, w;

  Byte4(int8_t initX, int8_t initY, int8_t initZ, int8_t initW)
    : x(initX), y(initY), z(initZ), w(initW) {}
  Byte4() : x(0), y(0), z(0), w(0) {}
};

class UByte2 {
 public:
  uint8_t x, y;

  UByte2(uint8_t initX, uint8_t initY)
    : x(initX), y(initY) {}
  UByte2() : x(0), y(0) {}
};

class UByte3 {
 public:
  uint8_t x, y, z;

  UByte3(uint8_t initX, uint8_t initY, uint8_t initZ)
    : x(initX), y(initY), z(initZ) {}
  UByte3() : x(0), y(0), z(0) {}
};

class UByte4 {
 public:
  uint8_t x, y, z, w;

  UByte4(uint8_t initX, uint8_t initY, uint8_t initZ, uint8_t initW)
    : x(initX), y(initY), z(initZ), w(initW) {}
  UByte4() : x(0), y(0), z(0), w(0) {}
};

class Short2 {
 public:
  short x, y;

  Short2(short initX, short initY)
    : x(initX), y(initY) {}
  Short2() : x(0), y(0) {}
};

class Short3 {
 public:
  short x, y, z;

  Short3(short initX, short initY, short initZ)
    : x(initX), y(initY), z(initZ) {}
  Short3() : x(0), y(0), z(0) {}
};

class Short4 {
 public:
  short x, y, z, w;

  Short4(short initX, short initY, short initZ, short initW)
    : x(initX), y(initY), z(initZ), w(initW) {}
  Short4() : x(0), y(0), z(0), w(0) {}
};

class UShort2 {
 public:
  uint16_t x, y;

  UShort2(uint16_t initX, uint16_t initY)
    : x(initX), y(initY) {}
  UShort2() : x(0), y(0) {}
};

class UShort3 {
 public:
  uint16_t x, y, z;

  UShort3(uint16_t initX, uint16_t initY, uint16_t initZ)
    : x(initX), y(initY), z(initZ) {}
  UShort3() : x(0), y(0), z(0) {}
};

class UShort4 {
 public:
  uint16_t x, y, z, w;

  UShort4(uint16_t initX, uint16_t initY, uint16_t initZ, uint16_t initW)
    : x(initX), y(initY), z(initZ), w(initW) {}
  UShort4() : x(0), y(0), z(0), w(0) {}
};

class Int2 {
 public:
  int x, y;

  Int2(int initX, int initY)
    : x(initX), y(initY) {}
  Int2() : x(0), y(0) {}
};

class Int3 {
 public:
  int x, y, z;

  Int3(int initX, int initY, int initZ)
    : x(initX), y(initY), z(initZ) {}
  Int3() : x(0), y(0), z(0) {}
};

class Int4 {
 public:
  int x, y, z, w;

  Int4(int initX, int initY, int initZ, int initW)
    : x(initX), y(initY), z(initZ), w(initW) {}
  Int4() : x(0), y(0), z(0), w(0) {}
};

class UInt2 {
 public:
  uint32_t x, y;

  UInt2(uint32_t initX, uint32_t initY)
    : x(initX), y(initY) {}
  UInt2() : x(0), y(0) {}
};

class UInt3 {
 public:
  uint32_t x, y, z;

  UInt3(uint32_t initX, uint32_t initY, uint32_t initZ)
    : x(initX), y(initY), z(initZ) {}
  UInt3() : x(0), y(0), z(0) {}
};

class UInt4 {
 public:
  uint32_t x, y, z, w;

  UInt4(uint32_t initX, uint32_t initY, uint32_t initZ, uint32_t initW)
    : x(initX), y(initY), z(initZ), w(initW) {}
  UInt4() : x(0), y(0), z(0), w(0) {}
};

class Long2 {
 public:
  int64_t x, y;

  Long2(int64_t initX, int64_t initY)
    : x(initX), y(initY) {}
  Long2() : x(0), y(0) {}
};

class Long3 {
 public:
  int64_t x, y, z;

  Long3(int64_t initX, int64_t initY, int64_t initZ)
    : x(initX), y(initY), z(initZ) {}
  Long3() : x(0), y(0), z(0) {}
};

class Long4 {
 public:
  int64_t x, y, z, w;

  Long4(int64_t initX, int64_t initY, int64_t initZ, int64_t initW)
    : x(initX), y(initY), z(initZ), w(initW) {}
  Long4() : x(0), y(0), z(0), w(0) {}
};

class ULong2 {
 public:
  uint64_t x, y;

  ULong2(uint64_t initX, uint64_t initY)
    : x(initX), y(initY) {}
  ULong2() : x(0), y(0) {}
};

class ULong3 {
 public:
  uint64_t x, y, z;

  ULong3(uint64_t initX, uint64_t initY, uint64_t initZ)
    : x(initX), y(initY), z(initZ) {}
  ULong3() : x(0), y(0), z(0) {}
};

class ULong4 {
 public:
  uint64_t x, y, z, w;

  ULong4(uint64_t initX, uint64_t initY, uint64_t initZ, uint64_t initW)
    : x(initX), y(initY), z(initZ), w(initW) {}
  ULong4() : x(0), y(0), z(0), w(0) {}
};

class Float2 {
 public:
  float x, y;

  Float2(float initX, float initY)
    : x(initX), y(initY) {}
  Float2() : x(0), y(0) {}
};

class Float3 {
 public:
  float x, y, z;

  Float3(float initX, float initY, float initZ)
    : x(initX), y(initY), z(initZ) {}
  Float3() : x(0.f), y(0.f), z(0.f) {}
};

class Float4 {
 public:
  float x, y, z, w;

  Float4(float initX, float initY, float initZ, float initW)
    : x(initX), y(initY), z(initZ), w(initW) {}
  Float4() : x(0.f), y(0.f), z(0.f), w(0.f) {}
};

class Double2 {
 public:
  double x, y;

  Double2(double initX, double initY)
    : x(initX), y(initY) {}
  Double2() : x(0), y(0) {}
};

class Double3 {
 public:
  double x, y, z;

  Double3(double initX, double initY, double initZ)
    : x(initX), y(initY), z(initZ) {}
  Double3() : x(0), y(0), z(0) {}
};

class Double4 {
 public:
  double x, y, z, w;

  Double4(double initX, double initY, double initZ, double initW)
    : x(initX), y(initY), z(initZ), w(initW) {}
  Double4() : x(0), y(0), z(0), w(0) {}
};

 /**
  * The RenderScript context. This class controls initialization, resource management, and teardown.
  */
 class RS : public android::RSC::LightRefBase<RS> {

 public:
    RS();
    virtual ~RS();

    /**
     * Initializes a RenderScript context. A context must be initialized before it can be used.
     * @param[in] name Directory name to be used by this context. This should be equivalent to
     * Context.getCacheDir().
     * @param[in] flags Optional flags for this context.
     * @return true on success
     */
    bool init(const char * name, uint32_t flags = 0);

    /**
     * Initializes a RenderScript context. A context must be initialized before it can be used.
     * @param[in] name Directory name to be used by this context. This should be equivalent to
     * Context.getCacheDir().
     * @param[in] flags Flags for this context.
     * @param[in] targetApi Target RS API level.
     * @return true on success
     */
    bool init(const char * name, uint32_t flags, int targetApi);

    /**
     * Sets the error handler function for this context. This error handler is
     * called whenever an error is set.
     *
     * @param[in] func Error handler function
     */
    void setErrorHandler(ErrorHandlerFunc_t func);

    /**
     * Returns the current error handler function for this context.
     *
     * @return pointer to current error handler function or NULL if not set
     */
    ErrorHandlerFunc_t getErrorHandler() { return mErrorFunc; }

    /**
     * Sets the message handler function for this context. This message handler
     * is called whenever a message is sent from a RenderScript kernel.
     *
     *  @param[in] func Message handler function
     */
    void setMessageHandler(MessageHandlerFunc_t func);

    /**
     * Returns the current message handler function for this context.
     *
     * @return pointer to current message handler function or NULL if not set
     */
    MessageHandlerFunc_t getMessageHandler() { return mMessageFunc; }

    /**
     * Returns current status for the context.
     *
     * @return current error
     */
    RSError getError();

    /**
     * Waits for any currently running asynchronous operations to finish. This
     * should only be used for performance testing and timing.
     */
    void finish();

    RsContext getContext() { return mContext; }
    void throwError(RSError error, const char *errMsg);

    static dispatchTable* dispatch;

 private:
    static bool usingNative;
    static bool initDispatch(int targetApi);

    static void * threadProc(void *);

    static bool gInitialized;
    static pthread_mutex_t gInitMutex;

    pthread_t mMessageThreadId;
    pid_t mNativeMessageThreadId;
    bool mMessageRun;

    RsContext mContext;
    RSError mCurrentError;

    ErrorHandlerFunc_t mErrorFunc;
    MessageHandlerFunc_t mMessageFunc;
    bool mInit;

    char mCacheDir[PATH_MAX+1];
    uint32_t mCacheDirLen;

    struct {
        sp<const Element> U8;
        sp<const Element> U8_2;
        sp<const Element> U8_3;
        sp<const Element> U8_4;
        sp<const Element> I8;
        sp<const Element> I8_2;
        sp<const Element> I8_3;
        sp<const Element> I8_4;
        sp<const Element> U16;
        sp<const Element> U16_2;
        sp<const Element> U16_3;
        sp<const Element> U16_4;
        sp<const Element> I16;
        sp<const Element> I16_2;
        sp<const Element> I16_3;
        sp<const Element> I16_4;
        sp<const Element> U32;
        sp<const Element> U32_2;
        sp<const Element> U32_3;
        sp<const Element> U32_4;
        sp<const Element> I32;
        sp<const Element> I32_2;
        sp<const Element> I32_3;
        sp<const Element> I32_4;
        sp<const Element> U64;
        sp<const Element> U64_2;
        sp<const Element> U64_3;
        sp<const Element> U64_4;
        sp<const Element> I64;
        sp<const Element> I64_2;
        sp<const Element> I64_3;
        sp<const Element> I64_4;
        sp<const Element> F16;
        sp<const Element> F16_2;
        sp<const Element> F16_3;
        sp<const Element> F16_4;
        sp<const Element> F32;
        sp<const Element> F32_2;
        sp<const Element> F32_3;
        sp<const Element> F32_4;
        sp<const Element> F64;
        sp<const Element> F64_2;
        sp<const Element> F64_3;
        sp<const Element> F64_4;
        sp<const Element> BOOLEAN;

        sp<const Element> ELEMENT;
        sp<const Element> TYPE;
        sp<const Element> ALLOCATION;
        sp<const Element> SAMPLER;
        sp<const Element> SCRIPT;
        sp<const Element> MESH;
        sp<const Element> PROGRAM_FRAGMENT;
        sp<const Element> PROGRAM_VERTEX;
        sp<const Element> PROGRAM_RASTER;
        sp<const Element> PROGRAM_STORE;

        sp<const Element> A_8;
        sp<const Element> RGB_565;
        sp<const Element> RGB_888;
        sp<const Element> RGBA_5551;
        sp<const Element> RGBA_4444;
        sp<const Element> RGBA_8888;

        sp<const Element> YUV;

        sp<const Element> MATRIX_4X4;
        sp<const Element> MATRIX_3X3;
        sp<const Element> MATRIX_2X2;
    } mElements;

    struct {
        sp<const Sampler> CLAMP_NEAREST;
        sp<const Sampler> CLAMP_LINEAR;
        sp<const Sampler> CLAMP_LINEAR_MIP_LINEAR;
        sp<const Sampler> WRAP_NEAREST;
        sp<const Sampler> WRAP_LINEAR;
        sp<const Sampler> WRAP_LINEAR_MIP_LINEAR;
        sp<const Sampler> MIRRORED_REPEAT_NEAREST;
        sp<const Sampler> MIRRORED_REPEAT_LINEAR;
        sp<const Sampler> MIRRORED_REPEAT_LINEAR_MIP_LINEAR;
    } mSamplers;
    friend class Sampler;
    friend class Element;
    friend class ScriptC;
};

 /**
  * Base class for all RenderScript objects. Not for direct use by developers.
  */
class BaseObj : public android::RSC::LightRefBase<BaseObj> {
public:
    void * getID() const;
    virtual ~BaseObj();
    virtual void updateFromNative();
    virtual bool equals(const sp<const BaseObj>& obj);

protected:
    void *mID;
    RS* mRS;
    const char * mName;

    BaseObj(void *id, sp<RS> rs);
    void checkValid();

    static void * getObjID(const sp<const BaseObj>& o);

};

 /**
  * This class provides the primary method through which data is passed to and
  * from RenderScript kernels. An Allocation provides the backing store for a
  * given Type.
  *
  * An Allocation also contains a set of usage flags that denote how the
  * Allocation could be used. For example, an Allocation may have usage flags
  * specifying that it can be used from a script as well as input to a
  * Sampler. A developer must synchronize across these different usages using
  * syncAll(int) in order to ensure that different users of the Allocation have
  * a consistent view of memory. For example, in the case where an Allocation is
  * used as the output of one kernel and as Sampler input in a later kernel, a
  * developer must call syncAll(RS_ALLOCATION_USAGE_SCRIPT) prior to launching the
  * second kernel to ensure correctness.
  */
class Allocation : public BaseObj {
protected:
    sp<const Type> mType;
    uint32_t mUsage;
    sp<Allocation> mAdaptedAllocation;

    bool mConstrainedLOD;
    bool mConstrainedFace;
    bool mConstrainedY;
    bool mConstrainedZ;
    bool mReadAllowed;
    bool mWriteAllowed;
    bool mAutoPadding;
    uint32_t mSelectedY;
    uint32_t mSelectedZ;
    uint32_t mSelectedLOD;
    RsAllocationCubemapFace mSelectedFace;

    uint32_t mCurrentDimX;
    uint32_t mCurrentDimY;
    uint32_t mCurrentDimZ;
    uint32_t mCurrentCount;

    void * getIDSafe() const;
    void updateCacheInfo(const sp<const Type>& t);

    Allocation(void *id, sp<RS> rs, sp<const Type> t, uint32_t usage);

    void validateIsInt64();
    void validateIsInt32();
    void validateIsInt16();
    void validateIsInt8();
    void validateIsFloat32();
    void validateIsFloat64();
    void validateIsObject();

    virtual void updateFromNative();

    void validate2DRange(uint32_t xoff, uint32_t yoff, uint32_t w, uint32_t h);
    void validate3DRange(uint32_t xoff, uint32_t yoff, uint32_t zoff,
                         uint32_t w, uint32_t h, uint32_t d);

public:

    /**
     * Return Type for the allocation.
     * @return pointer to underlying Type
     */
    sp<const Type> getType() const {
        return mType;
    }

    /**
     * Enable/Disable AutoPadding for Vec3 elements.
     *
     * @param useAutoPadding True: enable AutoPadding; flase: disable AutoPadding
     *
     */
    void setAutoPadding(bool useAutoPadding) {
        mAutoPadding = useAutoPadding;
    }

    /**
     * Propagate changes from one usage of the Allocation to other usages of the Allocation.
     * @param[in] srcLocation source location with changes to propagate elsewhere
     */
    void syncAll(RsAllocationUsageType srcLocation);

    /**
     * Send a buffer to the output stream.  The contents of the Allocation will
     * be undefined after this operation. This operation is only valid if
     * USAGE_IO_OUTPUT is set on the Allocation.
     */
    void ioSendOutput();

    /**
     * Receive the latest input into the Allocation. This operation
     * is only valid if USAGE_IO_INPUT is set on the Allocation.
     */
    void ioGetInput();

#if !defined(RS_SERVER) && !defined(RS_COMPATIBILITY_LIB)
    /**
     * Returns the handle to a raw buffer that is being managed by the screen
     * compositor. This operation is only valid for Allocations with USAGE_IO_INPUT.
     * @return Surface associated with allocation
     */
    sp<Surface> getSurface();

    /**
     * Associate a Surface with this Allocation. This
     * operation is only valid for Allocations with USAGE_IO_OUTPUT.
     * @param[in] s Surface to associate with allocation
     */
    void setSurface(const sp<Surface>& s);
#endif

    /**
     * Generate a mipmap chain. This is only valid if the Type of the Allocation
     * includes mipmaps. This function will generate a complete set of mipmaps
     * from the top level LOD and place them into the script memory space. If
     * the Allocation is also using other memory spaces, a call to
     * syncAll(Allocation.USAGE_SCRIPT) is required.
     */
    void generateMipmaps();

    /**
     * Copy an array into part of this Allocation.
     * @param[in] off offset of first Element to be overwritten
     * @param[in] count number of Elements to copy
     * @param[in] data array from which to copy
     */
    void copy1DRangeFrom(uint32_t off, size_t count, const void *data);

    /**
     * Copy part of an Allocation into part of this Allocation.
     * @param[in] off offset of first Element to be overwritten
     * @param[in] count number of Elements to copy
     * @param[in] data Allocation from which to copy
     * @param[in] dataOff offset of first Element in data to copy
     */
    void copy1DRangeFrom(uint32_t off, size_t count, const sp<const Allocation>& data, uint32_t dataOff);

    /**
     * Copy an array into part of this Allocation.
     * @param[in] off offset of first Element to be overwritten
     * @param[in] count number of Elements to copy
     * @param[in] data array from which to copy
     */
    void copy1DRangeTo(uint32_t off, size_t count, void *data);

    /**
     * Copy entire array to an Allocation.
     * @param[in] data array from which to copy
     */
    void copy1DFrom(const void* data);

    /**
     * Copy entire Allocation to an array.
     * @param[in] data destination array
     */
    void copy1DTo(void* data);

    /**
     * Copy from an array into a rectangular region in this Allocation. The
     * array is assumed to be tightly packed.
     * @param[in] xoff X offset of region to update in this Allocation
     * @param[in] yoff Y offset of region to update in this Allocation
     * @param[in] w Width of region to update
     * @param[in] h Height of region to update
     * @param[in] data Array from which to copy
     */
    void copy2DRangeFrom(uint32_t xoff, uint32_t yoff, uint32_t w, uint32_t h,
                         const void *data);

    /**
     * Copy from this Allocation into a rectangular region in an array. The
     * array is assumed to be tightly packed.
     * @param[in] xoff X offset of region to copy from this Allocation
     * @param[in] yoff Y offset of region to copy from this Allocation
     * @param[in] w Width of region to update
     * @param[in] h Height of region to update
     * @param[in] data destination array
     */
    void copy2DRangeTo(uint32_t xoff, uint32_t yoff, uint32_t w, uint32_t h,
                       void *data);

    /**
     * Copy from an Allocation into a rectangular region in this Allocation.
     * @param[in] xoff X offset of region to update in this Allocation
     * @param[in] yoff Y offset of region to update in this Allocation
     * @param[in] w Width of region to update
     * @param[in] h Height of region to update
     * @param[in] data Allocation from which to copy
     * @param[in] dataXoff X offset of region to copy from in data
     * @param[in] dataYoff Y offset of region to copy from in data
     */
    void copy2DRangeFrom(uint32_t xoff, uint32_t yoff, uint32_t w, uint32_t h,
                         const sp<const Allocation>& data, uint32_t dataXoff, uint32_t dataYoff);

    /**
     * Copy from a strided array into a rectangular region in this Allocation.
     * @param[in] xoff X offset of region to update in this Allocation
     * @param[in] yoff Y offset of region to update in this Allocation
     * @param[in] w Width of region to update
     * @param[in] h Height of region to update
     * @param[in] data array from which to copy
     * @param[in] stride stride of data in bytes
     */
    void copy2DStridedFrom(uint32_t xoff, uint32_t yoff, uint32_t w, uint32_t h,
                           const void *data, size_t stride);

    /**
     * Copy from a strided array into this Allocation.
     * @param[in] data array from which to copy
     * @param[in] stride stride of data in bytes
     */
    void copy2DStridedFrom(const void *data, size_t stride);

    /**
     * Copy from a rectangular region in this Allocation into a strided array.
     * @param[in] xoff X offset of region to update in this Allocation
     * @param[in] yoff Y offset of region to update in this Allocation
     * @param[in] w Width of region to update
     * @param[in] h Height of region to update
     * @param[in] data destination array
     * @param[in] stride stride of data in bytes
     */
    void copy2DStridedTo(uint32_t xoff, uint32_t yoff, uint32_t w, uint32_t h,
                         void *data, size_t stride);

    /**
     * Copy this Allocation into a strided array.
     * @param[in] data destination array
     * @param[in] stride stride of data in bytes
     */
    void copy2DStridedTo(void *data, size_t stride);


    /**
     * Copy from an array into a 3D region in this Allocation. The
     * array is assumed to be tightly packed.
     * @param[in] xoff X offset of region to update in this Allocation
     * @param[in] yoff Y offset of region to update in this Allocation
     * @param[in] zoff Z offset of region to update in this Allocation
     * @param[in] w Width of region to update
     * @param[in] h Height of region to update
     * @param[in] d Depth of region to update
     * @param[in] data Array from which to copy
     */
    void copy3DRangeFrom(uint32_t xoff, uint32_t yoff, uint32_t zoff, uint32_t w,
                         uint32_t h, uint32_t d, const void* data);

    /**
     * Copy from an Allocation into a 3D region in this Allocation.
     * @param[in] xoff X offset of region to update in this Allocation
     * @param[in] yoff Y offset of region to update in this Allocation
     * @param[in] zoff Z offset of region to update in this Allocation
     * @param[in] w Width of region to update
     * @param[in] h Height of region to update
     * @param[in] d Depth of region to update
     * @param[in] data Allocation from which to copy
     * @param[in] dataXoff X offset of region in data to copy from
     * @param[in] dataYoff Y offset of region in data to copy from
     * @param[in] dataZoff Z offset of region in data to copy from
     */
    void copy3DRangeFrom(uint32_t xoff, uint32_t yoff, uint32_t zoff,
                         uint32_t w, uint32_t h, uint32_t d,
                         const sp<const Allocation>& data,
                         uint32_t dataXoff, uint32_t dataYoff, uint32_t dataZoff);

    /**
     * Copy a 3D region in this Allocation into an array. The
     * array is assumed to be tightly packed.
     * @param[in] xoff X offset of region to update in this Allocation
     * @param[in] yoff Y offset of region to update in this Allocation
     * @param[in] zoff Z offset of region to update in this Allocation
     * @param[in] w Width of region to update
     * @param[in] h Height of region to update
     * @param[in] d Depth of region to update
     * @param[in] data Array from which to copy
     */
    void copy3DRangeTo(uint32_t xoff, uint32_t yoff, uint32_t zoff, uint32_t w,
                         uint32_t h, uint32_t d, void* data);

    /**
     * Creates an Allocation for use by scripts with a given Type.
     * @param[in] rs Context to which the Allocation will belong
     * @param[in] type Type of the Allocation
     * @param[in] mipmaps desired mipmap behavior for the Allocation
     * @param[in] usage usage for the Allocation
     * @return new Allocation
     */
    static sp<Allocation> createTyped(const sp<RS>& rs, const sp<const Type>& type,
                                   RsAllocationMipmapControl mipmaps, uint32_t usage);

    /**
     * Creates an Allocation for use by scripts with a given Type and a backing pointer. For use
     * with RS_ALLOCATION_USAGE_SHARED.
     * @param[in] rs Context to which the Allocation will belong
     * @param[in] type Type of the Allocation
     * @param[in] mipmaps desired mipmap behavior for the Allocation
     * @param[in] usage usage for the Allocation
     * @param[in] pointer existing backing store to use for this Allocation if possible
     * @return new Allocation
     */
    static sp<Allocation> createTyped(const sp<RS>& rs, const sp<const Type>& type,
                                   RsAllocationMipmapControl mipmaps, uint32_t usage, void * pointer);

    /**
     * Creates an Allocation for use by scripts with a given Type with no mipmaps.
     * @param[in] rs Context to which the Allocation will belong
     * @param[in] type Type of the Allocation
     * @param[in] usage usage for the Allocation
     * @return new Allocation
     */
    static sp<Allocation> createTyped(const sp<RS>& rs, const sp<const Type>& type,
                                   uint32_t usage = RS_ALLOCATION_USAGE_SCRIPT);
    /**
     * Creates an Allocation with a specified number of given elements.
     * @param[in] rs Context to which the Allocation will belong
     * @param[in] e Element used in the Allocation
     * @param[in] count Number of elements of the Allocation
     * @param[in] usage usage for the Allocation
     * @return new Allocation
     */
    static sp<Allocation> createSized(const sp<RS>& rs, const sp<const Element>& e, size_t count,
                                   uint32_t usage = RS_ALLOCATION_USAGE_SCRIPT);

    /**
     * Creates a 2D Allocation with a specified number of given elements.
     * @param[in] rs Context to which the Allocation will belong
     * @param[in] e Element used in the Allocation
     * @param[in] x Width in Elements of the Allocation
     * @param[in] y Height of the Allocation
     * @param[in] usage usage for the Allocation
     * @return new Allocation
     */
    static sp<Allocation> createSized2D(const sp<RS>& rs, const sp<const Element>& e,
                                        size_t x, size_t y,
                                        uint32_t usage = RS_ALLOCATION_USAGE_SCRIPT);


    /**
     * Get the backing pointer for a USAGE_SHARED allocation.
     * @param[in] stride optional parameter. when non-NULL, will contain
     *   stride in bytes of a 2D Allocation
     * @return pointer to data
     */
    void * getPointer(size_t *stride = NULL);
};

 /**
  * An Element represents one item within an Allocation. An Element is roughly
  * equivalent to a C type in a RenderScript kernel. Elements may be basic
  * or complex. Some basic elements are:

  * - A single float value (equivalent to a float in a kernel)
  * - A four-element float vector (equivalent to a float4 in a kernel)
  * - An unsigned 32-bit integer (equivalent to an unsigned int in a kernel)
  * - A single signed 8-bit integer (equivalent to a char in a kernel)

  * Basic Elements are comprised of a Element.DataType and a
  * Element.DataKind. The DataType encodes C type information of an Element,
  * while the DataKind encodes how that Element should be interpreted by a
  * Sampler. Note that Allocation objects with DataKind USER cannot be used as
  * input for a Sampler. In general, Allocation objects that are intended for
  * use with a Sampler should use bitmap-derived Elements such as
  * Element::RGBA_8888.
 */


class Element : public BaseObj {
public:
    bool isComplex();

    /**
     * Elements could be simple, such as an int or a float, or a structure with
     * multiple sub-elements, such as a collection of floats, float2,
     * float4. This function returns zero for simple elements or the number of
     * sub-elements otherwise.
     * @return number of sub-elements
     */
    size_t getSubElementCount() {
        return mVisibleElementMapSize;
    }

    /**
     * For complex Elements, this returns the sub-element at a given index.
     * @param[in] index index of sub-element
     * @return sub-element
     */
    sp<const Element> getSubElement(uint32_t index);

    /**
     * For complex Elements, this returns the name of the sub-element at a given
     * index.
     * @param[in] index index of sub-element
     * @return name of sub-element
     */
    const char * getSubElementName(uint32_t index);

    /**
     * For complex Elements, this returns the size of the sub-element at a given
     * index.
     * @param[in] index index of sub-element
     * @return size of sub-element
     */
    size_t getSubElementArraySize(uint32_t index);

    /**
     * Returns the location of a sub-element within a complex Element.
     * @param[in] index index of sub-element
     * @return offset in bytes
     */
    uint32_t getSubElementOffsetBytes(uint32_t index);

    /**
     * Returns the data type used for the Element.
     * @return data type
     */
    RsDataType getDataType() const {
        return mType;
    }

    /**
     * Returns the data kind used for the Element.
     * @return data kind
     */
    RsDataKind getDataKind() const {
        return mKind;
    }

    /**
     * Returns the size in bytes of the Element.
     * @return size in bytes
     */
    size_t getSizeBytes() const {
        return mSizeBytes;
    }

    /**
     * Returns the number of vector components for this Element.
     * @return number of vector components
     */
    uint32_t getVectorSize() const {
        return mVectorSize;
    }

    /**
     * Utility function for returning an Element containing a single bool.
     * @param[in] rs RenderScript context
     * @return Element
     */
    static sp<const Element> BOOLEAN(const sp<RS> &rs);
    /**
     * Utility function for returning an Element containing a single unsigned char.
     * @param[in] rs RenderScript context
     * @return Element
     */
    static sp<const Element> U8(const sp<RS> &rs);
    /**
     * Utility function for returning an Element containing a single signed char.
     * @param[in] rs RenderScript context
     * @return Element
     */
    static sp<const Element> I8(const sp<RS> &rs);
    /**
     * Utility function for returning an Element containing a single unsigned short.
     * @param[in] rs RenderScript context
     * @return Element
     */
    static sp<const Element> U16(const sp<RS> &rs);
    /**
     * Utility function for returning an Element containing a single signed short.
     * @param[in] rs RenderScript context
     * @return Element
     */
    static sp<const Element> I16(const sp<RS> &rs);
    /**
     * Utility function for returning an Element containing a single unsigned int.
     * @param[in] rs RenderScript context
     * @return Element
     */
    static sp<const Element> U32(const sp<RS> &rs);
    /**
     * Utility function for returning an Element containing a single signed int.
     * @param[in] rs RenderScript context
     * @return Element
     */
    static sp<const Element> I32(const sp<RS> &rs);
    /**
     * Utility function for returning an Element containing a single unsigned long long.
     * @param[in] rs RenderScript context
     * @return Element
     */
    static sp<const Element> U64(const sp<RS> &rs);
    /**
     * Utility function for returning an Element containing a single signed long long.
     * @param[in] rs RenderScript context
     * @return Element
     */
    static sp<const Element> I64(const sp<RS> &rs);
    /**
     * Utility function for returning an Element containing a single half.
     * @param[in] rs RenderScript context
     * @return Element
     */
    static sp<const Element> F16(const sp<RS> &rs);
    /**
     * Utility function for returning an Element containing a single float.
     * @param[in] rs RenderScript context
     * @return Element
     */
    static sp<const Element> F32(const sp<RS> &rs);
    /**
     * Utility function for returning an Element containing a single double.
     * @param[in] rs RenderScript context
     * @return Element
     */
    static sp<const Element> F64(const sp<RS> &rs);
    /**
     * Utility function for returning an Element containing a single Element.
     * @param[in] rs RenderScript context
     * @return Element
     */
    static sp<const Element> ELEMENT(const sp<RS> &rs);
    /**
     * Utility function for returning an Element containing a single Type.
     * @param[in] rs RenderScript context
     * @return Element
     */
    static sp<const Element> TYPE(const sp<RS> &rs);
    /**
     * Utility function for returning an Element containing a single Allocation.
     * @param[in] rs RenderScript context
     * @return Element
     */
    static sp<const Element> ALLOCATION(const sp<RS> &rs);
    /**
     * Utility function for returning an Element containing a single Sampler.
     * @param[in] rs RenderScript context
     * @return Element
     */
    static sp<const Element> SAMPLER(const sp<RS> &rs);
    /**
     * Utility function for returning an Element containing a single Script.
     * @param[in] rs RenderScript context
     * @return Element
     */
    static sp<const Element> SCRIPT(const sp<RS> &rs);
    /**
     * Utility function for returning an Element containing an ALPHA_8 pixel.
     * @param[in] rs RenderScript context
     * @return Element
     */
    static sp<const Element> A_8(const sp<RS> &rs);
    /**
     * Utility function for returning an Element containing an RGB_565 pixel.
     * @param[in] rs RenderScript context
     * @return Element
     */
    static sp<const Element> RGB_565(const sp<RS> &rs);
    /**
     * Utility function for returning an Element containing an RGB_888 pixel.
     * @param[in] rs RenderScript context
     * @return Element
     */
    static sp<const Element> RGB_888(const sp<RS> &rs);
    /**
     * Utility function for returning an Element containing an RGBA_5551 pixel.
     * @param[in] rs RenderScript context
     * @return Element
     */
    static sp<const Element> RGBA_5551(const sp<RS> &rs);
    /**
     * Utility function for returning an Element containing an RGBA_4444 pixel.
     * @param[in] rs RenderScript context
     * @return Element
     */
    static sp<const Element> RGBA_4444(const sp<RS> &rs);
    /**
     * Utility function for returning an Element containing an RGBA_8888 pixel.
     * @param[in] rs RenderScript context
     * @return Element
     */
    static sp<const Element> RGBA_8888(const sp<RS> &rs);

    /**
     * Utility function for returning an Element containing a half2.
     * @param[in] rs RenderScript context
     * @return Element
     */
    static sp<const Element> F16_2(const sp<RS> &rs);
    /**
     * Utility function for returning an Element containing a half3.
     * @param[in] rs RenderScript context
     * @return Element
     */
    static sp<const Element> F16_3(const sp<RS> &rs);
    /**
     * Utility function for returning an Element containing a half4.
     * @param[in] rs RenderScript context
     * @return Element
     */
    static sp<const Element> F16_4(const sp<RS> &rs);

    /**
     * Utility function for returning an Element containing a float2.
     * @param[in] rs RenderScript context
     * @return Element
     */
    static sp<const Element> F32_2(const sp<RS> &rs);
    /**
     * Utility function for returning an Element containing a float3.
     * @param[in] rs RenderScript context
     * @return Element
     */
    static sp<const Element> F32_3(const sp<RS> &rs);
    /**
     * Utility function for returning an Element containing a float4.
     * @param[in] rs RenderScript context
     * @return Element
     */
    static sp<const Element> F32_4(const sp<RS> &rs);
    /**
     * Utility function for returning an Element containing a double2.
     * @param[in] rs RenderScript context
     * @return Element
     */
    static sp<const Element> F64_2(const sp<RS> &rs);
    /**
     * Utility function for returning an Element containing a double3.
     * @param[in] rs RenderScript context
     * @return Element
     */
    static sp<const Element> F64_3(const sp<RS> &rs);
    /**
     * Utility function for returning an Element containing a double4.
     * @param[in] rs RenderScript context
     * @return Element
     */
    static sp<const Element> F64_4(const sp<RS> &rs);
    /**
     * Utility function for returning an Element containing a uchar2.
     * @param[in] rs RenderScript context
     * @return Element
     */
    static sp<const Element> U8_2(const sp<RS> &rs);
    /**
     * Utility function for returning an Element containing a uchar3.
     * @param[in] rs RenderScript context
     * @return Element
     */
    static sp<const Element> U8_3(const sp<RS> &rs);
    /**
     * Utility function for returning an Element containing a uchar4.
     * @param[in] rs RenderScript context
     * @return Element
     */
    static sp<const Element> U8_4(const sp<RS> &rs);
    /**
     * Utility function for returning an Element containing a char2.
     * @param[in] rs RenderScript context
     * @return Element
     */
    static sp<const Element> I8_2(const sp<RS> &rs);
    /**
     * Utility function for returning an Element containing a char3.
     * @param[in] rs RenderScript context
     * @return Element
     */
    static sp<const Element> I8_3(const sp<RS> &rs);
    /**
     * Utility function for returning an Element containing a char4.
     * @param[in] rs RenderScript context
     * @return Element
     */
    static sp<const Element> I8_4(const sp<RS> &rs);
    /**
     * Utility function for returning an Element containing a ushort2.
     * @param[in] rs RenderScript context
     * @return Element
     */
    static sp<const Element> U16_2(const sp<RS> &rs);
    /**
     * Utility function for returning an Element containing a ushort3.
     * @param[in] rs RenderScript context
     * @return Element
     */
    static sp<const Element> U16_3(const sp<RS> &rs);
    /**
     * Utility function for returning an Element containing a ushort4.
     * @param[in] rs RenderScript context
     * @return Element
     */
    static sp<const Element> U16_4(const sp<RS> &rs);
    /**
     * Utility function for returning an Element containing a short2.
     * @param[in] rs RenderScript context
     * @return Element
     */
    static sp<const Element> I16_2(const sp<RS> &rs);
    /**
     * Utility function for returning an Element containing a short3.
     * @param[in] rs RenderScript context
     * @return Element
     */
    static sp<const Element> I16_3(const sp<RS> &rs);
    /**
     * Utility function for returning an Element containing a short4.
     * @param[in] rs RenderScript context
     * @return Element
     */
    static sp<const Element> I16_4(const sp<RS> &rs);
    /**
     * Utility function for returning an Element containing a uint2.
     * @param[in] rs RenderScript context
     * @return Element
     */
    static sp<const Element> U32_2(const sp<RS> &rs);
    /**
     * Utility function for returning an Element containing a uint3.
     * @param[in] rs RenderScript context
     * @return Element
     */
    static sp<const Element> U32_3(const sp<RS> &rs);
    /**
     * Utility function for returning an Element containing a uint4.
     * @param[in] rs RenderScript context
     * @return Element
     */
    static sp<const Element> U32_4(const sp<RS> &rs);
    /**
     * Utility function for returning an Element containing an int2.
     * @param[in] rs RenderScript context
     * @return Element
     */
    static sp<const Element> I32_2(const sp<RS> &rs);
    /**
     * Utility function for returning an Element containing an int3.
     * @param[in] rs RenderScript context
     * @return Element
     */
    static sp<const Element> I32_3(const sp<RS> &rs);
    /**
     * Utility function for returning an Element containing an int4.
     * @param[in] rs RenderScript context
     * @return Element
     */
    static sp<const Element> I32_4(const sp<RS> &rs);
    /**
     * Utility function for returning an Element containing a ulong2.
     * @param[in] rs RenderScript context
     * @return Element
     */
    static sp<const Element> U64_2(const sp<RS> &rs);
    /**
     * Utility function for returning an Element containing a ulong3.
     * @param[in] rs RenderScript context
     * @return Element
     */
    static sp<const Element> U64_3(const sp<RS> &rs);
    /**
     * Utility function for returning an Element containing a ulong4.
     * @param[in] rs RenderScript context
     * @return Element
     */
    static sp<const Element> U64_4(const sp<RS> &rs);
    /**
     * Utility function for returning an Element containing a long2.
     * @param[in] rs RenderScript context
     * @return Element
     */
    static sp<const Element> I64_2(const sp<RS> &rs);
    /**
     * Utility function for returning an Element containing a long3.
     * @param[in] rs RenderScript context
     * @return Element
     */
    static sp<const Element> I64_3(const sp<RS> &rs);
    /**
     * Utility function for returning an Element containing a long4.
     * @param[in] rs RenderScript context
     * @return Element
     */
    static sp<const Element> I64_4(const sp<RS> &rs);
    /**
     * Utility function for returning an Element containing a YUV pixel.
     * @param[in] rs RenderScript context
     * @return Element
     */
    static sp<const Element> YUV(const sp<RS> &rs);
    /**
     * Utility function for returning an Element containing an rs_matrix_4x4.
     * @param[in] rs RenderScript context
     * @return Element
     */
    static sp<const Element> MATRIX_4X4(const sp<RS> &rs);
    /**
     * Utility function for returning an Element containing an rs_matrix_3x3.
     * @param[in] rs RenderScript context
     * @return Element
     */
    static sp<const Element> MATRIX_3X3(const sp<RS> &rs);
    /**
     * Utility function for returning an Element containing an rs_matrix_2x2.
     * @param[in] rs RenderScript context
     * @return Element
     */
    static sp<const Element> MATRIX_2X2(const sp<RS> &rs);

    void updateFromNative();

    /**
     * Create an Element with a given DataType.
     * @param[in] rs RenderScript context
     * @param[in] dt data type
     * @return Element
     */
    static sp<const Element> createUser(const sp<RS>& rs, RsDataType dt);
    /**
     * Create a vector Element with the given DataType
     * @param[in] rs RenderScript
     * @param[in] dt DataType
     * @param[in] size vector size
     * @return Element
     */
    static sp<const Element> createVector(const sp<RS>& rs, RsDataType dt, uint32_t size);
    /**
     * Create an Element with a given DataType and DataKind.
     * @param[in] rs RenderScript context
     * @param[in] dt DataType
     * @param[in] dk DataKind
     * @return Element
     */
    static sp<const Element> createPixel(const sp<RS>& rs, RsDataType dt, RsDataKind dk);

    /**
     * Returns true if the Element can interoperate with this Element.
     * @param[in] e Element to compare
     * @return true if Elements can interoperate
     */
    bool isCompatible(const sp<const Element>&e) const;

    /**
     * Builder class for producing complex elements with matching field and name
     * pairs. The builder starts empty. The order in which elements are added is
     * retained for the layout in memory.
     */
    class Builder {
    private:
        RS* mRS;
        size_t mElementsCount;
        size_t mElementsVecSize;
        sp<const Element> * mElements;
        char ** mElementNames;
        size_t * mElementNameLengths;
        uint32_t * mArraySizes;
        bool mSkipPadding;

    public:
        explicit Builder(sp<RS> rs);
        ~Builder();
        void add(const sp<const Element>& e, const char * name, uint32_t arraySize = 1);
        sp<const Element> create();
    };

protected:
    friend class Type;
    Element(void *id, sp<RS> rs,
            sp<const Element> * elements,
            size_t elementCount,
            const char ** elementNames,
            size_t * elementNameLengths,
            uint32_t * arraySizes);
    Element(void *id, sp<RS> rs, RsDataType dt, RsDataKind dk, bool norm, uint32_t size);
    Element(void *id, sp<RS> rs);
    explicit Element(sp<RS> rs);
    virtual ~Element();

private:
    void updateVisibleSubElements();

    size_t mElementsCount;
    size_t mVisibleElementMapSize;

    sp<const Element> * mElements;
    char ** mElementNames;
    size_t * mElementNameLengths;
    uint32_t * mArraySizes;
    uint32_t * mVisibleElementMap;
    uint32_t * mOffsetInBytes;

    RsDataType mType;
    RsDataKind mKind;
    bool mNormalized;
    size_t mSizeBytes;
    size_t mVectorSize;
};

class FieldPacker {
protected:
    unsigned char* mData;
    size_t mPos;
    size_t mLen;

public:
    explicit FieldPacker(size_t len)
        : mPos(0), mLen(len) {
            mData = new unsigned char[len];
        }

    virtual ~FieldPacker() {
        delete [] mData;
    }

    void align(size_t v) {
        if ((v & (v - 1)) != 0) {
            //            ALOGE("Non-power-of-two alignment: %zu", v);
            return;
        }

        while ((mPos & (v - 1)) != 0) {
            mData[mPos++] = 0;
        }
    }

    void reset() {
        mPos = 0;
    }

    void reset(size_t i) {
        if (i >= mLen) {
            //            ALOGE("Out of bounds: i (%zu) >= len (%zu)", i, mLen);
            return;
        }
        mPos = i;
    }

    void skip(size_t i) {
        size_t res = mPos + i;
        if (res > mLen) {
            //            ALOGE("Exceeded buffer length: i (%zu) > len (%zu)", i, mLen);
            return;
        }
        mPos = res;
    }

    void* getData() const {
        return mData;
    }

    size_t getLength() const {
        return mLen;
    }

    template <typename T>
        void add(T t) {
        align(sizeof(t));
        if (mPos + sizeof(t) <= mLen) {
            memcpy(&mData[mPos], &t, sizeof(t));
            mPos += sizeof(t);
        }
    }

    /*
      void add(rs_matrix4x4 m) {
      for (size_t i = 0; i < 16; i++) {
      add(m.m[i]);
      }
      }

      void add(rs_matrix3x3 m) {
      for (size_t i = 0; i < 9; i++) {
      add(m.m[i]);
      }
      }

      void add(rs_matrix2x2 m) {
      for (size_t i = 0; i < 4; i++) {
      add(m.m[i]);
      }
      }
    */

    void add(const sp<BaseObj>& obj) {
        if (obj != NULL) {
            add((uint32_t) (uintptr_t) obj->getID());
        } else {
            add((uint32_t) 0);
        }
    }
};

/**
 * A Type describes the Element and dimensions used for an Allocation or a
 * parallel operation.
 *
 * A Type always includes an Element and an X dimension. A Type may be
 * multidimensional, up to three dimensions. A nonzero value in the Y or Z
 * dimensions indicates that the dimension is present. Note that a Type with
 * only a given X dimension and a Type with the same X dimension but Y = 1 are
 * not equivalent.
 *
 * A Type also supports inclusion of level of detail (LOD) or cube map
 * faces. LOD and cube map faces are booleans to indicate present or not
 * present.
 *
 * A Type also supports YUV format information to support an Allocation in a YUV
 * format. The YUV formats supported are RS_YUV_YV12 and RS_YUV_NV21.
 */
class Type : public BaseObj {
protected:
    friend class Allocation;

    uint32_t mDimX;
    uint32_t mDimY;
    uint32_t mDimZ;
    RsYuvFormat mYuvFormat;
    bool mDimMipmaps;
    bool mDimFaces;
    size_t mElementCount;
    sp<const Element> mElement;

    Type(void *id, sp<RS> rs);

    void calcElementCount();
    virtual void updateFromNative();

public:

    /**
     * Returns the YUV format.
     * @return YUV format of the Allocation
     */
    RsYuvFormat getYuvFormat() const {
        return mYuvFormat;
    }

    /**
     * Returns the Element of the Allocation.
     * @return YUV format of the Allocation
     */
    sp<const Element> getElement() const {
        return mElement;
    }

    /**
     * Returns the X dimension of the Allocation.
     * @return X dimension of the allocation
     */
    uint32_t getX() const {
        return mDimX;
    }

    /**
     * Returns the Y dimension of the Allocation.
     * @return Y dimension of the allocation
     */
    uint32_t getY() const {
        return mDimY;
    }

    /**
     * Returns the Z dimension of the Allocation.
     * @return Z dimension of the allocation
     */
    uint32_t getZ() const {
        return mDimZ;
    }

    /**
     * Returns true if the Allocation has mipmaps.
     * @return true if the Allocation has mipmaps
     */
    bool hasMipmaps() const {
        return mDimMipmaps;
    }

    /**
     * Returns true if the Allocation is a cube map
     * @return true if the Allocation is a cube map
     */
    bool hasFaces() const {
        return mDimFaces;
    }

    /**
     * Returns number of accessible Elements in the Allocation
     * @return number of accessible Elements in the Allocation
     */
    size_t getCount() const {
        return mElementCount;
    }

    /**
     * Returns size in bytes of all Elements in the Allocation
     * @return size in bytes of all Elements in the Allocation
     */
    size_t getSizeBytes() const {
        return mElementCount * mElement->getSizeBytes();
    }

    /**
     * Creates a new Type with the given Element and dimensions.
     * @param[in] rs RenderScript context
     * @param[in] e Element
     * @param[in] dimX X dimension
     * @param[in] dimY Y dimension
     * @param[in] dimZ Z dimension
     * @return new Type
     */
    static sp<const Type> create(const sp<RS>& rs, const sp<const Element>& e, uint32_t dimX, uint32_t dimY, uint32_t dimZ);

    class Builder {
    protected:
        RS* mRS;
        uint32_t mDimX;
        uint32_t mDimY;
        uint32_t mDimZ;
        RsYuvFormat mYuvFormat;
        bool mDimMipmaps;
        bool mDimFaces;
        sp<const Element> mElement;

    public:
        Builder(sp<RS> rs, sp<const Element> e);

        void setX(uint32_t value);
        void setY(uint32_t value);
        void setZ(uint32_t value);
        void setYuvFormat(RsYuvFormat format);
        void setMipmaps(bool value);
        void setFaces(bool value);
        sp<const Type> create();
    };

};

/**
 * The parent class for all executable Scripts. This should not be used by applications.
 */
class Script : public BaseObj {
private:

protected:
    Script(void *id, sp<RS> rs);
    void forEach(uint32_t slot, const sp<const Allocation>& in, const sp<const Allocation>& out,
            const void *v, size_t) const;
    void bindAllocation(const sp<Allocation>& va, uint32_t slot) const;
    void setVar(uint32_t index, const void *, size_t len) const;
    void setVar(uint32_t index, const sp<const BaseObj>& o) const;
    void invoke(uint32_t slot, const void *v, size_t len) const;


    void invoke(uint32_t slot) const {
        invoke(slot, NULL, 0);
    }
    void setVar(uint32_t index, float v) const {
        setVar(index, &v, sizeof(v));
    }
    void setVar(uint32_t index, double v) const {
        setVar(index, &v, sizeof(v));
    }
    void setVar(uint32_t index, int32_t v) const {
        setVar(index, &v, sizeof(v));
    }
    void setVar(uint32_t index, uint32_t v) const {
        setVar(index, &v, sizeof(v));
    }
    void setVar(uint32_t index, int64_t v) const {
        setVar(index, &v, sizeof(v));
    }
    void setVar(uint32_t index, bool v) const {
        setVar(index, &v, sizeof(v));
    }

public:
    class FieldBase {
    protected:
        sp<const Element> mElement;
        sp<Allocation> mAllocation;

        void init(const sp<RS>& rs, uint32_t dimx, uint32_t usages = 0);

    public:
        sp<const Element> getElement() {
            return mElement;
        }

        sp<const Type> getType() {
            return mAllocation->getType();
        }

        sp<const Allocation> getAllocation() {
            return mAllocation;
        }

        //void updateAllocation();
    };
};

/**
 * The parent class for all user-defined scripts. This is intended to be used by auto-generated code only.
 */
class ScriptC : public Script {
protected:
    ScriptC(sp<RS> rs,
            const void *codeTxt, size_t codeLength,
            const char *cachedName, size_t cachedNameLength,
            const char *cacheDir, size_t cacheDirLength);

};

/**
 * The parent class for all script intrinsics. Intrinsics provide highly optimized implementations of
 * basic functions. This is not intended to be used directly.
 */
class ScriptIntrinsic : public Script {
 protected:
    sp<const Element> mElement;
    ScriptIntrinsic(sp<RS> rs, int id, sp<const Element> e);
    virtual ~ScriptIntrinsic();
};

/**
 * Intrinsic for converting RGB to RGBA by using a 3D lookup table. The incoming
 * r,g,b values are use as normalized x,y,z coordinates into a 3D
 * allocation. The 8 nearest values are sampled and linearly interpolated. The
 * result is placed in the output.
 */
class ScriptIntrinsic3DLUT : public ScriptIntrinsic {
 private:
    ScriptIntrinsic3DLUT(sp<RS> rs, sp<const Element> e);
 public:
    /**
     * Supported Element types are U8_4. Default lookup table is identity.
     * @param[in] rs RenderScript context
     * @param[in] e Element
     * @return new ScriptIntrinsic
     */
    static sp<ScriptIntrinsic3DLUT> create(const sp<RS>& rs, const sp<const Element>& e);

    /**
     * Launch the intrinsic.
     * @param[in] ain input Allocation
     * @param[in] aout output Allocation
     */
    void forEach(const sp<Allocation>& ain, const sp<Allocation>& aout);

    /**
     * Sets the lookup table. The lookup table must use the same Element as the
     * intrinsic.
     * @param[in] lut new lookup table
     */
    void setLUT(const sp<Allocation>& lut);
};


/**
 * Intrinsic kernel provides high performance RenderScript APIs to BLAS.
 *
 * The BLAS (Basic Linear Algebra Subprograms) are routines that provide standard
 * building blocks for performing basic vector and matrix operations.
 *
 * For detailed description of BLAS, please refer to http://www.netlib.org/blas/
 *
 **/
class ScriptIntrinsicBLAS : public ScriptIntrinsic {
 private:
    ScriptIntrinsicBLAS(sp<RS> rs, sp<const Element> e);
 public:
    /**
     * Create an intrinsic to access BLAS subroutines.
     *
     * @param rs The RenderScript context
     * @return ScriptIntrinsicBLAS
     */
    static sp<ScriptIntrinsicBLAS> create(const sp<RS>& rs);

    /**
     * SGEMV performs one of the matrix-vector operations
     * y := alpha*A*x + beta*y   or   y := alpha*A**T*x + beta*y
     *
     * Details: http://www.netlib.org/lapack/explore-html/db/d58/sgemv_8f.html
     *
     * @param TransA The type of transpose applied to matrix A.
     * @param alpha The scalar alpha.
     * @param A The input allocation contains matrix A, supported elements type: {Element#F32}.
     * @param X The input allocation contains vector x, supported elements type: {Element#F32}.
     * @param incX The increment for the elements of vector x, must be larger than zero.
     * @param beta The scalar beta.
     * @param Y The input allocation contains vector y, supported elements type: {Element#F32}.
     * @param incY The increment for the elements of vector y, must be larger than zero.
     */
    void SGEMV(RsBlasTranspose TransA,
               float alpha, const sp<Allocation>& A, const sp<Allocation>& X, int incX,
               float beta, const sp<Allocation>& Y, int incY);

    /**
     * DGEMV performs one of the matrix-vector operations
     * y := alpha*A*x + beta*y   or   y := alpha*A**T*x + beta*y
     *
     * Details: http://www.netlib.org/lapack/explore-html/dc/da8/dgemv_8f.html
     *
     * @param TransA The type of transpose applied to matrix A.
     * @param alpha The scalar alpha.
     * @param A The input allocation contains matrix A, supported elements type: {Element#F64}.
     * @param X The input allocation contains vector x, supported elements type: {Element#F64}.
     * @param incX The increment for the elements of vector x, must be larger than zero.
     * @param beta The scalar beta.
     * @param Y The input allocation contains vector y, supported elements type: {Element#F64}.
     * @param incY The increment for the elements of vector y, must be larger than zero.
     */
    void DGEMV(RsBlasTranspose TransA,
               double alpha, const sp<Allocation>& A, const sp<Allocation>& X, int incX,
               double beta, const sp<Allocation>& Y, int incY);

    /**
     * CGEMV performs one of the matrix-vector operations
     * y := alpha*A*x + beta*y   or   y := alpha*A**T*x + beta*y   or   y := alpha*A**H*x + beta*y
     *
     * Details: http://www.netlib.org/lapack/explore-html/d4/d8a/cgemv_8f.html
     *
     * @param TransA The type of transpose applied to matrix A.
     * @param alpha The scalar alpha.
     * @param A The input allocation contains matrix A, supported elements type: {Element#F32_2}.
     * @param X The input allocation contains vector x, supported elements type: {Element#F32_2}.
     * @param incX The increment for the elements of vector x, must be larger than zero.
     * @param beta The scalar beta.
     * @param Y The input allocation contains vector y, supported elements type: {Element#F32_2}.
     * @param incY The increment for the elements of vector y, must be larger than zero.
     */
    void CGEMV(RsBlasTranspose TransA,
               Float2 alpha, const sp<Allocation>& A, const sp<Allocation>& X, int incX,
               Float2 beta, const sp<Allocation>& Y, int incY);

    /**
     * ZGEMV performs one of the matrix-vector operations
     * y := alpha*A*x + beta*y   or   y := alpha*A**T*x + beta*y   or   y := alpha*A**H*x + beta*y
     *
     * Details: http://www.netlib.org/lapack/explore-html/db/d40/zgemv_8f.html
     *
     * @param TransA The type of transpose applied to matrix A.
     * @param alpha The scalar alpha.
     * @param A The input allocation contains matrix A, supported elements type: {Element#F64_2}.
     * @param X The input allocation contains vector x, supported elements type: {Element#F64_2}.
     * @param incX The increment for the elements of vector x, must be larger than zero.
     * @param beta The scalar beta.
     * @param Y The input allocation contains vector y, supported elements type: {Element#F64_2}.
     * @param incY The increment for the elements of vector y, must be larger than zero.
     */
    void ZGEMV(RsBlasTranspose TransA,
               Double2 alpha, const sp<Allocation>& A, const sp<Allocation>& X, int incX,
               Double2 beta, const sp<Allocation>& Y, int incY);

    /**
     * SGBMV performs one of the matrix-vector operations
     * y := alpha*A*x + beta*y   or   y := alpha*A**T*x + beta*y
     *
     * Details: http://www.netlib.org/lapack/explore-html/d6/d46/sgbmv_8f.html
     *
     * Note: For a M*N matrix, the input Allocation should also be of size M*N (dimY = M, dimX = N),
     *       but only the region M*(KL+KU+1) will be referenced. The following subroutine can is an
     *       example showing how to convert the original matrix 'a' to row-based band matrix 'b'.
     *           for i in range(0, m):
     *              for j in range(max(0, i-kl), min(i+ku+1, n)):
     *                  b[i, j-i+kl] = a[i, j]
     *
     * @param TransA The type of transpose applied to matrix A.
     * @param KL The number of sub-diagonals of the matrix A.
     * @param KU The number of super-diagonals of the matrix A.
     * @param alpha The scalar alpha.
     * @param A The input allocation contains the band matrix A, supported elements type: {Element#F32}.
     * @param X The input allocation contains vector x, supported elements type: {Element#F32}.
     * @param incX The increment for the elements of vector x, must be larger than zero.
     * @param beta The scalar beta.
     * @param Y The input allocation contains vector y, supported elements type: {Element#F32}.
     * @param incY The increment for the elements of vector y, must be larger than zero.
     */
    void SGBMV(RsBlasTranspose TransA,
               int KL, int KU, float alpha, const sp<Allocation>& A, const sp<Allocation>& X, int incX,
               float beta, const sp<Allocation>& Y, int incY);

    /**
     * DGBMV performs one of the matrix-vector operations
     * y := alpha*A*x + beta*y   or   y := alpha*A**T*x + beta*y
     *
     * Details: http://www.netlib.org/lapack/explore-html/d2/d3f/dgbmv_8f.html
     *
     * Note: For a M*N matrix, the input Allocation should also be of size M*N (dimY = M, dimX = N),
     *       but only the region M*(KL+KU+1) will be referenced. The following subroutine can is an
     *       example showing how to convert the original matrix 'a' to row-based band matrix 'b'.
     *           for i in range(0, m):
     *              for j in range(max(0, i-kl), min(i+ku+1, n)):
     *                  b[i, j-i+kl] = a[i, j]
     *
     * @param TransA The type of transpose applied to matrix A.
     * @param KL The number of sub-diagonals of the matrix A.
     * @param KU The number of super-diagonals of the matrix A.
     * @param alpha The scalar alpha.
     * @param A The input allocation contains the band matrix A, supported elements type: {Element#F64}.
     * @param X The input allocation contains vector x, supported elements type: {Element#F64}.
     * @param incX The increment for the elements of vector x, must be larger than zero.
     * @param beta The scalar beta.
     * @param Y The input allocation contains vector y, supported elements type: {Element#F64}.
     * @param incY The increment for the elements of vector y, must be larger than zero.
     */
    void DGBMV(RsBlasTranspose TransA,
               int KL, int KU, double alpha, const sp<Allocation>& A, const sp<Allocation>& X,
               int incX, double beta, const sp<Allocation>& Y, int incY);

    /**
     * CGBMV performs one of the matrix-vector operations
     * y := alpha*A*x + beta*y   or   y := alpha*A**T*x + beta*y   or   y := alpha*A**H*x + beta*y
     *
     * Details: http://www.netlib.org/lapack/explore-html/d0/d75/cgbmv_8f.html
     *
     * Note: For a M*N matrix, the input Allocation should also be of size M*N (dimY = M, dimX = N),
     *       but only the region M*(KL+KU+1) will be referenced. The following subroutine can is an
     *       example showing how to convert the original matrix 'a' to row-based band matrix 'b'.
     *           for i in range(0, m):
     *              for j in range(max(0, i-kl), min(i+ku+1, n)):
     *                  b[i, j-i+kl] = a[i, j]
     *
     * @param TransA The type of transpose applied to matrix A.
     * @param KL The number of sub-diagonals of the matrix A.
     * @param KU The number of super-diagonals of the matrix A.
     * @param alpha The scalar alpha.
     * @param A The input allocation contains the band matrix A, supported elements type: {Element#F32_2}.
     * @param X The input allocation contains vector x, supported elements type: {Element#F32_2}.
     * @param incX The increment for the elements of vector x, must be larger than zero.
     * @param beta The scalar beta.
     * @param Y The input allocation contains vector y, supported elements type: {Element#F32_2}.
     * @param incY The increment for the elements of vector y, must be larger than zero.
     */
    void CGBMV(RsBlasTranspose TransA,
               int KL, int KU, Float2 alpha, const sp<Allocation>& A, const sp<Allocation>& X,
               int incX, Float2 beta, const sp<Allocation>& Y, int incY);

    /**
     * ZGBMV performs one of the matrix-vector operations
     * y := alpha*A*x + beta*y   or   y := alpha*A**T*x + beta*y   or   y := alpha*A**H*x + beta*y
     *
     * Details: http://www.netlib.org/lapack/explore-html/d9/d46/zgbmv_8f.html
     *
     * Note: For a M*N matrix, the input Allocation should also be of size M*N (dimY = M, dimX = N),
     *       but only the region M*(KL+KU+1) will be referenced. The following subroutine can is an
     *       example showing how to convert the original matrix 'a' to row-based band matrix 'b'.
     *           for i in range(0, m):
     *              for j in range(max(0, i-kl), min(i+ku+1, n)):
     *                  b[i, j-i+kl] = a[i, j]
     *
     * @param TransA The type of transpose applied to matrix A.
     * @param KL The number of sub-diagonals of the matrix A.
     * @param KU The number of super-diagonals of the matrix A.
     * @param alpha The scalar alpha.
     * @param A The input allocation contains the band matrix A, supported elements type: {Element#F64_2}.
     * @param X The input allocation contains vector x, supported elements type: {Element#F64_2}.
     * @param incX The increment for the elements of vector x, must be larger than zero.
     * @param beta The scalar beta.
     * @param Y The input allocation contains vector y, supported elements type: {Element#F64_2}.
     * @param incY The increment for the elements of vector y, must be larger than zero.
     */
    void ZGBMV(RsBlasTranspose TransA,
               int KL, int KU, Double2 alpha, const sp<Allocation>& A, const sp<Allocation>& X, int incX,
               Double2 beta, const sp<Allocation>& Y, int incY);

    /**
     * STRMV performs one of the matrix-vector operations
     * x := A*x   or   x := A**T*x
     *
     * Details: http://www.netlib.org/lapack/explore-html/de/d45/strmv_8f.html
     *
     * @param Uplo Specifies whether the matrix is an upper or lower triangular matrix.
     * @param TransA The type of transpose applied to matrix A.
     * @param Diag Specifies whether or not A is unit triangular.
     * @param A The input allocation contains matrix A, supported elements type: {Element#F32}.
     * @param X The input allocation contains vector x, supported elements type: {Element#F32}.
     * @param incX The increment for the elements of vector x, must be larger than zero.
     */
    void STRMV(RsBlasUplo Uplo, RsBlasTranspose TransA, RsBlasDiag Diag,
               const sp<Allocation>& A, const sp<Allocation>& X, int incX);

    /**
     * DTRMV performs one of the matrix-vector operations
     * x := A*x   or   x := A**T*x
     *
     * Details: http://www.netlib.org/lapack/explore-html/dc/d7e/dtrmv_8f.html
     *
     * @param Uplo Specifies whether the matrix is an upper or lower triangular matrix.
     * @param TransA The type of transpose applied to matrix A.
     * @param Diag Specifies whether or not A is unit triangular.
     * @param A The input allocation contains matrix A, supported elements type: {Element#F64}.
     * @param X The input allocation contains vector x, supported elements type: {Element#F64}.
     * @param incX The increment for the elements of vector x, must be larger than zero.
     */
    void DTRMV(RsBlasUplo Uplo, RsBlasTranspose TransA, RsBlasDiag Diag,
               const sp<Allocation>& A, const sp<Allocation>& X, int incX);

    /**
     * CTRMV performs one of the matrix-vector operations
     * x := A*x   or   x := A**T*x   or   x := A**H*x
     *
     * Details: http://www.netlib.org/lapack/explore-html/df/d78/ctrmv_8f.html
     *
     * @param Uplo Specifies whether the matrix is an upper or lower triangular matrix.
     * @param TransA The type of transpose applied to matrix A.
     * @param Diag Specifies whether or not A is unit triangular.
     * @param A The input allocation contains matrix A, supported elements type: {Element#F32_2}.
     * @param X The input allocation contains vector x, supported elements type: {Element#F32_2}.
     * @param incX The increment for the elements of vector x, must be larger than zero.
     */
    void CTRMV(RsBlasUplo Uplo, RsBlasTranspose TransA, RsBlasDiag Diag,
               const sp<Allocation>& A, const sp<Allocation>& X, int incX);

    /**
     * ZTRMV performs one of the matrix-vector operations
     * x := A*x   or   x := A**T*x   or   x := A**H*x
     *
     * Details: http://www.netlib.org/lapack/explore-html/d0/dd1/ztrmv_8f.html
     *
     * @param Uplo Specifies whether the matrix is an upper or lower triangular matrix.
     * @param TransA The type of transpose applied to matrix A.
     * @param Diag Specifies whether or not A is unit triangular.
     * @param A The input allocation contains matrix A, supported elements type: {Element#F64_2}.
     * @param X The input allocation contains vector x, supported elements type: {Element#F64_2}.
     * @param incX The increment for the elements of vector x, must be larger than zero.
     */
    void ZTRMV(RsBlasUplo Uplo, RsBlasTranspose TransA, RsBlasDiag Diag,
               const sp<Allocation>& A, const sp<Allocation>& X, int incX);

    /**
     * STBMV performs one of the matrix-vector operations
     * x := A*x   or   x := A**T*x
     *
     * Details: http://www.netlib.org/lapack/explore-html/d6/d7d/stbmv_8f.html
     *
     * Note: For a N*N matrix, the input Allocation should also be of size N*N (dimY = N, dimX = N),
     *       but only the region N*(K+1) will be referenced. The following subroutine can is an
     *       example showing how to convert a UPPER trianglar matrix 'a' to row-based band matrix 'b'.
     *           for i in range(0, n):
     *              for j in range(i, min(i+k+1, n)):
     *                  b[i, j-i] = a[i, j]
     *
     * @param Uplo Specifies whether the matrix is an upper or lower triangular matrix.
     * @param TransA The type of transpose applied to matrix A.
     * @param Diag Specifies whether or not A is unit triangular.
     * @param K The number of off-diagonals of the matrix A
     * @param A The input allocation contains matrix A, supported elements type: {Element#F32}.
     * @param X The input allocation contains vector x, supported elements type: {Element#F32}.
     * @param incX The increment for the elements of vector x, must be larger than zero.
     */
    void STBMV(RsBlasUplo Uplo, RsBlasTranspose TransA, RsBlasDiag Diag,
               int K, const sp<Allocation>& A, const sp<Allocation>& X, int incX);

    /**
     * DTBMV performs one of the matrix-vector operations
     * x := A*x   or   x := A**T*x
     *
     * Details: http://www.netlib.org/lapack/explore-html/df/d29/dtbmv_8f.html
     *
     * Note: For a N*N matrix, the input Allocation should also be of size N*N (dimY = N, dimX = N),
     *       but only the region N*(K+1) will be referenced. The following subroutine can is an
     *       example showing how to convert a UPPER trianglar matrix 'a' to row-based band matrix 'b'.
     *           for i in range(0, n):
     *              for j in range(i, min(i+k+1, n)):
     *                  b[i, j-i] = a[i, j]
     *
     * @param Uplo Specifies whether the matrix is an upper or lower triangular matrix.
     * @param TransA The type of transpose applied to matrix A.
     * @param Diag Specifies whether or not A is unit triangular.
     * @param K The number of off-diagonals of the matrix A
     * @param A The input allocation contains matrix A, supported elements type: {Element#F64}.
     * @param X The input allocation contains vector x, supported elements type: {Element#F64}.
     * @param incX The increment for the elements of vector x, must be larger than zero.
     */
    void DTBMV(RsBlasUplo Uplo, RsBlasTranspose TransA, RsBlasDiag Diag,
               int K, const sp<Allocation>& A, const sp<Allocation>& X, int incX);

    /**
     * CTBMV performs one of the matrix-vector operations
     * x := A*x   or   x := A**T*x   or   x := A**H*x
     *
     * Details: http://www.netlib.org/lapack/explore-html/d3/dcd/ctbmv_8f.html
     *
     * Note: For a N*N matrix, the input Allocation should also be of size N*N (dimY = N, dimX = N),
     *       but only the region N*(K+1) will be referenced. The following subroutine can is an
     *       example showing how to convert a UPPER trianglar matrix 'a' to row-based band matrix 'b'.
     *           for i in range(0, n):
     *              for j in range(i, min(i+k+1, n)):
     *                  b[i, j-i] = a[i, j]
     *
     * @param Uplo Specifies whether the matrix is an upper or lower triangular matrix.
     * @param TransA The type of transpose applied to matrix A.
     * @param Diag Specifies whether or not A is unit triangular.
     * @param K The number of off-diagonals of the matrix A
     * @param A The input allocation contains matrix A, supported elements type: {Element#F32_2}.
     * @param X The input allocation contains vector x, supported elements type: {Element#F32_2}.
     * @param incX The increment for the elements of vector x, must be larger than zero.
     */
    void CTBMV(RsBlasUplo Uplo, RsBlasTranspose TransA, RsBlasDiag Diag,
               int K, const sp<Allocation>& A, const sp<Allocation>& X, int incX);

    /**
     * ZTBMV performs one of the matrix-vector operations
     * x := A*x   or   x := A**T*x   or   x := A**H*x
     *
     * Details: http://www.netlib.org/lapack/explore-html/d3/d39/ztbmv_8f.html
     *
     * Note: For a N*N matrix, the input Allocation should also be of size N*N (dimY = N, dimX = N),
     *       but only the region N*(K+1) will be referenced. The following subroutine can is an
     *       example showing how to convert a UPPER trianglar matrix 'a' to row-based band matrix 'b'.
     *           for i in range(0, n):
     *              for j in range(i, min(i+k+1, n)):
     *                  b[i, j-i] = a[i, j]
     *
     * @param Uplo Specifies whether the matrix is an upper or lower triangular matrix.
     * @param TransA The type of transpose applied to matrix A.
     * @param Diag Specifies whether or not A is unit triangular.
     * @param K The number of off-diagonals of the matrix A
     * @param A The input allocation contains matrix A, supported elements type: {Element#F64_2}.
     * @param X The input allocation contains vector x, supported elements type: {Element#F64_2}.
     * @param incX The increment for the elements of vector x, must be larger than zero.
     */
    void ZTBMV(RsBlasUplo Uplo, RsBlasTranspose TransA, RsBlasDiag Diag,
               int K, const sp<Allocation>& A, const sp<Allocation>& X, int incX);

    /**
     * STPMV performs one of the matrix-vector operations
     * x := A*x   or   x := A**T*x
     *
     * Details: http://www.netlib.org/lapack/explore-html/db/db1/stpmv_8f.html
     *
     * Note: For a N*N matrix, the input Allocation should be a 1D allocation of size dimX = N*(N+1)/2,
     *       The following subroutine can is an example showing how to convert a UPPER trianglar matrix
     *       'a' to packed matrix 'b'.
     *           k = 0
     *           for i in range(0, n):
     *              for j in range(i, n):
     *                  b[k++] = a[i, j]
     *
     * @param Uplo Specifies whether the matrix is an upper or lower triangular matrix.
     * @param TransA The type of transpose applied to matrix A.
     * @param Diag Specifies whether or not A is unit triangular.
     * @param Ap The input allocation contains packed matrix A, supported elements type: {Element#F32}.
     * @param X The input allocation contains vector x, supported elements type: {Element#F32}.
     * @param incX The increment for the elements of vector x, must be larger than zero.
     */
    void STPMV(RsBlasUplo Uplo, RsBlasTranspose TransA, RsBlasDiag Diag,
               const sp<Allocation>& Ap, const sp<Allocation>& X, int incX);

    /**
     * DTPMV performs one of the matrix-vector operations
     * x := A*x   or   x := A**T*x
     *
     * Details: http://www.netlib.org/lapack/explore-html/dc/dcd/dtpmv_8f.html
     *
     * Note: For a N*N matrix, the input Allocation should be a 1D allocation of size dimX = N*(N+1)/2,
     *       The following subroutine can is an example showing how to convert a UPPER trianglar matrix
     *       'a' to packed matrix 'b'.
     *           k = 0
     *           for i in range(0, n):
     *              for j in range(i, n):
     *                  b[k++] = a[i, j]
     *
     * @param Uplo Specifies whether the matrix is an upper or lower triangular matrix.
     * @param TransA The type of transpose applied to matrix A.
     * @param Diag Specifies whether or not A is unit triangular.
     * @param Ap The input allocation contains packed matrix A, supported elements type: {Element#F64}.
     * @param X The input allocation contains vector x, supported elements type: {Element#F64}.
     * @param incX The increment for the elements of vector x, must be larger than zero.
     */
    void DTPMV(RsBlasUplo Uplo, RsBlasTranspose TransA, RsBlasDiag Diag,
               const sp<Allocation>& Ap, const sp<Allocation>& X, int incX);

    /**
     * CTPMV performs one of the matrix-vector operations
     * x := A*x   or   x := A**T*x   or   x := A**H*x
     *
     * Details: http://www.netlib.org/lapack/explore-html/d4/dbb/ctpmv_8f.html
     *
     * Note: For a N*N matrix, the input Allocation should be a 1D allocation of size dimX = N*(N+1)/2,
     *       The following subroutine can is an example showing how to convert a UPPER trianglar matrix
     *       'a' to packed matrix 'b'.
     *           k = 0
     *           for i in range(0, n):
     *              for j in range(i, n):
     *                  b[k++] = a[i, j]
     *
     * @param Uplo Specifies whether the matrix is an upper or lower triangular matrix.
     * @param TransA The type of transpose applied to matrix A.
     * @param Diag Specifies whether or not A is unit triangular.
     * @param Ap The input allocation contains packed matrix A, supported elements type: {Element#F32_2}.
     * @param X The input allocation contains vector x, supported elements type: {Element#F32_2}.
     * @param incX The increment for the elements of vector x, must be larger than zero.
     */
    void CTPMV(RsBlasUplo Uplo, RsBlasTranspose TransA, RsBlasDiag Diag,
               const sp<Allocation>& Ap, const sp<Allocation>& X, int incX);

    /**
     * ZTPMV performs one of the matrix-vector operations
     * x := A*x   or   x := A**T*x   or   x := A**H*x
     *
     * Details: http://www.netlib.org/lapack/explore-html/d2/d9e/ztpmv_8f.html
     *
     * Note: For a N*N matrix, the input Allocation should be a 1D allocation of size dimX = N*(N+1)/2,
     *       The following subroutine can is an example showing how to convert a UPPER trianglar matrix
     *       'a' to packed matrix 'b'.
     *           k = 0
     *           for i in range(0, n):
     *              for j in range(i, n):
     *                  b[k++] = a[i, j]
     *
     * @param Uplo Specifies whether the matrix is an upper or lower triangular matrix.
     * @param TransA The type of transpose applied to matrix A.
     * @param Diag Specifies whether or not A is unit triangular.
     * @param Ap The input allocation contains packed matrix A, supported elements type: {Element#F64_2}.
     * @param X The input allocation contains vector x, supported elements type: {Element#F64_2}.
     * @param incX The increment for the elements of vector x, must be larger than zero.
     */
    void ZTPMV(RsBlasUplo Uplo, RsBlasTranspose TransA, RsBlasDiag Diag,
               const sp<Allocation>& Ap, const sp<Allocation>& X, int incX);

    /**
     * STRSV solves one of the systems of equations
     * A*x = b   or   A**T*x = b
     *
     * Details: http://www.netlib.org/lapack/explore-html/d0/d2a/strsv_8f.html
     *
     * @param Uplo Specifies whether the matrix is an upper or lower triangular matrix.
     * @param TransA The type of transpose applied to matrix A.
     * @param Diag Specifies whether or not A is unit triangular.
     * @param A The input allocation contains matrix A, supported elements type: {Element#F32}.
     * @param X The input allocation contains vector x, supported elements type: {Element#F32}.
     * @param incX The increment for the elements of vector x, must be larger than zero.
     */
    void STRSV(RsBlasUplo Uplo, RsBlasTranspose TransA, RsBlasDiag Diag,
               const sp<Allocation>& A, const sp<Allocation>& X, int incX);

    /**
     * DTRSV solves one of the systems of equations
     * A*x = b   or   A**T*x = b
     *
     * Details: http://www.netlib.org/lapack/explore-html/d6/d96/dtrsv_8f.html
     *
     * @param Uplo Specifies whether the matrix is an upper or lower triangular matrix.
     * @param TransA The type of transpose applied to matrix A.
     * @param Diag Specifies whether or not A is unit triangular.
     * @param A The input allocation contains matrix A, supported elements type: {Element#F64}.
     * @param X The input allocation contains vector x, supported elements type: {Element#F64}.
     * @param incX The increment for the elements of vector x, must be larger than zero.
     */
    void DTRSV(RsBlasUplo Uplo, RsBlasTranspose TransA, RsBlasDiag Diag,
               const sp<Allocation>& A, const sp<Allocation>& X, int incX);

    /**
     * CTRSV solves one of the systems of equations
     * A*x = b   or   A**T*x = b   or   A**H*x = b
     *
     * Details: http://www.netlib.org/lapack/explore-html/d4/dc8/ctrsv_8f.html
     *
     * @param Uplo Specifies whether the matrix is an upper or lower triangular matrix.
     * @param TransA The type of transpose applied to matrix A.
     * @param Diag Specifies whether or not A is unit triangular.
     * @param A The input allocation contains matrix A, supported elements type: {Element#F32_2}.
     * @param X The input allocation contains vector x, supported elements type: {Element#F32_2}.
     * @param incX The increment for the elements of vector x, must be larger than zero.
     */
    void CTRSV(RsBlasUplo Uplo, RsBlasTranspose TransA, RsBlasDiag Diag,
               const sp<Allocation>& A, const sp<Allocation>& X, int incX);

    /**
     * ZTRSV solves one of the systems of equations
     * A*x = b   or   A**T*x = b   or   A**H*x = b
     *
     * Details: http://www.netlib.org/lapack/explore-html/d1/d2f/ztrsv_8f.html
     *
     * @param Uplo Specifies whether the matrix is an upper or lower triangular matrix.
     * @param TransA The type of transpose applied to matrix A.
     * @param Diag Specifies whether or not A is unit triangular.
     * @param A The input allocation contains matrix A, supported elements type: {Element#F64_2}.
     * @param X The input allocation contains vector x, supported elements type: {Element#F64_2}.
     * @param incX The increment for the elements of vector x, must be larger than zero.
     */
    void ZTRSV(RsBlasUplo Uplo, RsBlasTranspose TransA, RsBlasDiag Diag,
               const sp<Allocation>& A, const sp<Allocation>& X, int incX);

    /**
     * STBSV solves one of the systems of equations
     * A*x = b   or   A**T*x = b
     *
     * Details: http://www.netlib.org/lapack/explore-html/d0/d1f/stbsv_8f.html
     *
     * Note: For a N*N matrix, the input Allocation should also be of size N*N (dimY = N, dimX = N),
     *       but only the region N*(K+1) will be referenced. The following subroutine can is an
     *       example showing how to convert a UPPER trianglar matrix 'a' to row-based band matrix 'b'.
     *           for i in range(0, n):
     *              for j in range(i, min(i+k+1, n)):
     *                  b[i, j-i] = a[i, j]
     *
     * @param Uplo Specifies whether the matrix is an upper or lower triangular matrix.
     * @param TransA The type of transpose applied to matrix A.
     * @param Diag Specifies whether or not A is unit triangular.
     * @param K The number of off-diagonals of the matrix A
     * @param A The input allocation contains matrix A, supported elements type: {Element#F32}.
     * @param X The input allocation contains vector x, supported elements type: {Element#F32}.
     * @param incX The increment for the elements of vector x, must be larger than zero.
     */
    void STBSV(RsBlasUplo Uplo, RsBlasTranspose TransA, RsBlasDiag Diag,
               int K, const sp<Allocation>& A, const sp<Allocation>& X, int incX);

    /**
     * DTBSV solves one of the systems of equations
     * A*x = b   or   A**T*x = b
     *
     * Details: http://www.netlib.org/lapack/explore-html/d4/dcf/dtbsv_8f.html
     *
     * Note: For a N*N matrix, the input Allocation should also be of size N*N (dimY = N, dimX = N),
     *       but only the region N*(K+1) will be referenced. The following subroutine can is an
     *       example showing how to convert a UPPER trianglar matrix 'a' to row-based band matrix 'b'.
     *           for i in range(0, n):
     *              for j in range(i, min(i+k+1, n)):
     *                  b[i, j-i] = a[i, j]
     *
     * @param Uplo Specifies whether the matrix is an upper or lower triangular matrix.
     * @param TransA The type of transpose applied to matrix A.
     * @param Diag Specifies whether or not A is unit triangular.
     * @param K The number of off-diagonals of the matrix A
     * @param A The input allocation contains matrix A, supported elements type: {Element#F64}.
     * @param X The input allocation contains vector x, supported elements type: {Element#F64}.
     * @param incX The increment for the elements of vector x, must be larger than zero.
     */
    void DTBSV(RsBlasUplo Uplo, RsBlasTranspose TransA, RsBlasDiag Diag,
               int K, const sp<Allocation>& A, const sp<Allocation>& X, int incX);

    /**
     * CTBSV solves one of the systems of equations
     * A*x = b   or   A**T*x = b   or   A**H*x = b
     *
     * Details: http://www.netlib.org/lapack/explore-html/d9/d5f/ctbsv_8f.html
     *
     * Note: For a N*N matrix, the input Allocation should also be of size N*N (dimY = N, dimX = N),
     *       but only the region N*(K+1) will be referenced. The following subroutine can is an
     *       example showing how to convert a UPPER trianglar matrix 'a' to row-based band matrix 'b'.
     *           for i in range(0, n):
     *              for j in range(i, min(i+k+1, n)):
     *                  b[i, j-i] = a[i, j]
     *
     * @param Uplo Specifies whether the matrix is an upper or lower triangular matrix.
     * @param TransA The type of transpose applied to matrix A.
     * @param Diag Specifies whether or not A is unit triangular.
     * @param K The number of off-diagonals of the matrix A
     * @param A The input allocation contains matrix A, supported elements type: {Element#F32_2}.
     * @param X The input allocation contains vector x, supported elements type: {Element#F32_2}.
     * @param incX The increment for the elements of vector x, must be larger than zero.
     */
    void CTBSV(RsBlasUplo Uplo, RsBlasTranspose TransA, RsBlasDiag Diag,
               int K, const sp<Allocation>& A, const sp<Allocation>& X, int incX);

    /**
     * ZTBSV solves one of the systems of equations
     * A*x = b   or   A**T*x = b   or   A**H*x = b
     *
     * Details: http://www.netlib.org/lapack/explore-html/d4/d5a/ztbsv_8f.html
     *
     * Note: For a N*N matrix, the input Allocation should also be of size N*N (dimY = N, dimX = N),
     *       but only the region N*(K+1) will be referenced. The following subroutine can is an
     *       example showing how to convert a UPPER trianglar matrix 'a' to row-based band matrix 'b'.
     *           for i in range(0, n):
     *              for j in range(i, min(i+k+1, n)):
     *                  b[i, j-i] = a[i, j]
     *
     * @param Uplo Specifies whether the matrix is an upper or lower triangular matrix.
     * @param TransA The type of transpose applied to matrix A.
     * @param Diag Specifies whether or not A is unit triangular.
     * @param K The number of off-diagonals of the matrix A
     * @param A The input allocation contains matrix A, supported elements type: {Element#F64_2}.
     * @param X The input allocation contains vector x, supported elements type: {Element#F64_2}.
     * @param incX The increment for the elements of vector x, must be larger than zero.
     */
    void ZTBSV(RsBlasUplo Uplo, RsBlasTranspose TransA, RsBlasDiag Diag,
               int K, const sp<Allocation>& A, const sp<Allocation>& X, int incX);

    /**
     * STPSV solves one of the systems of equations
     * A*x = b   or   A**T*x = b
     *
     * Details: http://www.netlib.org/lapack/explore-html/d0/d7c/stpsv_8f.html
     *
     * Note: For a N*N matrix, the input Allocation should be a 1D allocation of size dimX = N*(N+1)/2,
     *       The following subroutine can is an example showing how to convert a UPPER trianglar matrix
     *       'a' to packed matrix 'b'.
     *           k = 0
     *           for i in range(0, n):
     *              for j in range(i, n):
     *                  b[k++] = a[i, j]
     *
     * @param Uplo Specifies whether the matrix is an upper or lower triangular matrix.
     * @param TransA The type of transpose applied to matrix A.
     * @param Diag Specifies whether or not A is unit triangular.
     * @param Ap The input allocation contains packed matrix A, supported elements type: {Element#F32}.
     * @param X The input allocation contains vector x, supported elements type: {Element#F32}.
     * @param incX The increment for the elements of vector x, must be larger than zero.
     */
    void STPSV(RsBlasUplo Uplo, RsBlasTranspose TransA, RsBlasDiag Diag,
               const sp<Allocation>& Ap, const sp<Allocation>& X, int incX);

    /**
     * DTPSV solves one of the systems of equations
     * A*x = b   or   A**T*x = b
     *
     * Details: http://www.netlib.org/lapack/explore-html/d9/d84/dtpsv_8f.html
     *
     * Note: For a N*N matrix, the input Allocation should be a 1D allocation of size dimX = N*(N+1)/2,
     *       The following subroutine can is an example showing how to convert a UPPER trianglar matrix
     *       'a' to packed matrix 'b'.
     *           k = 0
     *           for i in range(0, n):
     *              for j in range(i, n):
     *                  b[k++] = a[i, j]
     *
     * @param Uplo Specifies whether the matrix is an upper or lower triangular matrix.
     * @param TransA The type of transpose applied to matrix A.
     * @param Diag Specifies whether or not A is unit triangular.
     * @param Ap The input allocation contains packed matrix A, supported elements type: {Element#F64}.
     * @param X The input allocation contains vector x, supported elements type: {Element#F64}.
     * @param incX The increment for the elements of vector x, must be larger than zero.
     */
    void DTPSV(RsBlasUplo Uplo, RsBlasTranspose TransA, RsBlasDiag Diag,
               const sp<Allocation>& Ap, const sp<Allocation>& X, int incX);

    /**
     * CTPSV solves one of the systems of equations
     * A*x = b   or   A**T*x = b   or   A**H*x = b
     *
     * Details: http://www.netlib.org/lapack/explore-html/d8/d56/ctpsv_8f.html
     *
     * Note: For a N*N matrix, the input Allocation should be a 1D allocation of size dimX = N*(N+1)/2,
     *       The following subroutine can is an example showing how to convert a UPPER trianglar matrix
     *       'a' to packed matrix 'b'.
     *           k = 0
     *           for i in range(0, n):
     *              for j in range(i, n):
     *                  b[k++] = a[i, j]
     *
     * @param Uplo Specifies whether the matrix is an upper or lower triangular matrix.
     * @param TransA The type of transpose applied to matrix A.
     * @param Diag Specifies whether or not A is unit triangular.
     * @param Ap The input allocation contains packed matrix A, supported elements type: {Element#F32_2}.
     * @param X The input allocation contains vector x, supported elements type: {Element#F32_2}.
     * @param incX The increment for the elements of vector x, must be larger than zero.
     */
    void CTPSV(RsBlasUplo Uplo, RsBlasTranspose TransA, RsBlasDiag Diag,
               const sp<Allocation>& Ap, const sp<Allocation>& X, int incX);

    /**
     * ZTPSV solves one of the systems of equations
     * A*x = b   or   A**T*x = b   or   A**H*x = b
     *
     * Details: http://www.netlib.org/lapack/explore-html/da/d57/ztpsv_8f.html
     *
     * Note: For a N*N matrix, the input Allocation should be a 1D allocation of size dimX = N*(N+1)/2,
     *       The following subroutine can is an example showing how to convert a UPPER trianglar matrix
     *       'a' to packed matrix 'b'.
     *           k = 0
     *           for i in range(0, n):
     *              for j in range(i, n):
     *                  b[k++] = a[i, j]
     *
     * @param Uplo Specifies whether the matrix is an upper or lower triangular matrix.
     * @param TransA The type of transpose applied to matrix A.
     * @param Diag Specifies whether or not A is unit triangular.
     * @param Ap The input allocation contains packed matrix A, supported elements type: {Element#F64_2}.
     * @param X The input allocation contains vector x, supported elements type: {Element#F64_2}.
     * @param incX The increment for the elements of vector x, must be larger than zero.
     */
    void ZTPSV(RsBlasUplo Uplo, RsBlasTranspose TransA, RsBlasDiag Diag,
               const sp<Allocation>& Ap, const sp<Allocation>& X, int incX);

    /**
     * SSYMV performs the matrix-vector operation
     * y := alpha*A*x + beta*y
     *
     * Details: http://www.netlib.org/lapack/explore-html/d2/d94/ssymv_8f.html
     *
     * @param Uplo Specifies whether the upper or lower triangular part is to be referenced.
     * @param alpha The scalar alpha.
     * @param A The input allocation contains matrix A, supported elements type: {Element#F32}.
     * @param X The input allocation contains vector x, supported elements type: {Element#F32}.
     * @param incX The increment for the elements of vector x, must be larger than zero.
     * @param beta The scalar beta.
     * @param Y The input allocation contains vector y, supported elements type: {Element#F32}.
     * @param incY The increment for the elements of vector y, must be larger than zero.
     */
    void SSYMV(RsBlasUplo Uplo, float alpha, const sp<Allocation>& A, const sp<Allocation>& X,
               int incX, float beta, const sp<Allocation>& Y, int incY);

    /**
     * SSBMV performs the matrix-vector operation
     * y := alpha*A*x + beta*y
     *
     * Details: http://www.netlib.org/lapack/explore-html/d3/da1/ssbmv_8f.html
     *
     * Note: For a N*N matrix, the input Allocation should also be of size N*N (dimY = N, dimX = N),
     *       but only the region N*(K+1) will be referenced. The following subroutine can is an
     *       example showing how to convert a UPPER trianglar matrix 'a' to row-based band matrix 'b'.
     *           for i in range(0, n):
     *              for j in range(i, min(i+k+1, n)):
     *                  b[i, j-i] = a[i, j]
     *
     * @param Uplo Specifies whether the upper or lower triangular part of the band matrix A is being supplied.
     * @param K The number of off-diagonals of the matrix A
     * @param alpha The scalar alpha.
     * @param A The input allocation contains matrix A, supported elements type: {Element#F32}.
     * @param X The input allocation contains vector x, supported elements type: {Element#F32}.
     * @param incX The increment for the elements of vector x, must be larger than zero.
     * @param beta The scalar beta.
     * @param Y The input allocation contains vector y, supported elements type: {Element#F32}.
     * @param incY The increment for the elements of vector y, must be larger than zero.
     */
    void SSBMV(RsBlasUplo Uplo, int K, float alpha, const sp<Allocation>& A, const sp<Allocation>& X,
               int incX, float beta, const sp<Allocation>& Y, int incY);

    /**
     * SSPMV performs the matrix-vector operation
     * y := alpha*A*x + beta*y
     *
     * Details: http://www.netlib.org/lapack/explore-html/d8/d68/sspmv_8f.html
     *
     * Note: For a N*N matrix, the input Allocation should be a 1D allocation of size dimX = N*(N+1)/2,
     *       The following subroutine can is an example showing how to convert a UPPER trianglar matrix
     *       'a' to packed matrix 'b'.
     *           k = 0
     *           for i in range(0, n):
     *              for j in range(i, n):
     *                  b[k++] = a[i, j]
     *
     * @param Uplo Specifies whether the upper or lower triangular part of the matrix A is supplied in packed form.
     * @param alpha The scalar alpha.
     * @param Ap The input allocation contains matrix A, supported elements type: {Element#F32}.
     * @param X The input allocation contains vector x, supported elements type: {Element#F32}.
     * @param incX The increment for the elements of vector x, must be larger than zero.
     * @param beta The scalar beta.
     * @param Y The input allocation contains vector y, supported elements type: {Element#F32}.
     * @param incY The increment for the elements of vector y, must be larger than zero.
     */
    void SSPMV(RsBlasUplo Uplo, float alpha, const sp<Allocation>& Ap, const sp<Allocation>& X,
               int incX, float beta, const sp<Allocation>& Y, int incY);

    /**
     * SGER performs the rank 1 operation
     * A := alpha*x*y**T + A
     *
     * Details: http://www.netlib.org/lapack/explore-html/db/d5c/sger_8f.html
     *
     * @param alpha The scalar alpha.
     * @param X The input allocation contains vector x, supported elements type: {Element#F32}.
     * @param incX The increment for the elements of vector x, must be larger than zero.
     * @param Y The input allocation contains vector y, supported elements type: {Element#F32}.
     * @param incY The increment for the elements of vector y, must be larger than zero.
     * @param A The input allocation contains matrix A, supported elements type: {Element#F32}.
     */
    void SGER(float alpha, const sp<Allocation>& X, int incX, const sp<Allocation>& Y, int incY, const sp<Allocation>& A);

    /**
     * SSYR performs the rank 1 operation
     * A := alpha*x*x**T + A
     *
     * Details: http://www.netlib.org/lapack/explore-html/d6/dac/ssyr_8f.html
     *
     * @param Uplo Specifies whether the upper or lower triangular part is to be referenced.
     * @param alpha The scalar alpha.
     * @param X The input allocation contains vector x, supported elements type: {Element#F32}.
     * @param incX The increment for the elements of vector x, must be larger than zero.
     * @param A The input allocation contains matrix A, supported elements type: {Element#F32}.
     */
    void SSYR(RsBlasUplo Uplo, float alpha, const sp<Allocation>& X, int incX, const sp<Allocation>& A);

    /**
     * SSPR performs the rank 1 operation
     * A := alpha*x*x**T + A
     *
     * Details: http://www.netlib.org/lapack/explore-html/d2/d9b/sspr_8f.html
     *
     * Note: For a N*N matrix, the input Allocation should be a 1D allocation of size dimX = N*(N+1)/2,
     *       The following subroutine can is an example showing how to convert a UPPER trianglar matrix
     *       'a' to packed matrix 'b'.
     *           k = 0
     *           for i in range(0, n):
     *              for j in range(i, n):
     *                  b[k++] = a[i, j]
     *
     * @param Uplo Specifies whether the upper or lower triangular part is to be supplied in the packed form.
     * @param alpha The scalar alpha.
     * @param X The input allocation contains vector x, supported elements type: {Element#F32}.
     * @param incX The increment for the elements of vector x, must be larger than zero.
     * @param Ap The input allocation contains matrix A, supported elements type: {Element#F32}.
     */
    void SSPR(RsBlasUplo Uplo, float alpha, const sp<Allocation>& X, int incX, const sp<Allocation>& Ap);

    /**
     * SSYR2 performs the symmetric rank 2 operation
     * A := alpha*x*y**T + alpha*y*x**T + A
     *
     * Details: http://www.netlib.org/lapack/explore-html/db/d99/ssyr2_8f.html
     *
     * @param Uplo Specifies whether the upper or lower triangular part is to be referenced.
     * @param alpha The scalar alpha.
     * @param X The input allocation contains vector x, supported elements type: {Element#F32}.
     * @param incX The increment for the elements of vector x, must be larger than zero.
     * @param Y The input allocation contains vector y, supported elements type: {Element#F32}.
     * @param incY The increment for the elements of vector y, must be larger than zero.
     * @param A The input allocation contains matrix A, supported elements type: {Element#F32}.
     */
    void SSYR2(RsBlasUplo Uplo, float alpha, const sp<Allocation>& X, int incX,
               const sp<Allocation>& Y, int incY, const sp<Allocation>& A);

    /**
     * SSPR2 performs the symmetric rank 2 operation
     * A := alpha*x*y**T + alpha*y*x**T + A
     *
     * Details: http://www.netlib.org/lapack/explore-html/db/d3e/sspr2_8f.html
     *
     * Note: For a N*N matrix, the input Allocation should be a 1D allocation of size dimX = N*(N+1)/2,
     *       The following subroutine can is an example showing how to convert a UPPER trianglar matrix
     *       'a' to packed matrix 'b'.
     *           k = 0
     *           for i in range(0, n):
     *              for j in range(i, n):
     *                  b[k++] = a[i, j]
     *
     * @param Uplo Specifies whether the upper or lower triangular part is to be supplied in the packed form.
     * @param alpha The scalar alpha.
     * @param X The input allocation contains vector x, supported elements type: {Element#F32}.
     * @param incX The increment for the elements of vector x, must be larger than zero.
     * @param Y The input allocation contains vector y, supported elements type: {Element#F32}.
     * @param incY The increment for the elements of vector y, must be larger than zero.
     * @param Ap The input allocation contains matrix A, supported elements type: {Element#F32}.
     */
    void SSPR2(RsBlasUplo Uplo, float alpha, const sp<Allocation>& X, int incX,
               const sp<Allocation>& Y, int incY, const sp<Allocation>& Ap);

    /**
     * DSYMV performs the matrix-vector operation
     * y := alpha*A*x + beta*y
     *
     * Details: http://www.netlib.org/lapack/explore-html/d8/dbe/dsymv_8f.html
     *
     * @param Uplo Specifies whether the upper or lower triangular part is to be referenced.
     * @param alpha The scalar alpha.
     * @param A The input allocation contains matrix A, supported elements type: {Element#F64}.
     * @param X The input allocation contains vector x, supported elements type: {Element#F64}.
     * @param incX The increment for the elements of vector x, must be larger than zero.
     * @param beta The scalar beta.
     * @param Y The input allocation contains vector y, supported elements type: {Element#F64}.
     * @param incY The increment for the elements of vector y, must be larger than zero.
     */
    void DSYMV(RsBlasUplo Uplo, double alpha, const sp<Allocation>& A, const sp<Allocation>& X, int incX,
               double beta, const sp<Allocation>& Y, int incY);

    /**
     * DSBMV performs the matrix-vector operation
     * y := alpha*A*x + beta*y
     *
     * Details: http://www.netlib.org/lapack/explore-html/d8/d1e/dsbmv_8f.html
     *
     * Note: For a N*N matrix, the input Allocation should also be of size N*N (dimY = N, dimX = N),
     *       but only the region N*(K+1) will be referenced. The following subroutine can is an
     *       example showing how to convert a UPPER trianglar matrix 'a' to row-based band matrix 'b'.
     *           for i in range(0, n):
     *              for j in range(i, min(i+k+1, n)):
     *                  b[i, j-i] = a[i, j]
     *
     * @param Uplo Specifies whether the upper or lower triangular part of the band matrix A is being supplied.
     * @param K The number of off-diagonals of the matrix A
     * @param alpha The scalar alpha.
     * @param A The input allocation contains matrix A, supported elements type: {Element#F64}.
     * @param X The input allocation contains vector x, supported elements type: {Element#F64}.
     * @param incX The increment for the elements of vector x, must be larger than zero.
     * @param beta The scalar beta.
     * @param Y The input allocation contains vector y, supported elements type: {Element#F64}.
     * @param incY The increment for the elements of vector y, must be larger than zero.
     */
    void DSBMV(RsBlasUplo Uplo, int K, double alpha, const sp<Allocation>& A, const sp<Allocation>& X, int incX,
               double beta, const sp<Allocation>& Y, int incY);

    /**
     * DSPMV performs the matrix-vector operation
     * y := alpha*A*x + beta*y
     *
     * Details: http://www.netlib.org/lapack/explore-html/d4/d85/dspmv_8f.html
     *
     * Note: For a N*N matrix, the input Allocation should be a 1D allocation of size dimX = N*(N+1)/2,
     *       The following subroutine can is an example showing how to convert a UPPER trianglar matrix
     *       'a' to packed matrix 'b'.
     *           k = 0
     *           for i in range(0, n):
     *              for j in range(i, n):
     *                  b[k++] = a[i, j]
     *
     * @param Uplo Specifies whether the upper or lower triangular part of the matrix A is supplied in packed form.
     * @param alpha The scalar alpha.
     * @param Ap The input allocation contains matrix A, supported elements type: {Element#F64}.
     * @param X The input allocation contains vector x, supported elements type: {Element#F64}.
     * @param incX The increment for the elements of vector x, must be larger than zero.
     * @param beta The scalar beta.
     * @param Y The input allocation contains vector y, supported elements type: {Element#F64}.
     * @param incY The increment for the elements of vector y, must be larger than zero.
     */
    void DSPMV(RsBlasUplo Uplo, double alpha, const sp<Allocation>& Ap, const sp<Allocation>& X, int incX,
               double beta, const sp<Allocation>& Y, int incY);

    /**
     * DGER performs the rank 1 operation
     * A := alpha*x*y**T + A
     *
     * Details: http://www.netlib.org/lapack/explore-html/dc/da8/dger_8f.html
     *
     * @param alpha The scalar alpha.
     * @param X The input allocation contains vector x, supported elements type: {Element#F64}.
     * @param incX The increment for the elements of vector x, must be larger than zero.
     * @param Y The input allocation contains vector y, supported elements type: {Element#F64}.
     * @param incY The increment for the elements of vector y, must be larger than zero.
     * @param A The input allocation contains matrix A, supported elements type: {Element#F64}.
     */
    void DGER(double alpha, const sp<Allocation>& X, int incX, const sp<Allocation>& Y, int incY, const sp<Allocation>& A);

    /**
     * DSYR performs the rank 1 operation
     * A := alpha*x*x**T + A
     *
     * Details: http://www.netlib.org/lapack/explore-html/d3/d60/dsyr_8f.html
     *
     * @param Uplo Specifies whether the upper or lower triangular part is to be referenced.
     * @param alpha The scalar alpha.
     * @param X The input allocation contains vector x, supported elements type: {Element#F64}.
     * @param incX The increment for the elements of vector x, must be larger than zero.
     * @param A The input allocation contains matrix A, supported elements type: {Element#F64}.
     */
    void DSYR(RsBlasUplo Uplo, double alpha, const sp<Allocation>& X, int incX, const sp<Allocation>& A);

    /**
     * DSPR performs the rank 1 operation
     * A := alpha*x*x**T + A
     *
     * Details: http://www.netlib.org/lapack/explore-html/dd/dba/dspr_8f.html
     *
     * Note: For a N*N matrix, the input Allocation should be a 1D allocation of size dimX = N*(N+1)/2,
     *       The following subroutine can is an example showing how to convert a UPPER trianglar matrix
     *       'a' to packed matrix 'b'.
     *           k = 0
     *           for i in range(0, n):
     *              for j in range(i, n):
     *                  b[k++] = a[i, j]
     *
     * @param Uplo Specifies whether the upper or lower triangular part is to be supplied in the packed form.
     * @param alpha The scalar alpha.
     * @param X The input allocation contains vector x, supported elements type: {Element#F64}.
     * @param incX The increment for the elements of vector x, must be larger than zero.
     * @param Ap The input allocation contains matrix A, supported elements type: {Element#F64}.
     */
    void DSPR(RsBlasUplo Uplo, double alpha, const sp<Allocation>& X, int incX, const sp<Allocation>& Ap);

    /**
     * DSYR2 performs the symmetric rank 2 operation
     * A := alpha*x*y**T + alpha*y*x**T + A
     *
     * Details: http://www.netlib.org/lapack/explore-html/de/d41/dsyr2_8f.html
     *
     * @param Uplo Specifies whether the upper or lower triangular part is to be referenced.
     * @param alpha The scalar alpha.
     * @param X The input allocation contains vector x, supported elements type: {Element#F64}.
     * @param incX The increment for the elements of vector x, must be larger than zero.
     * @param Y The input allocation contains vector y, supported elements type: {Element#F64}.
     * @param incY The increment for the elements of vector y, must be larger than zero.
     * @param A The input allocation contains matrix A, supported elements type: {Element#F64}.
     */
    void DSYR2(RsBlasUplo Uplo, double alpha, const sp<Allocation>& X, int incX,
               const sp<Allocation>& Y, int incY, const sp<Allocation>& A);

    /**
     * DSPR2 performs the symmetric rank 2 operation
     * A := alpha*x*y**T + alpha*y*x**T + A
     *
     * Details: http://www.netlib.org/lapack/explore-html/dd/d9e/dspr2_8f.html
     *
     * Note: For a N*N matrix, the input Allocation should be a 1D allocation of size dimX = N*(N+1)/2,
     *       The following subroutine can is an example showing how to convert a UPPER trianglar matrix
     *       'a' to packed matrix 'b'.
     *           k = 0
     *           for i in range(0, n):
     *              for j in range(i, n):
     *                  b[k++] = a[i, j]
     *
     * @param Uplo Specifies whether the upper or lower triangular part is to be supplied in the packed form.
     * @param alpha The scalar alpha.
     * @param X The input allocation contains vector x, supported elements type: {Element#F64}.
     * @param incX The increment for the elements of vector x, must be larger than zero.
     * @param Y The input allocation contains vector y, supported elements type: {Element#F64}.
     * @param incY The increment for the elements of vector y, must be larger than zero.
     * @param Ap The input allocation contains matrix A, supported elements type: {Element#F64}.
     */
    void DSPR2(RsBlasUplo Uplo, double alpha, const sp<Allocation>& X, int incX,
               const sp<Allocation>& Y, int incY, const sp<Allocation>& Ap);

    /**
     * CHEMV performs the matrix-vector operation
     * y := alpha*A*x + beta*y
     *
     * Details: http://www.netlib.org/lapack/explore-html/d7/d51/chemv_8f.html
     *
     * @param Uplo Specifies whether the upper or lower triangular part is to be referenced.
     * @param alpha The scalar alpha.
     * @param A The input allocation contains matrix A, supported elements type: {Element#F32_2}.
     * @param X The input allocation contains vector x, supported elements type: {Element#F32_2}.
     * @param incX The increment for the elements of vector x, must be larger than zero.
     * @param beta The scalar beta.
     * @param Y The input allocation contains vector y, supported elements type: {Element#F32_2}.
     * @param incY The increment for the elements of vector y, must be larger than zero.
     */
    void CHEMV(RsBlasUplo Uplo, Float2 alpha, const sp<Allocation>& A, const sp<Allocation>& X,
               int incX, Float2 beta, const sp<Allocation>& Y, int incY);

    /**
     * CHBMV performs the matrix-vector operation
     * y := alpha*A*x + beta*y
     *
     * Details: http://www.netlib.org/lapack/explore-html/db/dc2/chbmv_8f.html
     *
     * Note: For a N*N matrix, the input Allocation should also be of size N*N (dimY = N, dimX = N),
     *       but only the region N*(K+1) will be referenced. The following subroutine can is an
     *       example showing how to convert a UPPER trianglar matrix 'a' to row-based band matrix 'b'.
     *           for i in range(0, n):
     *              for j in range(i, min(i+k+1, n)):
     *                  b[i, j-i] = a[i, j]
     *
     * @param Uplo Specifies whether the upper or lower triangular part of the band matrix A is being supplied.
     * @param K The number of off-diagonals of the matrix A
     * @param alpha The scalar alpha.
     * @param A The input allocation contains matrix A, supported elements type: {Element#F32_2}.
     * @param X The input allocation contains vector x, supported elements type: {Element#F32_2}.
     * @param incX The increment for the elements of vector x, must be larger than zero.
     * @param beta The scalar beta.
     * @param Y The input allocation contains vector y, supported elements type: {Element#F32_2}.
     * @param incY The increment for the elements of vector y, must be larger than zero.
     */
    void CHBMV(RsBlasUplo Uplo, int K, Float2 alpha, const sp<Allocation>& A, const sp<Allocation>& X,
               int incX, Float2 beta, const sp<Allocation>& Y, int incY);

    /**
     * CHPMV performs the matrix-vector operation
     * y := alpha*A*x + beta*y
     *
     * Details: http://www.netlib.org/lapack/explore-html/d2/d06/chpmv_8f.html
     *
     * Note: For a N*N matrix, the input Allocation should be a 1D allocation of size dimX = N*(N+1)/2,
     *       The following subroutine can is an example showing how to convert a UPPER trianglar matrix
     *       'a' to packed matrix 'b'.
     *           k = 0
     *           for i in range(0, n):
     *              for j in range(i, n):
     *                  b[k++] = a[i, j]
     *
     * @param Uplo Specifies whether the upper or lower triangular part of the matrix A is supplied in packed form.
     * @param alpha The scalar alpha.
     * @param Ap The input allocation contains matrix A, supported elements type: {Element#F32_2}.
     * @param X The input allocation contains vector x, supported elements type: {Element#F32_2}.
     * @param incX The increment for the elements of vector x, must be larger than zero.
     * @param beta The scalar beta.
     * @param Y The input allocation contains vector y, supported elements type: {Element#F32_2}.
     * @param incY The increment for the elements of vector y, must be larger than zero.
     */
    void CHPMV(RsBlasUplo Uplo, Float2 alpha, const sp<Allocation>& Ap, const sp<Allocation>& X,
               int incX, Float2 beta, const sp<Allocation>& Y, int incY);

    /**
     * CGERU performs the rank 1 operation
     * A := alpha*x*y**T + A
     *
     * Details: http://www.netlib.org/lapack/explore-html/db/d5f/cgeru_8f.html
     *
     * @param alpha The scalar alpha.
     * @param X The input allocation contains vector x, supported elements type: {Element#F32_2}.
     * @param incX The increment for the elements of vector x, must be larger than zero.
     * @param Y The input allocation contains vector y, supported elements type: {Element#F32_2}.
     * @param incY The increment for the elements of vector y, must be larger than zero.
     * @param A The input allocation contains matrix A, supported elements type: {Element#F32_2}.
     */
    void CGERU(Float2 alpha, const sp<Allocation>& X, int incX,
               const sp<Allocation>& Y, int incY, const sp<Allocation>& A);

    /**
     * CGERC performs the rank 1 operation
     * A := alpha*x*y**H + A
     *
     * Details: http://www.netlib.org/lapack/explore-html/dd/d84/cgerc_8f.html
     *
     * @param alpha The scalar alpha.
     * @param X The input allocation contains vector x, supported elements type: {Element#F32_2}.
     * @param incX The increment for the elements of vector x, must be larger than zero.
     * @param Y The input allocation contains vector y, supported elements type: {Element#F32_2}.
     * @param incY The increment for the elements of vector y, must be larger than zero.
     * @param A The input allocation contains matrix A, supported elements type: {Element#F32_2}.
     */
    void CGERC(Float2 alpha, const sp<Allocation>& X, int incX,
               const sp<Allocation>& Y, int incY, const sp<Allocation>& A);

    /**
     * CHER performs the rank 1 operation
     * A := alpha*x*x**H + A
     *
     * Details: http://www.netlib.org/lapack/explore-html/d3/d6d/cher_8f.html
     *
     * @param Uplo Specifies whether the upper or lower triangular part is to be referenced.
     * @param alpha The scalar alpha.
     * @param X The input allocation contains vector x, supported elements type: {Element#F32_2}.
     * @param incX The increment for the elements of vector x, must be larger than zero.
     * @param A The input allocation contains matrix A, supported elements type: {Element#F32_2}.
     */
    void CHER(RsBlasUplo Uplo, float alpha, const sp<Allocation>& X, int incX, const sp<Allocation>& A);

    /**
     * CHPR performs the rank 1 operation
     * A := alpha*x*x**H + A
     *
     * Details: http://www.netlib.org/lapack/explore-html/db/dcd/chpr_8f.html
     *
     * Note: For a N*N matrix, the input Allocation should be a 1D allocation of size dimX = N*(N+1)/2,
     *       The following subroutine can is an example showing how to convert a UPPER trianglar matrix
     *       'a' to packed matrix 'b'.
     *           k = 0
     *           for i in range(0, n):
     *              for j in range(i, n):
     *                  b[k++] = a[i, j]
     *
     * @param Uplo Specifies whether the upper or lower triangular part is to be supplied in the packed form.
     * @param alpha The scalar alpha.
     * @param X The input allocation contains vector x, supported elements type: {Element#F32_2}.
     * @param incX The increment for the elements of vector x, must be larger than zero.
     * @param Ap The input allocation contains matrix A, supported elements type: {Element#F32_2}.
     */
    void CHPR(RsBlasUplo Uplo, float alpha, const sp<Allocation>& X, int incX, const sp<Allocation>& Ap);

    /**
     * CHER2 performs the symmetric rank 2 operation
     * A := alpha*x*y**H + alpha*y*x**H + A
     *
     * Details: http://www.netlib.org/lapack/explore-html/db/d87/cher2_8f.html
     *
     * @param Uplo Specifies whether the upper or lower triangular part is to be referenced.
     * @param alpha The scalar alpha.
     * @param X The input allocation contains vector x, supported elements type: {Element#F32_2}.
     * @param incX The increment for the elements of vector x, must be larger than zero.
     * @param Y The input allocation contains vector y, supported elements type: {Element#F32_2}.
     * @param incY The increment for the elements of vector y, must be larger than zero.
     * @param A The input allocation contains matrix A, supported elements type: {Element#F32_2}.
     */
    void CHER2(RsBlasUplo Uplo, Float2 alpha, const sp<Allocation>& X, int incX,
               const sp<Allocation>& Y, int incY, const sp<Allocation>& A);

    /**
     * CHPR2 performs the symmetric rank 2 operation
     * A := alpha*x*y**H + alpha*y*x**H + A
     *
     * Details: http://www.netlib.org/lapack/explore-html/d6/d44/chpr2_8f.html
     *
     * Note: For a N*N matrix, the input Allocation should be a 1D allocation of size dimX = N*(N+1)/2,
     *       The following subroutine can is an example showing how to convert a UPPER trianglar matrix
     *       'a' to packed matrix 'b'.
     *           k = 0
     *           for i in range(0, n):
     *              for j in range(i, n):
     *                  b[k++] = a[i, j]
     *
     * @param Uplo Specifies whether the upper or lower triangular part is to be supplied in the packed form.
     * @param alpha The scalar alpha.
     * @param X The input allocation contains vector x, supported elements type: {Element#F32_2}.
     * @param incX The increment for the elements of vector x, must be larger than zero.
     * @param Y The input allocation contains vector y, supported elements type: {Element#F32_2}.
     * @param incY The increment for the elements of vector y, must be larger than zero.
     * @param Ap The input allocation contains matrix A, supported elements type: {Element#F32_2}.
     */
    void CHPR2(RsBlasUplo Uplo, Float2 alpha, const sp<Allocation>& X, int incX,
               const sp<Allocation>& Y, int incY, const sp<Allocation>& Ap);

    /**
     * ZHEMV performs the matrix-vector operation
     * y := alpha*A*x + beta*y
     *
     * Details: http://www.netlib.org/lapack/explore-html/d0/ddd/zhemv_8f.html
     *
     * @param Uplo Specifies whether the upper or lower triangular part is to be referenced.
     * @param alpha The scalar alpha.
     * @param A The input allocation contains matrix A, supported elements type: {Element#F64_2}.
     * @param X The input allocation contains vector x, supported elements type: {Element#F64_2}.
     * @param incX The increment for the elements of vector x, must be larger than zero.
     * @param beta The scalar beta.
     * @param Y The input allocation contains vector y, supported elements type: {Element#F64_2}.
     * @param incY The increment for the elements of vector y, must be larger than zero.
     */
    void ZHEMV(RsBlasUplo Uplo, Double2 alpha, const sp<Allocation>& A, const sp<Allocation>& X,
               int incX, Double2 beta, const sp<Allocation>& Y, int incY);

    /**
     * ZHBMV performs the matrix-vector operation
     * y := alpha*A*x + beta*y
     *
     * Details: http://www.netlib.org/lapack/explore-html/d3/d1a/zhbmv_8f.html
     *
     * Note: For a N*N matrix, the input Allocation should also be of size N*N (dimY = N, dimX = N),
     *       but only the region N*(K+1) will be referenced. The following subroutine can is an
     *       example showing how to convert a UPPER trianglar matrix 'a' to row-based band matrix 'b'.
     *           for i in range(0, n):
     *              for j in range(i, min(i+k+1, n)):
     *                  b[i, j-i] = a[i, j]
     *
     * @param Uplo Specifies whether the upper or lower triangular part of the band matrix A is being supplied.
     * @param K The number of off-diagonals of the matrix A
     * @param alpha The scalar alpha.
     * @param A The input allocation contains matrix A, supported elements type: {Element#F64_2}.
     * @param X The input allocation contains vector x, supported elements type: {Element#F64_2}.
     * @param incX The increment for the elements of vector x, must be larger than zero.
     * @param beta The scalar beta.
     * @param Y The input allocation contains vector y, supported elements type: {Element#F64_2}.
     * @param incY The increment for the elements of vector y, must be larger than zero.
     */
    void ZHBMV(RsBlasUplo Uplo, int K, Double2 alpha, const sp<Allocation>& A, const sp<Allocation>& X,
               int incX, Double2 beta, const sp<Allocation>& Y, int incY);

    /**
     * ZHPMV performs the matrix-vector operation
     * y := alpha*A*x + beta*y
     *
     * Details: http://www.netlib.org/lapack/explore-html/d0/d60/zhpmv_8f.html
     *
     * Note: For a N*N matrix, the input Allocation should be a 1D allocation of size dimX = N*(N+1)/2,
     *       The following subroutine can is an example showing how to convert a UPPER trianglar matrix
     *       'a' to packed matrix 'b'.
     *           k = 0
     *           for i in range(0, n):
     *              for j in range(i, n):
     *                  b[k++] = a[i, j]
     *
     * @param Uplo Specifies whether the upper or lower triangular part of the matrix A is supplied in packed form.
     * @param alpha The scalar alpha.
     * @param Ap The input allocation contains matrix A, supported elements type: {Element#F64_2}.
     * @param X The input allocation contains vector x, supported elements type: {Element#F64_2}.
     * @param incX The increment for the elements of vector x, must be larger than zero.
     * @param beta The scalar beta.
     * @param Y The input allocation contains vector y, supported elements type: {Element#F64_2}.
     * @param incY The increment for the elements of vector y, must be larger than zero.
     */
    void ZHPMV(RsBlasUplo Uplo, Double2 alpha, const sp<Allocation>& Ap, const sp<Allocation>& X,
               int incX, Double2 beta, const sp<Allocation>& Y, int incY);

    /**
     * ZGERU performs the rank 1 operation
     * A := alpha*x*y**T + A
     *
     * Details: http://www.netlib.org/lapack/explore-html/d7/d12/zgeru_8f.html
     *
     * @param alpha The scalar alpha.
     * @param X The input allocation contains vector x, supported elements type: {Element#F64_2}.
     * @param incX The increment for the elements of vector x, must be larger than zero.
     * @param Y The input allocation contains vector y, supported elements type: {Element#F64_2}.
     * @param incY The increment for the elements of vector y, must be larger than zero.
     * @param A The input allocation contains matrix A, supported elements type: {Element#F64_2}.
     */
    void ZGERU(Double2 alpha, const sp<Allocation>& X, int incX,
               const sp<Allocation>& Y, int incY, const sp<Allocation>& A);

    /**
     * ZGERC performs the rank 1 operation
     * A := alpha*x*y**H + A
     *
     * Details: http://www.netlib.org/lapack/explore-html/d3/dad/zgerc_8f.html
     *
     * @param alpha The scalar alpha.
     * @param X The input allocation contains vector x, supported elements type: {Element#F64_2}.
     * @param incX The increment for the elements of vector x, must be larger than zero.
     * @param Y The input allocation contains vector y, supported elements type: {Element#F64_2}.
     * @param incY The increment for the elements of vector y, must be larger than zero.
     * @param A The input allocation contains matrix A, supported elements type: {Element#F64_2}.
     */
    void ZGERC(Double2 alpha, const sp<Allocation>& X, int incX,
               const sp<Allocation>& Y, int incY, const sp<Allocation>& A);

    /**
     * ZHER performs the rank 1 operation
     * A := alpha*x*x**H + A
     *
     * Details: http://www.netlib.org/lapack/explore-html/de/d0e/zher_8f.html
     *
     * @param Uplo Specifies whether the upper or lower triangular part is to be referenced.
     * @param alpha The scalar alpha.
     * @param X The input allocation contains vector x, supported elements type: {Element#F64_2}.
     * @param incX The increment for the elements of vector x, must be larger than zero.
     * @param A The input allocation contains matrix A, supported elements type: {Element#F64_2}.
     */
    void ZHER(RsBlasUplo Uplo, double alpha, const sp<Allocation>& X, int incX, const sp<Allocation>& A);

    /**
     * ZHPR performs the rank 1 operation
     * A := alpha*x*x**H + A
     *
     * Details: http://www.netlib.org/lapack/explore-html/de/de1/zhpr_8f.html
     *
     * Note: For a N*N matrix, the input Allocation should be a 1D allocation of size dimX = N*(N+1)/2,
     *       The following subroutine can is an example showing how to convert a UPPER trianglar matrix
     *       'a' to packed matrix 'b'.
     *           k = 0
     *           for i in range(0, n):
     *              for j in range(i, n):
     *                  b[k++] = a[i, j]
     *
     * @param Uplo Specifies whether the upper or lower triangular part is to be supplied in the packed form.
     * @param alpha The scalar alpha.
     * @param X The input allocation contains vector x, supported elements type: {Element#F64_2}.
     * @param incX The increment for the elements of vector x, must be larger than zero.
     * @param Ap The input allocation contains matrix A, supported elements type: {Element#F64_2}.
     */
    void ZHPR(RsBlasUplo Uplo, double alpha, const sp<Allocation>& X, int incX, const sp<Allocation>& Ap);

    /**
     * ZHER2 performs the symmetric rank 2 operation
     * A := alpha*x*y**H + alpha*y*x**H + A
     *
     * Details: http://www.netlib.org/lapack/explore-html/da/d8a/zher2_8f.html
     *
     * @param Uplo Specifies whether the upper or lower triangular part is to be referenced.
     * @param alpha The scalar alpha.
     * @param X The input allocation contains vector x, supported elements type: {Element#F64_2}.
     * @param incX The increment for the elements of vector x, must be larger than zero.
     * @param Y The input allocation contains vector y, supported elements type: {Element#F64_2}.
     * @param incY The increment for the elements of vector y, must be larger than zero.
     * @param A The input allocation contains matrix A, supported elements type: {Element#F64_2}.
     */
    void ZHER2(RsBlasUplo Uplo, Double2 alpha, const sp<Allocation>& X, int incX,
               const sp<Allocation>& Y, int incY, const sp<Allocation>& A);

    /**
     * ZHPR2 performs the symmetric rank 2 operation
     * A := alpha*x*y**H + alpha*y*x**H + A
     *
     * Details: http://www.netlib.org/lapack/explore-html/d5/d52/zhpr2_8f.html
     *
     * Note: For a N*N matrix, the input Allocation should be a 1D allocation of size dimX = N*(N+1)/2,
     *       The following subroutine can is an example showing how to convert a UPPER trianglar matrix
     *       'a' to packed matrix 'b'.
     *           k = 0
     *           for i in range(0, n):
     *              for j in range(i, n):
     *                  b[k++] = a[i, j]
     *
     * @param Uplo Specifies whether the upper or lower triangular part is to be supplied in the packed form.
     * @param alpha The scalar alpha.
     * @param X The input allocation contains vector x, supported elements type: {Element#F64_2}.
     * @param incX The increment for the elements of vector x, must be larger than zero.
     * @param Y The input allocation contains vector y, supported elements type: {Element#F64_2}.
     * @param incY The increment for the elements of vector y, must be larger than zero.
     * @param Ap The input allocation contains matrix A, supported elements type: {Element#F64_2}.
     */
    void ZHPR2(RsBlasUplo Uplo, Double2 alpha, const sp<Allocation>& X, int incX,
               const sp<Allocation>& Y, int incY, const sp<Allocation>& Ap);

    /**
     * SGEMM performs one of the matrix-matrix operations
     * C := alpha*op(A)*op(B) + beta*C   where op(X) is one of op(X) = X  or  op(X) = X**T
     *
     * Details: http://www.netlib.org/lapack/explore-html/d4/de2/sgemm_8f.html
     *
     * @param TransA The type of transpose applied to matrix A.
     * @param TransB The type of transpose applied to matrix B.
     * @param alpha The scalar alpha.
     * @param A The input allocation contains matrix A, supported elements type: {Element#F32}.
     * @param B The input allocation contains matrix B, supported elements type: {Element#F32}.
     * @param beta The scalar beta.
     * @param C The input allocation contains matrix C, supported elements type: {Element#F32}.
     */
    void SGEMM(RsBlasTranspose TransA, RsBlasTranspose TransB, float alpha, const sp<Allocation>& A,
                      const sp<Allocation>& B, float beta, const sp<Allocation>& C);


    /**
     * DGEMM performs one of the matrix-matrix operations
     * C := alpha*op(A)*op(B) + beta*C   where op(X) is one of op(X) = X  or  op(X) = X**T
     *
     * Details: http://www.netlib.org/lapack/explore-html/d7/d2b/dgemm_8f.html
     *
     * @param TransA The type of transpose applied to matrix A.
     * @param TransB The type of transpose applied to matrix B.
     * @param alpha The scalar alpha.
     * @param A The input allocation contains matrix A, supported elements type: {Element#F64}.
     * @param B The input allocation contains matrix B, supported elements type: {Element#F64}.
     * @param beta The scalar beta.
     * @param C The input allocation contains matrix C, supported elements type: {Element#F64}.
     */
    void DGEMM(RsBlasTranspose TransA, RsBlasTranspose TransB, double alpha, const sp<Allocation>& A,
                      const sp<Allocation>& B, double beta, const sp<Allocation>& C);

    /**
     * CGEMM performs one of the matrix-matrix operations
     * C := alpha*op(A)*op(B) + beta*C   where op(X) is one of op(X) = X  or  op(X) = X**T  or  op(X) = X**H
     *
     * Details: http://www.netlib.org/lapack/explore-html/d6/d5b/cgemm_8f.html
     *
     * @param TransA The type of transpose applied to matrix A.
     * @param TransB The type of transpose applied to matrix B.
     * @param alpha The scalar alpha.
     * @param A The input allocation contains matrix A, supported elements type: {Element#F32_2}.
     * @param B The input allocation contains matrix B, supported elements type: {Element#F32_2}.
     * @param beta The scalar beta.
     * @param C The input allocation contains matrix C, supported elements type: {Element#F32_2}.
     */
    void CGEMM(RsBlasTranspose TransA, RsBlasTranspose TransB, Float2 alpha, const sp<Allocation>& A,
                      const sp<Allocation>& B, Float2 beta, const sp<Allocation>& C);

    /**
     * ZGEMM performs one of the matrix-matrix operations
     * C := alpha*op(A)*op(B) + beta*C   where op(X) is one of op(X) = X  or  op(X) = X**T  or  op(X) = X**H
     *
     * Details: http://www.netlib.org/lapack/explore-html/d7/d76/zgemm_8f.html
     *
     * @param TransA The type of transpose applied to matrix A.
     * @param TransB The type of transpose applied to matrix B.
     * @param alpha The scalar alpha.
     * @param A The input allocation contains matrix A, supported elements type: {Element#F64_2
     * @param B The input allocation contains matrix B, supported elements type: {Element#F64_2
     * @param beta The scalar beta.
     * @param C The input allocation contains matrix C, supported elements type: {Element#F64_2
     */
    void ZGEMM(RsBlasTranspose TransA, RsBlasTranspose TransB, Double2 alpha, const sp<Allocation>& A,
                      const sp<Allocation>& B, Double2 beta, const sp<Allocation>& C);

    /**
     * SSYMM performs one of the matrix-matrix operations
     * C := alpha*A*B + beta*C   or   C := alpha*B*A + beta*C
     *
     * Details: http://www.netlib.org/lapack/explore-html/d7/d42/ssymm_8f.html
     *
     * @param Side Specifies whether the symmetric matrix A appears on the left or right.
     * @param Uplo Specifies whether the upper or lower triangular part is to be referenced.
     * @param alpha The scalar alpha.
     * @param A The input allocation contains matrix A, supported elements type: {Element#F32}.
     * @param B The input allocation contains matrix B, supported elements type: {Element#F32}.
     * @param beta The scalar beta.
     * @param C The input allocation contains matrix C, supported elements type: {Element#F32}.
     */
    void SSYMM(RsBlasSide Side, RsBlasUplo Uplo, float alpha, const sp<Allocation>& A,
                      const sp<Allocation>& B, float beta, const sp<Allocation>& C);

    /**
     * DSYMM performs one of the matrix-matrix operations
     * C := alpha*A*B + beta*C   or   C := alpha*B*A + beta*C
     *
     * Details: http://www.netlib.org/lapack/explore-html/d8/db0/dsymm_8f.html
     *
     * @param Side Specifies whether the symmetric matrix A appears on the left or right.
     * @param Uplo Specifies whether the upper or lower triangular part is to be referenced.
     * @param alpha The scalar alpha.
     * @param A The input allocation contains matrix A, supported elements type: {Element#F64}.
     * @param B The input allocation contains matrix B, supported elements type: {Element#F64}.
     * @param beta The scalar beta.
     * @param C The input allocation contains matrix C, supported elements type: {Element#F64}.
     */
    void DSYMM(RsBlasSide Side, RsBlasUplo Uplo, double alpha, const sp<Allocation>& A,
                      const sp<Allocation>& B, double beta, const sp<Allocation>& C);

    /**
     * CSYMM performs one of the matrix-matrix operations
     * C := alpha*A*B + beta*C   or   C := alpha*B*A + beta*C
     *
     * Details: http://www.netlib.org/lapack/explore-html/db/d59/csymm_8f.html
     *
     * @param Side Specifies whether the symmetric matrix A appears on the left or right.
     * @param Uplo Specifies whether the upper or lower triangular part is to be referenced.
     * @param alpha The scalar alpha.
     * @param A The input allocation contains matrix A, supported elements type: {Element#F32_2}.
     * @param B The input allocation contains matrix B, supported elements type: {Element#F32_2}.
     * @param beta The scalar beta.
     * @param C The input allocation contains matrix C, supported elements type: {Element#F32_2}.
     */
    void CSYMM(RsBlasSide Side, RsBlasUplo Uplo, Float2 alpha, const sp<Allocation>& A,
                      const sp<Allocation>& B, Float2 beta, const sp<Allocation>& C);

    /**
     * ZSYMM performs one of the matrix-matrix operations
     * C := alpha*A*B + beta*C   or   C := alpha*B*A + beta*C
     *
     * Details: http://www.netlib.org/lapack/explore-html/df/d51/zsymm_8f.html
     *
     * @param Side Specifies whether the symmetric matrix A appears on the left or right.
     * @param Uplo Specifies whether the upper or lower triangular part is to be referenced.
     * @param alpha The scalar alpha.
     * @param A The input allocation contains matrix A, supported elements type: {Element#F64_2}.
     * @param B The input allocation contains matrix B, supported elements type: {Element#F64_2}.
     * @param beta The scalar beta.
     * @param C The input allocation contains matrix C, supported elements type: {Element#F64_2}.
     */
    void ZSYMM(RsBlasSide Side, RsBlasUplo Uplo, Double2 alpha, const sp<Allocation>& A,
                      const sp<Allocation>& B, Double2 beta, const sp<Allocation>& C);

    /**
     * SSYRK performs one of the symmetric rank k operations
     * C := alpha*A*A**T + beta*C   or   C := alpha*A**T*A + beta*C
     *
     * Details: http://www.netlib.org/lapack/explore-html/d0/d40/ssyrk_8f.html
     *
     * @param Uplo Specifies whether the upper or lower triangular part of C is to be referenced.
     * @param Trans The type of transpose applied to the operation.
     * @param alpha The scalar alpha.
     * @param A The input allocation contains matrix A, supported elements type: {Element#F32}.
     * @param beta The scalar beta.
     * @param C The input allocation contains matrix C, supported elements type: {Element#F32}.
     */
    void SSYRK(RsBlasUplo Uplo, RsBlasTranspose Trans, float alpha,
               const sp<Allocation>& A, float beta, const sp<Allocation>& C);

    /**
     * DSYRK performs one of the symmetric rank k operations
     * C := alpha*A*A**T + beta*C   or   C := alpha*A**T*A + beta*C
     *
     * Details: http://www.netlib.org/lapack/explore-html/dc/d05/dsyrk_8f.html
     *
     * @param Uplo Specifies whether the upper or lower triangular part of C is to be referenced.
     * @param Trans The type of transpose applied to the operation.
     * @param alpha The scalar alpha.
     * @param A The input allocation contains matrix A, supported elements type: {Element#F64}.
     * @param beta The scalar beta.
     * @param C The input allocation contains matrix C, supported elements type: {Element#F64}.
     */
    void DSYRK(RsBlasUplo Uplo, RsBlasTranspose Trans, double alpha,
               const sp<Allocation>& A, double beta, const sp<Allocation>& C);

    /**
     * CSYRK performs one of the symmetric rank k operations
     * C := alpha*A*A**T + beta*C   or   C := alpha*A**T*A + beta*C
     *
     * Details: http://www.netlib.org/lapack/explore-html/d3/d6a/csyrk_8f.html
     *
     * @param Uplo Specifies whether the upper or lower triangular part of C is to be referenced.
     * @param Trans The type of transpose applied to the operation.
     * @param alpha The scalar alpha.
     * @param A The input allocation contains matrix A, supported elements type: {Element#F32_2}.
     * @param beta The scalar beta.
     * @param C The input allocation contains matrix C, supported elements type: {Element#F32_2}.
     */
    void CSYRK(RsBlasUplo Uplo, RsBlasTranspose Trans, Float2 alpha,
               const sp<Allocation>& A, Float2 beta, const sp<Allocation>& C);

    /**
     * ZSYRK performs one of the symmetric rank k operations
     * C := alpha*A*A**T + beta*C   or   C := alpha*A**T*A + beta*C
     *
     * Details: http://www.netlib.org/lapack/explore-html/de/d54/zsyrk_8f.html
     *
     * @param Uplo Specifies whether the upper or lower triangular part of C is to be referenced.
     * @param Trans The type of transpose applied to the operation.
     * @param alpha The scalar alpha.
     * @param A The input allocation contains matrix A, supported elements type: {Element#F64_2}.
     * @param beta The scalar beta.
     * @param C The input allocation contains matrix C, supported elements type: {Element#F64_2}.
     */
    void ZSYRK(RsBlasUplo Uplo, RsBlasTranspose Trans, Double2 alpha,
               const sp<Allocation>& A, Double2 beta, const sp<Allocation>& C);

    /**
     * SSYR2K performs one of the symmetric rank 2k operations
     * C := alpha*A*B**T + alpha*B*A**T + beta*C   or   C := alpha*A**T*B + alpha*B**T*A + beta*C
     *
     * Details: http://www.netlib.org/lapack/explore-html/df/d3d/ssyr2k_8f.html
     *
     * @param Uplo Specifies whether the upper or lower triangular part of C is to be referenced.
     * @param Trans The type of transpose applied to the operation.
     * @param alpha The scalar alpha.
     * @param A The input allocation contains matrix A, supported elements type: {Element#F32}.
     * @param B The input allocation contains matrix B, supported elements type: {Element#F32}.
     * @param beta The scalar beta.
     * @param C The input allocation contains matrix C, supported elements type: {Element#F32}.
     */
    void SSYR2K(RsBlasUplo Uplo, RsBlasTranspose Trans, float alpha,
                const sp<Allocation>& A, const sp<Allocation>& B, float beta, const sp<Allocation>& C);

    /**
     * DSYR2K performs one of the symmetric rank 2k operations
     * C := alpha*A*B**T + alpha*B*A**T + beta*C   or   C := alpha*A**T*B + alpha*B**T*A + beta*C
     *
     * Details: http://www.netlib.org/lapack/explore-html/d1/dec/dsyr2k_8f.html
     *
     * @param Uplo Specifies whether the upper or lower triangular part of C is to be referenced.
     * @param Trans The type of transpose applied to the operation.
     * @param alpha The scalar alpha.
     * @param A The input allocation contains matrix A, supported elements type: {Element#F64}.
     * @param B The input allocation contains matrix B, supported elements type: {Element#F64}.
     * @param beta The scalar beta.
     * @param C The input allocation contains matrix C, supported elements type: {Element#F64}.
     */
    void DSYR2K(RsBlasUplo Uplo, RsBlasTranspose Trans, double alpha,
                const sp<Allocation>& A, const sp<Allocation>& B, double beta, const sp<Allocation>& C);

    /**
     * CSYR2K performs one of the symmetric rank 2k operations
     * C := alpha*A*B**T + alpha*B*A**T + beta*C   or   C := alpha*A**T*B + alpha*B**T*A + beta*C
     *
     * Details: http://www.netlib.org/lapack/explore-html/de/d7e/csyr2k_8f.html
     *
     * @param Uplo Specifies whether the upper or lower triangular part of C is to be referenced.
     * @param Trans The type of transpose applied to the operation.
     * @param alpha The scalar alpha.
     * @param A The input allocation contains matrix A, supported elements type: {Element#F32_2}.
     * @param B The input allocation contains matrix B, supported elements type: {Element#F32_2}.
     * @param beta The scalar beta.
     * @param C The input allocation contains matrix C, supported elements type: {Element#F32_2}.
     */
    void CSYR2K(RsBlasUplo Uplo, RsBlasTranspose Trans, Float2 alpha,
                const sp<Allocation>& A, const sp<Allocation>& B, Float2 beta, const sp<Allocation>& C);

    /**
     * ZSYR2K performs one of the symmetric rank 2k operations
     * C := alpha*A*B**T + alpha*B*A**T + beta*C   or   C := alpha*A**T*B + alpha*B**T*A + beta*C
     *
     * Details: http://www.netlib.org/lapack/explore-html/df/d20/zsyr2k_8f.html
     *
     * @param Uplo Specifies whether the upper or lower triangular part of C is to be referenced.
     * @param Trans The type of transpose applied to the operation.
     * @param alpha The scalar alpha.
     * @param A The input allocation contains matrix A, supported elements type: {Element#F64_2}.
     * @param B The input allocation contains matrix B, supported elements type: {Element#F64_2}.
     * @param beta The scalar beta.
     * @param C The input allocation contains matrix C, supported elements type: {Element#F64_2}.
     */
    void ZSYR2K(RsBlasUplo Uplo, RsBlasTranspose Trans, Double2 alpha,
                const sp<Allocation>& A, const sp<Allocation>& B, Double2 beta, const sp<Allocation>& C);

    /**
     * STRMM performs one of the matrix-matrix operations
     * B := alpha*op(A)*B   or   B := alpha*B*op(A)
     * op(A) is one of  op(A) = A  or  op(A) = A**T
     *
     * Details: http://www.netlib.org/lapack/explore-html/df/d01/strmm_8f.html
     *
     * @param Side Specifies whether the symmetric matrix A appears on the left or right.
     * @param Uplo Specifies whether matrix A is upper or lower triangular.
     * @param TransA The type of transpose applied to matrix A.
     * @param Diag Specifies whether or not A is unit triangular.
     * @param alpha The scalar alpha.
     * @param A The input allocation contains matrix A, supported elements type: {Element#F32}.
     * @param B The input allocation contains matrix B, supported elements type: {Element#F32}.
     */
    void STRMM(RsBlasSide Side, RsBlasUplo Uplo, RsBlasTranspose TransA,
               RsBlasDiag Diag, float alpha, const sp<Allocation>& A, const sp<Allocation>& B);

    /**
     * DTRMM performs one of the matrix-matrix operations
     * B := alpha*op(A)*B   or   B := alpha*B*op(A)
     * op(A) is one of  op(A) = A  or  op(A) = A**T
     *
     * Details: http://www.netlib.org/lapack/explore-html/dd/d19/dtrmm_8f.html
     *
     * @param Side Specifies whether the symmetric matrix A appears on the left or right.
     * @param Uplo Specifies whether matrix A is upper or lower triangular.
     * @param TransA The type of transpose applied to matrix A.
     * @param Diag Specifies whether or not A is unit triangular.
     * @param alpha The scalar alpha.
     * @param A The input allocation contains matrix A, supported elements type: {Element#F64}.
     * @param B The input allocation contains matrix B, supported elements type: {Element#F64}.
     */
    void DTRMM(RsBlasSide Side, RsBlasUplo Uplo, RsBlasTranspose TransA, RsBlasDiag Diag,
               double alpha, const sp<Allocation>& A, const sp<Allocation>& B);

    /**
     * CTRMM performs one of the matrix-matrix operations
     * B := alpha*op(A)*B   or   B := alpha*B*op(A)
     * op(A) is one of  op(A) = A  or  op(A) = A**T  or  op(A) = A**H
     *
     * Details: http://www.netlib.org/lapack/explore-html/d4/d9b/ctrmm_8f.html
     *
     * @param Side Specifies whether the symmetric matrix A appears on the left or right.
     * @param Uplo Specifies whether matrix A is upper or lower triangular.
     * @param TransA The type of transpose applied to matrix A.
     * @param Diag Specifies whether or not A is unit triangular.
     * @param alpha The scalar alpha.
     * @param A The input allocation contains matrix A, supported elements type: {Element#F32_2}.
     * @param B The input allocation contains matrix B, supported elements type: {Element#F32_2}.
     */
    void CTRMM(RsBlasSide Side, RsBlasUplo Uplo, RsBlasTranspose TransA, RsBlasDiag Diag,
               Float2 alpha, const sp<Allocation>& A, const sp<Allocation>& B);

    /**
     * ZTRMM performs one of the matrix-matrix operations
     * B := alpha*op(A)*B   or   B := alpha*B*op(A)
     * op(A) is one of  op(A) = A  or  op(A) = A**T  or  op(A) = A**H
     *
     * Details: http://www.netlib.org/lapack/explore-html/d8/de1/ztrmm_8f.html
     *
     * @param Side Specifies whether the symmetric matrix A appears on the left or right.
     * @param Uplo Specifies whether matrix A is upper or lower triangular.
     * @param TransA The type of transpose applied to matrix A.
     * @param Diag Specifies whether or not A is unit triangular.
     * @param alpha The scalar alpha.
     * @param A The input allocation contains matrix A, supported elements type: {Element#F64_2}.
     * @param B The input allocation contains matrix B, supported elements type: {Element#F64_2}.
     */
    void ZTRMM(RsBlasSide Side, RsBlasUplo Uplo, RsBlasTranspose TransA, RsBlasDiag Diag,
               Double2 alpha, const sp<Allocation>& A, const sp<Allocation>& B);

    /**
     * STRSM solves one of the matrix equations
     * op(A)*X := alpha*B   or   X*op(A) := alpha*B
     * op(A) is one of  op(A) = A  or  op(A) = A**T
     *
     * Details: http://www.netlib.org/lapack/explore-html/d2/d8b/strsm_8f.html
     *
     * @param Side Specifies whether the symmetric matrix A appears on the left or right.
     * @param Uplo Specifies whether matrix A is upper or lower triangular.
     * @param TransA The type of transpose applied to matrix A.
     * @param Diag Specifies whether or not A is unit triangular.
     * @param alpha The scalar alpha.
     * @param A The input allocation contains matrix A, supported elements type: {Element#F32}.
     * @param B The input allocation contains matrix B, supported elements type: {Element#F32}.
     */
    void STRSM(RsBlasSide Side, RsBlasUplo Uplo, RsBlasTranspose TransA, RsBlasDiag Diag,
               float alpha, const sp<Allocation>& A, const sp<Allocation>& B);

    /**
     * DTRSM solves one of the matrix equations
     * op(A)*X := alpha*B   or   X*op(A) := alpha*B
     * op(A) is one of  op(A) = A  or  op(A) = A**T
     *
     * Details: http://www.netlib.org/lapack/explore-html/de/da7/dtrsm_8f.html
     *
     * @param Side Specifies whether the symmetric matrix A appears on the left or right.
     * @param Uplo Specifies whether matrix A is upper or lower triangular.
     * @param TransA The type of transpose applied to matrix A.
     * @param Diag Specifies whether or not A is unit triangular.
     * @param alpha The scalar alpha.
     * @param A The input allocation contains matrix A, supported elements type: {Element#F64}.
     * @param B The input allocation contains matrix B, supported elements type: {Element#F64}.
     */
    void DTRSM(RsBlasSide Side, RsBlasUplo Uplo, RsBlasTranspose TransA, RsBlasDiag Diag,
               double alpha, const sp<Allocation>& A, const sp<Allocation>& B);

    /**
     * CTRSM solves one of the matrix equations
     * op(A)*X := alpha*B   or   X*op(A) := alpha*B
     * op(A) is one of  op(A) = A  or  op(A) = A**T  or  op(A) = A**H
     *
     * Details: http://www.netlib.org/lapack/explore-html/de/d30/ctrsm_8f.html
     *
     * @param Side Specifies whether the symmetric matrix A appears on the left or right.
     * @param Uplo Specifies whether matrix A is upper or lower triangular.
     * @param TransA The type of transpose applied to matrix A.
     * @param Diag Specifies whether or not A is unit triangular.
     * @param alpha The scalar alpha.
     * @param A The input allocation contains matrix A, supported elements type: {Element#F32_2}.
     * @param B The input allocation contains matrix B, supported elements type: {Element#F32_2}.
     */
    void CTRSM(RsBlasSide Side, RsBlasUplo Uplo, RsBlasTranspose TransA, RsBlasDiag Diag,
               Float2 alpha, const sp<Allocation>& A, const sp<Allocation>& B);

    /**
     * ZTRSM solves one of the matrix equations
     * op(A)*X := alpha*B   or   X*op(A) := alpha*B
     * op(A) is one of  op(A) = A  or  op(A) = A**T  or  op(A) = A**H
     *
     * Details: http://www.netlib.org/lapack/explore-html/d1/d39/ztrsm_8f.html
     *
     * @param Side Specifies whether the symmetric matrix A appears on the left or right.
     * @param Uplo Specifies whether matrix A is upper or lower triangular.
     * @param TransA The type of transpose applied to matrix A.
     * @param Diag Specifies whether or not A is unit triangular.
     * @param alpha The scalar alpha.
     * @param A The input allocation contains matrix A, supported elements type: {Element#F64_2}.
     * @param B The input allocation contains matrix B, supported elements type: {Element#F64_2}.
     */
    void ZTRSM(RsBlasSide Side, RsBlasUplo Uplo, RsBlasTranspose TransA, RsBlasDiag Diag,
               Double2 alpha, const sp<Allocation>& A, const sp<Allocation>& B);

    /**
     * CHEMM performs one of the matrix-matrix operations
     * C := alpha*A*B + beta*C   or   C := alpha*B*A + beta*C
     *
     * Details: http://www.netlib.org/lapack/explore-html/d3/d66/chemm_8f.html
     *
     * @param Side Specifies whether the symmetric matrix A appears on the left or right.
     * @param Uplo Specifies whether the upper or lower triangular part is to be referenced.
     * @param alpha The scalar alpha.
     * @param A The input allocation contains matrix A, supported elements type: {Element#F32_2}.
     * @param B The input allocation contains matrix B, supported elements type: {Element#F32_2}.
     * @param beta The scalar beta.
     * @param C The input allocation contains matrix C, supported elements type: {Element#F32_2}.
     */
    void CHEMM(RsBlasSide Side, RsBlasUplo Uplo, Float2 alpha, const sp<Allocation>& A,
               const sp<Allocation>& B, Float2 beta, const sp<Allocation>& C);

    /**
     * ZHEMM performs one of the matrix-matrix operations
     * C := alpha*A*B + beta*C   or   C := alpha*B*A + beta*C
     *
     * Details: http://www.netlib.org/lapack/explore-html/d6/d3e/zhemm_8f.html
     *
     * @param Side Specifies whether the symmetric matrix A appears on the left or right.
     * @param Uplo Specifies whether the upper or lower triangular part is to be referenced.
     * @param alpha The scalar alpha.
     * @param A The input allocation contains matrix A, supported elements type: {Element#F64_2}.
     * @param B The input allocation contains matrix B, supported elements type: {Element#F64_2}.
     * @param beta The scalar beta.
     * @param C The input allocation contains matrix C, supported elements type: {Element#F64_2}.
     */
    void ZHEMM(RsBlasSide Side, RsBlasUplo Uplo, Double2 alpha, const sp<Allocation>& A,
               const sp<Allocation>& B, Double2 beta, const sp<Allocation>& C);

    /**
     * CHERK performs one of the hermitian rank k operations
     * C := alpha*A*A**H + beta*C   or   C := alpha*A**H*A + beta*C
     *
     * Details: http://www.netlib.org/lapack/explore-html/d8/d52/cherk_8f.html
     *
     * @param Uplo Specifies whether the upper or lower triangular part of C is to be referenced.
     * @param Trans The type of transpose applied to the operation.
     * @param alpha The scalar alpha.
     * @param A The input allocation contains matrix A, supported elements type: {Element#F32_2}.
     * @param beta The scalar beta.
     * @param C The input allocation contains matrix C, supported elements type: {Element#F32_2}.
     */
    void CHERK(RsBlasUplo Uplo, RsBlasTranspose Trans, float alpha, const sp<Allocation>& A,
               float beta, const sp<Allocation>& C);

    /**
     * ZHERK performs one of the hermitian rank k operations
     * C := alpha*A*A**H + beta*C   or   C := alpha*A**H*A + beta*C
     *
     * Details: http://www.netlib.org/lapack/explore-html/d1/db1/zherk_8f.html
     *
     * @param Uplo Specifies whether the upper or lower triangular part of C is to be referenced.
     * @param Trans The type of transpose applied to the operation.
     * @param alpha The scalar alpha.
     * @param A The input allocation contains matrix A, supported elements type: {Element#F64_2}.
     * @param beta The scalar beta.
     * @param C The input allocation contains matrix C, supported elements type: {Element#F64_2}.
     */
    void ZHERK(RsBlasUplo Uplo, RsBlasTranspose Trans, double alpha, const sp<Allocation>& A,
               double beta, const sp<Allocation>& C);

    /**
     * CHER2K performs one of the hermitian rank 2k operations
     * C := alpha*A*B**H + conjg( alpha )*B*A**H + beta*C   or   C := alpha*A**H*B + conjg( alpha )*B**H*A + beta*C
     *
     * Details: http://www.netlib.org/lapack/explore-html/d1/d82/cher2k_8f.html
     *
     * @param Uplo Specifies whether the upper or lower triangular part of C is to be referenced.
     * @param Trans The type of transpose applied to the operation.
     * @param alpha The scalar alpha.
     * @param A The input allocation contains matrix A, supported elements type: {Element#F32_2}.
     * @param B The input allocation contains matrix B, supported elements type: {Element#F32_2}.
     * @param beta The scalar beta.
     * @param C The input allocation contains matrix C, supported elements type: {Element#F32_2}.
     */
    void CHER2K(RsBlasUplo Uplo, RsBlasTranspose Trans, Float2 alpha, const sp<Allocation>& A,
                const sp<Allocation>& B, float beta, const sp<Allocation>& C);

    /**
     * ZHER2K performs one of the hermitian rank 2k operations
     * C := alpha*A*B**H + conjg( alpha )*B*A**H + beta*C   or   C := alpha*A**H*B + conjg( alpha )*B**H*A + beta*C
     *
     * Details: http://www.netlib.org/lapack/explore-html/d7/dfa/zher2k_8f.html
     *
     * @param Uplo Specifies whether the upper or lower triangular part of C is to be referenced.
     * @param Trans The type of transpose applied to the operation.
     * @param alpha The scalar alpha.
     * @param A The input allocation contains matrix A, supported elements type: {Element#F64_2}.
     * @param B The input allocation contains matrix B, supported elements type: {Element#F64_2}.
     * @param beta The scalar beta.
     * @param C The input allocation contains matrix C, supported elements type: {Element#F64_2}.
     */
    void ZHER2K(RsBlasUplo Uplo, RsBlasTranspose Trans, Double2 alpha, const sp<Allocation>& A,
                const sp<Allocation>& B, double beta, const sp<Allocation>& C);

    /**
     * 8-bit GEMM-like operation for neural networks: C = A * Transpose(B)
     * Calculations are done in 1.10.21 fixed-point format for the final output,
     * just before there's a shift down to drop the fractional parts. The output
     * values are gated to 0 to 255 to fit in a byte, but the 10-bit format
     * gives some headroom to avoid wrapping around on small overflows.
     *
     * @param A The input allocation contains matrix A, supported elements type: {Element#U8}.
     * @param a_offset The offset for all values in matrix A, e.g A[i,j] = A[i,j] - a_offset. Value should be from 0 to 255.
     * @param B The input allocation contains matrix B, supported elements type: {Element#U8}.
     * @param b_offset The offset for all values in matrix B, e.g B[i,j] = B[i,j] - b_offset. Value should be from 0 to 255.
     * @param C The input allocation contains matrix C, supported elements type: {Element#U8}.
     * @param c_offset The offset for all values in matrix C.
     * @param c_mult The multiplier for all values in matrix C, e.g C[i,j] = (C[i,j] + c_offset) * c_mult.
     **/
    void BNNM(const sp<Allocation>& A, int a_offset, const sp<Allocation>& B, int b_offset, const sp<Allocation>& C,
              int c_offset, int c_mult);
};

/**
 * Intrinsic kernel for blending two Allocations.
 */
class ScriptIntrinsicBlend : public ScriptIntrinsic {
 private:
    ScriptIntrinsicBlend(sp<RS> rs, sp<const Element> e);
 public:
    /**
     * Supported Element types are U8_4.
     * @param[in] rs RenderScript context
     * @param[in] e Element
     * @return new ScriptIntrinsicBlend
     */
    static sp<ScriptIntrinsicBlend> create(const sp<RS>& rs, const sp<const Element>& e);
    /**
     * sets dst = {0, 0, 0, 0}
     * @param[in] in input Allocation
     * @param[in] out output Allocation
     */
    void forEachClear(const sp<Allocation>& in, const sp<Allocation>& out);
    /**
     * Sets dst = src
     * @param[in] in input Allocation
     * @param[in] out output Allocation
     */
    void forEachSrc(const sp<Allocation>& in, const sp<Allocation>& out);
    /**
     * Sets dst = dst (NOP)
     * @param[in] in input Allocation
     * @param[in] out output Allocation
     */
    void forEachDst(const sp<Allocation>& in, const sp<Allocation>& out);
    /**
     * Sets dst = src + dst * (1.0 - src.a)
     * @param[in] in input Allocation
     * @param[in] out output Allocation
     */
    void forEachSrcOver(const sp<Allocation>& in, const sp<Allocation>& out);
    /**
     * Sets dst = dst + src * (1.0 - dst.a)
     * @param[in] in input Allocation
     * @param[in] out output Allocation
     */
    void forEachDstOver(const sp<Allocation>& in, const sp<Allocation>& out);
    /**
     * Sets dst = src * dst.a
     * @param[in] in input Allocation
     * @param[in] out output Allocation
     */
    void forEachSrcIn(const sp<Allocation>& in, const sp<Allocation>& out);
    /**
     * Sets dst = dst * src.a
     * @param[in] in input Allocation
     * @param[in] out output Allocation
     */
    void forEachDstIn(const sp<Allocation>& in, const sp<Allocation>& out);
    /**
     * Sets dst = src * (1.0 - dst.a)
     * @param[in] in input Allocation
     * @param[in] out output Allocation
     */
    void forEachSrcOut(const sp<Allocation>& in, const sp<Allocation>& out);
    /**
     * Sets dst = dst * (1.0 - src.a)
     * @param[in] in input Allocation
     * @param[in] out output Allocation
     */
    void forEachDstOut(const sp<Allocation>& in, const sp<Allocation>& out);
    /**
     * Sets dst.rgb = src.rgb * dst.a + (1.0 - src.a) * dst.rgb
     * @param[in] in input Allocation
     * @param[in] out output Allocation
     */
    void forEachSrcAtop(const sp<Allocation>& in, const sp<Allocation>& out);
    /**
     * Sets dst.rgb = dst.rgb * src.a + (1.0 - dst.a) * src.rgb
     * @param[in] in input Allocation
     * @param[in] out output Allocation
     */
    void forEachDstAtop(const sp<Allocation>& in, const sp<Allocation>& out);
    /**
     * Sets dst = {src.r ^ dst.r, src.g ^ dst.g, src.b ^ dst.b, src.a ^ dst.a}
     * @param[in] in input Allocation
     * @param[in] out output Allocation
     */
    void forEachXor(const sp<Allocation>& in, const sp<Allocation>& out);
    /**
     * Sets dst = src * dst
     * @param[in] in input Allocation
     * @param[in] out output Allocation
     */
    void forEachMultiply(const sp<Allocation>& in, const sp<Allocation>& out);
    /**
     * Sets dst = min(src + dst, 1.0)
     * @param[in] in input Allocation
     * @param[in] out output Allocation
     */
    void forEachAdd(const sp<Allocation>& in, const sp<Allocation>& out);
    /**
     * Sets dst = max(dst - src, 0.0)
     * @param[in] in input Allocation
     * @param[in] out output Allocation
     */
    void forEachSubtract(const sp<Allocation>& in, const sp<Allocation>& out);
};

/**
 * Intrinsic Gausian blur filter. Applies a Gaussian blur of the specified
 * radius to all elements of an Allocation.
 */
class ScriptIntrinsicBlur : public ScriptIntrinsic {
 private:
    ScriptIntrinsicBlur(sp<RS> rs, sp<const Element> e);
 public:
    /**
     * Supported Element types are U8 and U8_4.
     * @param[in] rs RenderScript context
     * @param[in] e Element
     * @return new ScriptIntrinsicBlur
     */
    static sp<ScriptIntrinsicBlur> create(const sp<RS>& rs, const sp<const Element>& e);
    /**
     * Sets the input of the blur.
     * @param[in] in input Allocation
     */
    void setInput(const sp<Allocation>& in);
    /**
     * Runs the intrinsic.
     * @param[in] output Allocation
     */
    void forEach(const sp<Allocation>& out);
    /**
     * Sets the radius of the blur. The supported range is 0 < radius <= 25.
     * @param[in] radius radius of the blur
     */
    void setRadius(float radius);
};

/**
 * Intrinsic for applying a color matrix to allocations. This has the
 * same effect as loading each element and converting it to a
 * F32_N, multiplying the result by the 4x4 color matrix
 * as performed by rsMatrixMultiply() and writing it to the output
 * after conversion back to U8_N or F32_N.
 */
class ScriptIntrinsicColorMatrix : public ScriptIntrinsic {
 private:
    ScriptIntrinsicColorMatrix(sp<RS> rs, sp<const Element> e);
 public:
    /**
     * Creates a new intrinsic.
     * @param[in] rs RenderScript context
     * @return new ScriptIntrinsicColorMatrix
     */
    static sp<ScriptIntrinsicColorMatrix> create(const sp<RS>& rs);
    /**
     * Applies the color matrix. Supported types are U8 and F32 with
     * vector lengths between 1 and 4.
     * @param[in] in input Allocation
     * @param[out] out output Allocation
     */
    void forEach(const sp<Allocation>& in, const sp<Allocation>& out);
    /**
     * Set the value to be added after the color matrix has been
     * applied. The default value is {0, 0, 0, 0}.
     * @param[in] add float[4] of values
     */
    void setAdd(float* add);

    /**
     * Set the color matrix which will be applied to each cell of the
     * image. The alpha channel will be copied.
     *
     * @param[in] m float[9] of values
     */
    void setColorMatrix3(float* m);
    /**
     * Set the color matrix which will be applied to each cell of the
     * image.
     *
     * @param[in] m float[16] of values
     */
    void setColorMatrix4(float* m);
    /**
     * Set a color matrix to convert from RGB to luminance. The alpha
     * channel will be a copy.
     */
    void setGreyscale();
    /**
     * Set the matrix to convert from RGB to YUV with a direct copy of
     * the 4th channel.
     */
    void setRGBtoYUV();
    /**
     * Set the matrix to convert from YUV to RGB with a direct copy of
     * the 4th channel.
     */
    void setYUVtoRGB();
};

/**
 * Intrinsic for applying a 3x3 convolve to an allocation.
 */
class ScriptIntrinsicConvolve3x3 : public ScriptIntrinsic {
 private:
    ScriptIntrinsicConvolve3x3(sp<RS> rs, sp<const Element> e);
 public:
    /**
     * Supported types U8 and F32 with vector lengths between 1 and
     * 4. The default convolution kernel is the identity.
     * @param[in] rs RenderScript context
     * @param[in] e Element
     * @return new ScriptIntrinsicConvolve3x3
     */
    static sp<ScriptIntrinsicConvolve3x3> create(const sp<RS>& rs, const sp<const Element>& e);
    /**
     * Sets input for intrinsic.
     * @param[in] in input Allocation
     */
    void setInput(const sp<Allocation>& in);
    /**
     * Launches the intrinsic.
     * @param[in] out output Allocation
     */
    void forEach(const sp<Allocation>& out);
    /**
     * Sets convolution kernel.
     * @param[in] v float[9] of values
     */
    void setCoefficients(float* v);
};

/**
 * Intrinsic for applying a 5x5 convolve to an allocation.
 */
class ScriptIntrinsicConvolve5x5 : public ScriptIntrinsic {
 private:
    ScriptIntrinsicConvolve5x5(sp<RS> rs, sp<const Element> e);
 public:
    /**
     * Supported types U8 and F32 with vector lengths between 1 and
     * 4. The default convolution kernel is the identity.
     * @param[in] rs RenderScript context
     * @param[in] e Element
     * @return new ScriptIntrinsicConvolve5x5
     */
    static sp<ScriptIntrinsicConvolve5x5> create(const sp<RS>& rs, const sp<const Element>& e);
    /**
     * Sets input for intrinsic.
     * @param[in] in input Allocation
     */
    void setInput(const sp<Allocation>& in);
    /**
     * Launches the intrinsic.
     * @param[in] out output Allocation
     */
    void forEach(const sp<Allocation>& out);
    /**
     * Sets convolution kernel.
     * @param[in] v float[25] of values
     */
    void setCoefficients(float* v);
};

/**
 * Intrinsic for computing a histogram.
 */
class ScriptIntrinsicHistogram : public ScriptIntrinsic {
 private:
    ScriptIntrinsicHistogram(sp<RS> rs, sp<const Element> e);
    sp<Allocation> mOut;
 public:
    /**
     * Create an intrinsic for calculating the histogram of an uchar
     * or uchar4 image.
     *
     * Supported elements types are U8_4, U8_3, U8_2, and U8.
     *
     * @param[in] rs The RenderScript context
     * @param[in] e Element type for inputs
     *
     * @return ScriptIntrinsicHistogram
     */
    static sp<ScriptIntrinsicHistogram> create(const sp<RS>& rs, const sp<const Element>& e);
    /**
     * Set the output of the histogram.  32 bit integer types are
     * supported.
     *
     * @param[in] aout The output allocation
     */
    void setOutput(const sp<Allocation>& aout);
    /**
     * Set the coefficients used for the dot product calculation. The
     * default is {0.299f, 0.587f, 0.114f, 0.f}.
     *
     * Coefficients must be >= 0 and sum to 1.0 or less.
     *
     * @param[in] r Red coefficient
     * @param[in] g Green coefficient
     * @param[in] b Blue coefficient
     * @param[in] a Alpha coefficient
     */
    void setDotCoefficients(float r, float g, float b, float a);
    /**
     * Process an input buffer and place the histogram into the output
     * allocation. The output allocation may be a narrower vector size
     * than the input. In this case the vector size of the output is
     * used to determine how many of the input channels are used in
     * the computation. This is useful if you have an RGBA input
     * buffer but only want the histogram for RGB.
     *
     * 1D and 2D input allocations are supported.
     *
     * @param[in] ain The input image
     */
    void forEach(const sp<Allocation>& ain);
    /**
     * Process an input buffer and place the histogram into the output
     * allocation. The dot product of the input channel and the
     * coefficients from 'setDotCoefficients' are used to calculate
     * the output values.
     *
     * 1D and 2D input allocations are supported.
     *
     * @param ain The input image
     */
    void forEach_dot(const sp<Allocation>& ain);
};

/**
 * Intrinsic for applying a per-channel lookup table. Each channel of
 * the input has an independant lookup table. The tables are 256
 * entries in size and can cover the full value range of U8_4.
 **/
class ScriptIntrinsicLUT : public ScriptIntrinsic {
 private:
    sp<Allocation> LUT;
    bool mDirty;
    unsigned char mCache[1024];
    void setTable(unsigned int offset, unsigned char base, unsigned int length, unsigned char* lutValues);
    ScriptIntrinsicLUT(sp<RS> rs, sp<const Element> e);

 public:
    /**
     * Supported elements types are U8_4.
     *
     * The defaults tables are identity.
     *
     * @param[in] rs The RenderScript context
     * @param[in] e Element type for intputs and outputs
     *
     * @return ScriptIntrinsicLUT
     */
    static sp<ScriptIntrinsicLUT> create(const sp<RS>& rs, const sp<const Element>& e);
    /**
     * Invoke the kernel and apply the lookup to each cell of ain and
     * copy to aout.
     *
     * @param[in] ain Input allocation
     * @param[in] aout Output allocation
     */
    void forEach(const sp<Allocation>& ain, const sp<Allocation>& aout);
    /**
     * Sets entries in LUT for the red channel.
     * @param[in] base base of region to update
     * @param[in] length length of region to update
     * @param[in] lutValues LUT values to use
     */
    void setRed(unsigned char base, unsigned int length, unsigned char* lutValues);
    /**
     * Sets entries in LUT for the green channel.
     * @param[in] base base of region to update
     * @param[in] length length of region to update
     * @param[in] lutValues LUT values to use
     */
    void setGreen(unsigned char base, unsigned int length, unsigned char* lutValues);
    /**
     * Sets entries in LUT for the blue channel.
     * @param[in] base base of region to update
     * @param[in] length length of region to update
     * @param[in] lutValues LUT values to use
     */
    void setBlue(unsigned char base, unsigned int length, unsigned char* lutValues);
    /**
     * Sets entries in LUT for the alpha channel.
     * @param[in] base base of region to update
     * @param[in] length length of region to update
     * @param[in] lutValues LUT values to use
     */
    void setAlpha(unsigned char base, unsigned int length, unsigned char* lutValues);
    virtual ~ScriptIntrinsicLUT();
};

/**
 * Intrinsic for performing a resize of a 2D allocation.
 */
class ScriptIntrinsicResize : public ScriptIntrinsic {
 private:
    sp<Allocation> mInput;
    ScriptIntrinsicResize(sp<RS> rs, sp<const Element> e);
 public:
    /**
     * Supported Element types are U8_4. Default lookup table is identity.
     * @param[in] rs RenderScript context
     * @param[in] e Element
     * @return new ScriptIntrinsic
     */
    static sp<ScriptIntrinsicResize> create(const sp<RS>& rs);

    /**
     * Resize copy the input allocation to the output specified. The
     * Allocation is rescaled if necessary using bi-cubic
     * interpolation.
     * @param[in] ain input Allocation
     * @param[in] aout output Allocation
     */
    void forEach_bicubic(const sp<Allocation>& aout);

    /**
     * Set the input of the resize.
     * @param[in] lut new lookup table
     */
    void setInput(const sp<Allocation>& ain);
};

/**
 * Intrinsic for converting an Android YUV buffer to RGB.
 *
 * The input allocation should be supplied in a supported YUV format
 * as a YUV element Allocation. The output is RGBA; the alpha channel
 * will be set to 255.
 */
class ScriptIntrinsicYuvToRGB : public ScriptIntrinsic {
 private:
    ScriptIntrinsicYuvToRGB(sp<RS> rs, sp<const Element> e);
 public:
    /**
     * Create an intrinsic for converting YUV to RGB.
     *
     * Supported elements types are U8_4.
     *
     * @param[in] rs The RenderScript context
     * @param[in] e Element type for output
     *
     * @return ScriptIntrinsicYuvToRGB
     */
    static sp<ScriptIntrinsicYuvToRGB> create(const sp<RS>& rs, const sp<const Element>& e);
    /**
     * Set the input YUV allocation.
     *
     * @param[in] ain The input allocation.
     */
    void setInput(const sp<Allocation>& in);

    /**
     * Convert the image to RGB.
     *
     * @param[in] aout Output allocation. Must match creation element
     *                 type.
     */
    void forEach(const sp<Allocation>& out);

};

/**
 * Sampler object that defines how Allocations can be read as textures
 * within a kernel. Samplers are used in conjunction with the rsSample
 * runtime function to return values from normalized coordinates.
 *
 * Any Allocation used with a Sampler must have been created with
 * RS_ALLOCATION_USAGE_GRAPHICS_TEXTURE; using a Sampler on an
 * Allocation that was not created with
 * RS_ALLOCATION_USAGE_GRAPHICS_TEXTURE is undefined.
 **/
 class Sampler : public BaseObj {
 private:
    Sampler(sp<RS> rs, void* id);
    Sampler(sp<RS> rs, void* id, RsSamplerValue min, RsSamplerValue mag,
            RsSamplerValue wrapS, RsSamplerValue wrapT, float anisotropy);
    RsSamplerValue mMin;
    RsSamplerValue mMag;
    RsSamplerValue mWrapS;
    RsSamplerValue mWrapT;
    float mAniso;

 public:
    /**
     * Creates a non-standard Sampler.
     * @param[in] rs RenderScript context
     * @param[in] min minification
     * @param[in] mag magnification
     * @param[in] wrapS S wrapping mode
     * @param[in] wrapT T wrapping mode
     * @param[in] anisotropy anisotropy setting
     */
    static sp<Sampler> create(const sp<RS>& rs, RsSamplerValue min, RsSamplerValue mag, RsSamplerValue wrapS, RsSamplerValue wrapT, float anisotropy);

    /**
     * @return minification setting for the sampler
     */
    RsSamplerValue getMinification();
    /**
     * @return magnification setting for the sampler
     */
    RsSamplerValue getMagnification();
    /**
     * @return S wrapping mode for the sampler
     */
    RsSamplerValue getWrapS();
    /**
     * @return T wrapping mode for the sampler
     */
    RsSamplerValue getWrapT();
    /**
     * @return anisotropy setting for the sampler
     */
    float getAnisotropy();

    /**
     * Retrieve a sampler with min and mag set to nearest and wrap modes set to
     * clamp.
     *
     * @param rs Context to which the sampler will belong.
     *
     * @return Sampler
     */
    static sp<const Sampler> CLAMP_NEAREST(const sp<RS> &rs);
    /**
     * Retrieve a sampler with min and mag set to linear and wrap modes set to
     * clamp.
     *
     * @param rs Context to which the sampler will belong.
     *
     * @return Sampler
     */
    static sp<const Sampler> CLAMP_LINEAR(const sp<RS> &rs);
    /**
     * Retrieve a sampler with mag set to linear, min linear mipmap linear, and
     * wrap modes set to clamp.
     *
     * @param rs Context to which the sampler will belong.
     *
     * @return Sampler
     */
    static sp<const Sampler> CLAMP_LINEAR_MIP_LINEAR(const sp<RS> &rs);
    /**
     * Retrieve a sampler with min and mag set to nearest and wrap modes set to
     * wrap.
     *
     * @param rs Context to which the sampler will belong.
     *
     * @return Sampler
     */
    static sp<const Sampler> WRAP_NEAREST(const sp<RS> &rs);
    /**
     * Retrieve a sampler with min and mag set to linear and wrap modes set to
     * wrap.
     *
     * @param rs Context to which the sampler will belong.
     *
     * @return Sampler
     */
    static sp<const Sampler> WRAP_LINEAR(const sp<RS> &rs);
    /**
     * Retrieve a sampler with mag set to linear, min linear mipmap linear, and
     * wrap modes set to wrap.
     *
     * @param rs Context to which the sampler will belong.
     *
     * @return Sampler
     */
    static sp<const Sampler> WRAP_LINEAR_MIP_LINEAR(const sp<RS> &rs);
    /**
     * Retrieve a sampler with min and mag set to nearest and wrap modes set to
     * mirrored repeat.
     *
     * @param rs Context to which the sampler will belong.
     *
     * @return Sampler
     */
    static sp<const Sampler> MIRRORED_REPEAT_NEAREST(const sp<RS> &rs);
    /**
     * Retrieve a sampler with min and mag set to linear and wrap modes set to
     * mirrored repeat.
     *
     * @param rs Context to which the sampler will belong.
     *
     * @return Sampler
     */
    static sp<const Sampler> MIRRORED_REPEAT_LINEAR(const sp<RS> &rs);
    /**
     * Retrieve a sampler with min and mag set to linear and wrap modes set to
     * mirrored repeat.
     *
     * @param rs Context to which the sampler will belong.
     *
     * @return Sampler
     */
    static sp<const Sampler> MIRRORED_REPEAT_LINEAR_MIP_LINEAR(const sp<RS> &rs);

};

}

}

#endif
