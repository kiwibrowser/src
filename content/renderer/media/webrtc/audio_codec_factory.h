// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_RENDERER_MEDIA_WEBRTC_AUDIO_CODEC_FACTORY_H_
#define CONTENT_RENDERER_MEDIA_WEBRTC_AUDIO_CODEC_FACTORY_H_

#include "third_party/webrtc/api/audio_codecs/audio_decoder_factory.h"
#include "third_party/webrtc/api/audio_codecs/audio_encoder_factory.h"
#include "third_party/webrtc/rtc_base/scoped_ref_ptr.h"

namespace content {

rtc::scoped_refptr<webrtc::AudioEncoderFactory>
CreateWebrtcAudioEncoderFactory();

rtc::scoped_refptr<webrtc::AudioDecoderFactory>
CreateWebrtcAudioDecoderFactory();

}  // namespace content

#endif  // CONTENT_RENDERER_MEDIA_WEBRTC_AUDIO_CODEC_FACTORY_H_
