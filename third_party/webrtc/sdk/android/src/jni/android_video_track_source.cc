/*
 *  Copyright (c) 2016 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include "sdk/android/src/jni/android_video_track_source.h"

#include "sdk/android/generated_video_jni/jni/NativeAndroidVideoTrackSource_jni.h"

#include <utility>

#include "rtc_base/logging.h"

namespace webrtc {
namespace jni {

namespace {
// MediaCodec wants resolution to be divisible by 2.
const int kRequiredResolutionAlignment = 2;

VideoRotation jintToVideoRotation(jint rotation) {
  RTC_DCHECK(rotation == 0 || rotation == 90 || rotation == 180 ||
             rotation == 270);
  return static_cast<VideoRotation>(rotation);
}

absl::optional<std::pair<int, int>> OptionalAspectRatio(jint j_width,
                                                        jint j_height) {
  if (j_width > 0 && j_height > 0)
    return std::pair<int, int>(j_width, j_height);
  return absl::nullopt;
}

}  // namespace

AndroidVideoTrackSource::AndroidVideoTrackSource(rtc::Thread* signaling_thread,
                                                 JNIEnv* jni,
                                                 bool is_screencast,
                                                 bool align_timestamps)
    : AdaptedVideoTrackSource(kRequiredResolutionAlignment),
      signaling_thread_(signaling_thread),
      is_screencast_(is_screencast),
      align_timestamps_(align_timestamps) {
  RTC_LOG(LS_INFO) << "AndroidVideoTrackSource ctor";
}
AndroidVideoTrackSource::~AndroidVideoTrackSource() = default;

bool AndroidVideoTrackSource::is_screencast() const {
  return is_screencast_;
}

absl::optional<bool> AndroidVideoTrackSource::needs_denoising() const {
  return false;
}

void AndroidVideoTrackSource::SetState(JNIEnv* env,
                                       const JavaRef<jobject>& j_caller,
                                       jboolean j_is_live) {
  InternalSetState(j_is_live ? kLive : kEnded);
}

void AndroidVideoTrackSource::InternalSetState(SourceState state) {
  if (rtc::Thread::Current() != signaling_thread_) {
    invoker_.AsyncInvoke<void>(
        RTC_FROM_HERE, signaling_thread_,
        rtc::Bind(&AndroidVideoTrackSource::InternalSetState, this, state));
    return;
  }

  if (state_ != state) {
    state_ = state;
    FireOnChanged();
  }
}

AndroidVideoTrackSource::SourceState AndroidVideoTrackSource::state() const {
  return state_;
}

bool AndroidVideoTrackSource::remote() const {
  return false;
}

void AndroidVideoTrackSource::OnFrameCaptured(
    JNIEnv* env,
    const JavaRef<jobject>& j_caller,
    jint j_width,
    jint j_height,
    jint j_rotation,
    jlong j_timestamp_ns,
    const JavaRef<jobject>& j_video_frame_buffer) {
  const VideoRotation rotation = jintToVideoRotation(j_rotation);

  int64_t camera_time_us = j_timestamp_ns / rtc::kNumNanosecsPerMicrosec;
  int64_t translated_camera_time_us =
      align_timestamps_ ? timestamp_aligner_.TranslateTimestamp(
                              camera_time_us, rtc::TimeMicros())
                        : camera_time_us;

  int adapted_width;
  int adapted_height;
  int crop_width;
  int crop_height;
  int crop_x;
  int crop_y;

  if (rotation % 180 == 0) {
    if (!AdaptFrame(j_width, j_height, camera_time_us, &adapted_width,
                    &adapted_height, &crop_width, &crop_height, &crop_x,
                    &crop_y)) {
      return;
    }
  } else {
    // Swap all width/height and x/y.
    if (!AdaptFrame(j_height, j_width, camera_time_us, &adapted_height,
                    &adapted_width, &crop_height, &crop_width, &crop_y,
                    &crop_x)) {
      return;
    }
  }

  rtc::scoped_refptr<VideoFrameBuffer> buffer =
      AndroidVideoBuffer::Create(env, j_video_frame_buffer)
          ->CropAndScale(env, crop_x, crop_y, crop_width, crop_height,
                         adapted_width, adapted_height);

  // AdaptedVideoTrackSource handles applying rotation for I420 frames.
  if (apply_rotation() && rotation != kVideoRotation_0) {
    buffer = buffer->ToI420();
  }

  OnFrame(VideoFrame::Builder()
              .set_video_frame_buffer(buffer)
              .set_rotation(rotation)
              .set_timestamp_us(translated_camera_time_us)
              .build());
}

void AndroidVideoTrackSource::AdaptOutputFormat(
    JNIEnv* env,
    const JavaRef<jobject>& j_caller,
    jint j_landscape_width,
    jint j_landscape_height,
    const JavaRef<jobject>& j_max_landscape_pixel_count,
    jint j_portrait_width,
    jint j_portrait_height,
    const JavaRef<jobject>& j_max_portrait_pixel_count,
    const JavaRef<jobject>& j_max_fps) {
  video_adapter()->OnOutputFormatRequest(
      OptionalAspectRatio(j_landscape_width, j_landscape_height),
      JavaToNativeOptionalInt(env, j_max_landscape_pixel_count),
      OptionalAspectRatio(j_portrait_width, j_portrait_height),
      JavaToNativeOptionalInt(env, j_max_portrait_pixel_count),
      JavaToNativeOptionalInt(env, j_max_fps));
}

}  // namespace jni
}  // namespace webrtc
