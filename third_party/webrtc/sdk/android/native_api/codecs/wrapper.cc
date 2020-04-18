/*
 *  Copyright 2018 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include "sdk/android/native_api/codecs/wrapper.h"

#include "rtc_base/ptr_util.h"
#include "sdk/android/src/jni/videodecoderfactorywrapper.h"
#include "sdk/android/src/jni/videoencoderfactorywrapper.h"

namespace webrtc {

std::unique_ptr<VideoDecoderFactory> JavaToNativeVideoDecoderFactory(
    JNIEnv* jni,
    jobject decoder_factory) {
  return rtc::MakeUnique<jni::VideoDecoderFactoryWrapper>(
      jni, JavaParamRef<jobject>(decoder_factory));
}

std::unique_ptr<VideoEncoderFactory> JavaToNativeVideoEncoderFactory(
    JNIEnv* jni,
    jobject en) {
  return rtc::MakeUnique<jni::VideoEncoderFactoryWrapper>(
      jni, JavaParamRef<jobject>(en));
}

}  // namespace webrtc
