// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_MODULES_AUDIO_OUTPUT_DEVICES_AUDIO_OUTPUT_DEVICE_CLIENT_IMPL_H_
#define THIRD_PARTY_BLINK_RENDERER_MODULES_AUDIO_OUTPUT_DEVICES_AUDIO_OUTPUT_DEVICE_CLIENT_IMPL_H_

#include <memory>
#include "third_party/blink/renderer/modules/audio_output_devices/audio_output_device_client.h"
#include "third_party/blink/renderer/modules/modules_export.h"
#include "third_party/blink/renderer/platform/heap/handle.h"

namespace blink {

class MODULES_EXPORT AudioOutputDeviceClientImpl
    : public GarbageCollectedFinalized<AudioOutputDeviceClientImpl>,
      public AudioOutputDeviceClient {
  USING_GARBAGE_COLLECTED_MIXIN(AudioOutputDeviceClientImpl);
  WTF_MAKE_NONCOPYABLE(AudioOutputDeviceClientImpl);

 public:
  explicit AudioOutputDeviceClientImpl(LocalFrame&);

  ~AudioOutputDeviceClientImpl() override;

  // AudioOutputDeviceClient implementation.
  void CheckIfAudioSinkExistsAndIsAuthorized(
      ExecutionContext*,
      const WebString& sink_id,
      std::unique_ptr<WebSetSinkIdCallbacks>) override;

  // GarbageCollectedFinalized implementation.
  void Trace(blink::Visitor* visitor) override {
    AudioOutputDeviceClient::Trace(visitor);
  }
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_MODULES_AUDIO_OUTPUT_DEVICES_AUDIO_OUTPUT_DEVICE_CLIENT_IMPL_H_
