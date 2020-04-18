/*
 *  Copyright (c) 2016 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#ifndef API_ANDROID_JNI_ANDROIDVIDEOTRACKSOURCE_H_
#define API_ANDROID_JNI_ANDROIDVIDEOTRACKSOURCE_H_

#include <jni.h>

#include "common_video/include/i420_buffer_pool.h"
#include "common_video/libyuv/include/webrtc_libyuv.h"
#include "media/base/adapted_video_track_source.h"
#include "rtc_base/async_invoker.h"
#include "rtc_base/checks.h"
#include "rtc_base/timestamp_aligner.h"
#include "sdk/android/src/jni/video_frame.h"

namespace webrtc {
namespace jni {

class AndroidVideoTrackSource : public rtc::AdaptedVideoTrackSource {
 public:
  AndroidVideoTrackSource(rtc::Thread* signaling_thread,
                          JNIEnv* jni,
                          bool is_screencast,
                          bool align_timestamps);
  ~AndroidVideoTrackSource() override;

  bool is_screencast() const override;

  // Indicates that the encoder should denoise video before encoding it.
  // If it is not set, the default configuration is used which is different
  // depending on video codec.
  absl::optional<bool> needs_denoising() const override;

  void SetState(SourceState state);

  SourceState state() const override;

  bool remote() const override;

  void OnFrameCaptured(JNIEnv* env,
                       const JavaRef<jobject>& j_caller,
                       jint j_width,
                       jint j_height,
                       jint j_rotation,
                       jlong j_timestamp_ns,
                       const JavaRef<jobject>& j_video_frame_buffer);

  void SetState(JNIEnv* env,
                const JavaRef<jobject>& j_caller,
                jboolean j_is_live);

  void AdaptOutputFormat(JNIEnv* env,
                         const JavaRef<jobject>& j_caller,
                         jint j_landscape_width,
                         jint j_landscape_height,
                         const JavaRef<jobject>& j_max_landscape_pixel_count,
                         jint j_portrait_width,
                         jint j_portrait_height,
                         const JavaRef<jobject>& j_max_portrait_pixel_count,
                         const JavaRef<jobject>& j_max_fps);

 private:
  void InternalSetState(SourceState state);

  rtc::Thread* signaling_thread_;
  rtc::AsyncInvoker invoker_;
  SourceState state_;
  const bool is_screencast_;
  rtc::TimestampAligner timestamp_aligner_;
  const bool align_timestamps_;
};

}  // namespace jni
}  // namespace webrtc

#endif  // API_ANDROID_JNI_ANDROIDVIDEOTRACKSOURCE_H_
