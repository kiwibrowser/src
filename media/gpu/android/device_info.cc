// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "media/gpu/android/device_info.h"

#include "base/android/build_info.h"
#include "media/base/android/media_codec_util.h"

namespace media {

DeviceInfo* DeviceInfo::GetInstance() {
  static DeviceInfo* info = new DeviceInfo();
  return info;
}

int DeviceInfo::SdkVersion() {
  static int result = base::android::BuildInfo::GetInstance()->sdk_int();
  return result;
}

bool DeviceInfo::IsVp8DecoderAvailable() {
  static bool result = MediaCodecUtil::IsVp8DecoderAvailable();
  return result;
}

bool DeviceInfo::IsVp9DecoderAvailable() {
  static bool result = MediaCodecUtil::IsVp9DecoderAvailable();
  return result;
}

bool DeviceInfo::IsDecoderKnownUnaccelerated(VideoCodec codec) {
  return MediaCodecUtil::IsKnownUnaccelerated(codec,
                                              MediaCodecDirection::DECODER);
}

bool DeviceInfo::IsSetOutputSurfaceSupported() {
  static bool result = MediaCodecUtil::IsSetOutputSurfaceSupported();
  return result;
}

bool DeviceInfo::SupportsOverlaySurfaces() {
  static bool result = MediaCodecUtil::IsSurfaceViewOutputSupported();
  return result;
}

bool DeviceInfo::CodecNeedsFlushWorkaround(MediaCodecBridge* codec) {
  return MediaCodecUtil::CodecNeedsFlushWorkaround(codec);
}

}  // namespace media
