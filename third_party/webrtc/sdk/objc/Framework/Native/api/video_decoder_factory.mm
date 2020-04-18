/*
 *  Copyright 2018 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include "sdk/objc/Framework/Native/api/video_decoder_factory.h"

#include "rtc_base/ptr_util.h"
#include "sdk/objc/Framework/Native/src/objc_video_decoder_factory.h"

namespace webrtc {

std::unique_ptr<VideoDecoderFactory> ObjCToNativeVideoDecoderFactory(
    id<RTCVideoDecoderFactory> objc_video_decoder_factory) {
  return rtc::MakeUnique<ObjCVideoDecoderFactory>(objc_video_decoder_factory);
}

}  // namespace webrtc
