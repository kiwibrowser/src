/*
 *  Copyright 2015 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#ifndef SDK_ANDROID_SRC_JNI_VIDEOFRAME_H_
#define SDK_ANDROID_SRC_JNI_VIDEOFRAME_H_

#include <jni.h>

#include "api/video/video_frame.h"
#include "api/video/video_frame_buffer.h"
#include "api/video/video_rotation.h"
#include "rtc_base/callback.h"
#include "sdk/android/src/jni/jni_helpers.h"

namespace webrtc {
namespace jni {

// TODO(sakal): Remove once clients have migrated.
using ::webrtc::JavaParamRef;

// Open gl texture matrix, in column-major order. Operations are
// in-place.
class Matrix {
 public:
  Matrix(JNIEnv* jni, const JavaRef<jfloatArray>& a);

  static Matrix fromAndroidGraphicsMatrix(JNIEnv* jni,
                                          const JavaRef<jobject>& j_matrix);

  ScopedJavaLocalRef<jfloatArray> ToJava(JNIEnv* jni) const;

  // Crop arguments are relative to original size.
  void Crop(float cropped_width,
            float cropped_height,
            float crop_x,
            float crop_y);

  void Rotate(VideoRotation rotation);

 private:
  Matrix() {}

  static void Multiply(const float a[16], const float b[16], float result[16]);
  float elem_[16];
};

// Wrapper for texture object.
struct NativeHandleImpl {
  NativeHandleImpl(JNIEnv* jni,
                   jint j_oes_texture_id,
                   const JavaRef<jfloatArray>& j_transform_matrix);

  NativeHandleImpl(int id, const Matrix& matrix);

  const int oes_texture_id;
  Matrix sampling_matrix;
};

class AndroidVideoBuffer : public VideoFrameBuffer {
 public:
  // Creates a native VideoFrameBuffer from a Java VideoFrame.Buffer.
  static rtc::scoped_refptr<AndroidVideoBuffer> Create(
      JNIEnv* jni,
      const JavaRef<jobject>& j_video_frame_buffer);

  // Similar to the Create() above, but adopts and takes ownership of the Java
  // VideoFrame.Buffer. I.e. retain() will not be called, but release() will be
  // called when the returned AndroidVideoBuffer is destroyed.
  static rtc::scoped_refptr<AndroidVideoBuffer> Adopt(
      JNIEnv* jni,
      const JavaRef<jobject>& j_video_frame_buffer);

  ~AndroidVideoBuffer() override;

  const ScopedJavaGlobalRef<jobject>& video_frame_buffer() const;

  // Crops a region defined by |crop_x|, |crop_y|, |crop_width| and
  // |crop_height|. Scales it to size |scale_width| x |scale_height|.
  rtc::scoped_refptr<AndroidVideoBuffer> CropAndScale(JNIEnv* jni,
                                                      int crop_x,
                                                      int crop_y,
                                                      int crop_width,
                                                      int crop_height,
                                                      int scale_width,
                                                      int scale_height);

 protected:
  // Should not be called directly. Adopts the Java VideoFrame.Buffer. Use
  // Create() or Adopt() instead for clarity.
  AndroidVideoBuffer(JNIEnv* jni, const JavaRef<jobject>& j_video_frame_buffer);

 private:
  Type type() const override;
  int width() const override;
  int height() const override;

  rtc::scoped_refptr<I420BufferInterface> ToI420() override;

  const int width_;
  const int height_;
  // Holds a VideoFrame.Buffer.
  const ScopedJavaGlobalRef<jobject> j_video_frame_buffer_;
};

VideoFrame JavaToNativeFrame(JNIEnv* jni,
                             const JavaRef<jobject>& j_video_frame,
                             uint32_t timestamp_rtp);

// NOTE: Returns a new video frame that has to be released by calling
// ReleaseJavaVideoFrame.
ScopedJavaLocalRef<jobject> NativeToJavaVideoFrame(JNIEnv* jni,
                                                   const VideoFrame& frame);
void ReleaseJavaVideoFrame(JNIEnv* jni, const JavaRef<jobject>& j_video_frame);

int64_t GetJavaVideoFrameTimestampNs(JNIEnv* jni,
                                     const JavaRef<jobject>& j_video_frame);

}  // namespace jni
}  // namespace webrtc

#endif  // SDK_ANDROID_SRC_JNI_VIDEOFRAME_H_
