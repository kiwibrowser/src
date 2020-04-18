/*
 *  Copyright (c) 2017 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include <jni.h>

#include "api/video/video_frame.h"
#include "api/video/video_sink_interface.h"
#include "sdk/android/generated_video_jni/jni/VideoRenderer_jni.h"
#include "sdk/android/src/jni/jni_helpers.h"
#include "sdk/android/src/jni/videoframe.h"

namespace webrtc {
namespace jni {

// Wrapper dispatching rtc::VideoSinkInterface to a Java VideoRenderer
// instance.
class JavaVideoRendererWrapper : public rtc::VideoSinkInterface<VideoFrame> {
 public:
  JavaVideoRendererWrapper(JNIEnv* jni, const JavaRef<jobject>& j_callbacks)
      : j_callbacks_(jni, j_callbacks) {}

  ~JavaVideoRendererWrapper() override {}

  void OnFrame(const VideoFrame& video_frame) override {
    JNIEnv* env = AttachCurrentThreadIfNeeded();

    ScopedJavaLocalRef<jobject> j_frame;
    if (video_frame.video_frame_buffer()->type() ==
        VideoFrameBuffer::Type::kNative) {
      j_frame = FromWrappedJavaBuffer(env, video_frame);
    } else {
      j_frame = ToJavaI420Frame(env, video_frame);
    }
    // |j_callbacks_| is responsible for releasing |j_frame| with
    // VideoRenderer.renderFrameDone().
    Java_Callbacks_renderFrame(env, j_callbacks_, j_frame);
  }

 private:
  // Make a shallow copy of |frame| to be used with Java. The callee has
  // ownership of the frame, and the frame should be released with
  // VideoRenderer.releaseNativeFrame().
  static jlong javaShallowCopy(const VideoFrame& frame) {
    return jlongFromPointer(new VideoFrame(frame));
  }

  // Return a VideoRenderer.I420Frame referring to the data in |frame|.
  ScopedJavaLocalRef<jobject> FromWrappedJavaBuffer(JNIEnv* env,
                                                    const VideoFrame& frame) {
    return Java_I420Frame_Constructor(
        env, frame.rotation(),
        static_cast<AndroidVideoBuffer*>(frame.video_frame_buffer().get())
            ->video_frame_buffer(),
        javaShallowCopy(frame));
  }

  // Return a VideoRenderer.I420Frame referring to the data in |frame|.
  ScopedJavaLocalRef<jobject> ToJavaI420Frame(JNIEnv* env,
                                              const VideoFrame& frame) {
    rtc::scoped_refptr<I420BufferInterface> i420_buffer =
        frame.video_frame_buffer()->ToI420();
    ScopedJavaLocalRef<jobject> y_buffer =
        NewDirectByteBuffer(env, const_cast<uint8_t*>(i420_buffer->DataY()),
                            i420_buffer->StrideY() * i420_buffer->height());
    size_t chroma_height = i420_buffer->ChromaHeight();
    ScopedJavaLocalRef<jobject> u_buffer =
        NewDirectByteBuffer(env, const_cast<uint8_t*>(i420_buffer->DataU()),
                            i420_buffer->StrideU() * chroma_height);
    ScopedJavaLocalRef<jobject> v_buffer =
        NewDirectByteBuffer(env, const_cast<uint8_t*>(i420_buffer->DataV()),
                            i420_buffer->StrideV() * chroma_height);
    return Java_I420Frame_createI420Frame(
        env, frame.width(), frame.height(), static_cast<int>(frame.rotation()),
        i420_buffer->StrideY(), y_buffer, i420_buffer->StrideU(), u_buffer,
        i420_buffer->StrideV(), v_buffer, javaShallowCopy(frame));
  }

  ScopedJavaGlobalRef<jobject> j_callbacks_;
};

static void JNI_VideoRenderer_FreeWrappedVideoRenderer(
    JNIEnv*,
    const JavaParamRef<jclass>&,
    jlong j_p) {
  delete reinterpret_cast<JavaVideoRendererWrapper*>(j_p);
}

static void JNI_VideoRenderer_ReleaseFrame(JNIEnv* jni,
                                           const JavaParamRef<jclass>&,
                                           jlong j_frame_ptr) {
  delete reinterpret_cast<const VideoFrame*>(j_frame_ptr);
}

static jlong JNI_VideoRenderer_CreateVideoRenderer(
    JNIEnv* jni,
    const JavaParamRef<jclass>&,
    const JavaParamRef<jobject>& j_callbacks) {
  std::unique_ptr<JavaVideoRendererWrapper> renderer(
      new JavaVideoRendererWrapper(jni, j_callbacks));
  return jlongFromPointer(renderer.release());
}

}  // namespace jni
}  // namespace webrtc
