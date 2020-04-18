// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/modules/audio_output_devices/audio_output_device_client_impl.h"

#include <memory>
#include "third_party/blink/public/web/web_frame_client.h"
#include "third_party/blink/renderer/core/dom/document.h"
#include "third_party/blink/renderer/core/execution_context/execution_context.h"
#include "third_party/blink/renderer/core/frame/web_local_frame_impl.h"

namespace blink {

AudioOutputDeviceClientImpl::AudioOutputDeviceClientImpl(LocalFrame& frame)
    : AudioOutputDeviceClient(frame) {}

AudioOutputDeviceClientImpl::~AudioOutputDeviceClientImpl() = default;

void AudioOutputDeviceClientImpl::CheckIfAudioSinkExistsAndIsAuthorized(
    ExecutionContext* context,
    const WebString& sink_id,
    std::unique_ptr<WebSetSinkIdCallbacks> callbacks) {
  DCHECK(context);
  DCHECK(context->IsDocument());
  Document* document = ToDocument(context);
  WebLocalFrameImpl* web_frame =
      WebLocalFrameImpl::FromFrame(document->GetFrame());
  web_frame->Client()->CheckIfAudioSinkExistsAndIsAuthorized(
      sink_id, callbacks.release());
}

}  // namespace blink
