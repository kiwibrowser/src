// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_MODULES_AUDIO_OUTPUT_DEVICES_AUDIO_OUTPUT_DEVICE_CLIENT_H_
#define THIRD_PARTY_BLINK_RENDERER_MODULES_AUDIO_OUTPUT_DEVICES_AUDIO_OUTPUT_DEVICE_CLIENT_H_

#include <memory>
#include "third_party/blink/public/platform/web_set_sink_id_callbacks.h"
#include "third_party/blink/renderer/core/frame/local_frame.h"
#include "third_party/blink/renderer/modules/modules_export.h"
#include "third_party/blink/renderer/platform/supplementable.h"

namespace blink {

class ExecutionContext;
class WebString;

class MODULES_EXPORT AudioOutputDeviceClient : public Supplement<LocalFrame> {
 public:
  static const char kSupplementName[];

  explicit AudioOutputDeviceClient(LocalFrame&);
  virtual ~AudioOutputDeviceClient() = default;

  // Checks that a given sink exists and has permissions to be used from the
  // origin of the current frame.
  virtual void CheckIfAudioSinkExistsAndIsAuthorized(
      ExecutionContext*,
      const WebString& sink_id,
      std::unique_ptr<WebSetSinkIdCallbacks>) = 0;

  void Trace(blink::Visitor*) override;

  // Supplement requirements.
  static AudioOutputDeviceClient* From(ExecutionContext*);
};

MODULES_EXPORT void ProvideAudioOutputDeviceClientTo(LocalFrame&,
                                                     AudioOutputDeviceClient*);

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_MODULES_AUDIO_OUTPUT_DEVICES_AUDIO_OUTPUT_DEVICE_CLIENT_H_
