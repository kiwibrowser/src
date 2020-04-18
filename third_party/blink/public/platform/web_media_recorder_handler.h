// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_PUBLIC_PLATFORM_WEB_MEDIA_RECORDER_HANDLER_H_
#define THIRD_PARTY_BLINK_PUBLIC_PLATFORM_WEB_MEDIA_RECORDER_HANDLER_H_

#include <memory>

#include "third_party/blink/public/platform/web_common.h"

#include "third_party/blink/public/platform/modules/media_capabilities/web_media_capabilities_info.h"

namespace blink {

class WebMediaRecorderHandlerClient;
struct WebMediaConfiguration;
class WebMediaStream;
class WebString;

// Platform interface of a MediaRecorder.
class BLINK_PLATFORM_EXPORT WebMediaRecorderHandler {
 public:
  virtual ~WebMediaRecorderHandler() = default;
  virtual bool Initialize(WebMediaRecorderHandlerClient* client,
                          const WebMediaStream& stream,
                          const WebString& type,
                          const WebString& codecs,
                          int32_t audio_bits_per_second,
                          int32_t video_bits_per_second) {
    return false;
  }
  virtual bool Start(int timeslice) { return false; }
  virtual void Stop() {}
  virtual void Pause() {}
  virtual void Resume() {}

  // MediaRecorder API isTypeSupported(), which boils down to
  // CanSupportMimeType() [1] "If true is returned from this method, it only
  // indicates that the MediaRecorder implementation is capable of recording
  // Blob objects for the specified MIME type. Recording may still fail if
  // sufficient resources are not available to support the concrete media
  // encoding."
  // [1] https://w3c.github.io/mediacapture-record/MediaRecorder.html#methods
  virtual bool CanSupportMimeType(const WebString& type,
                                  const WebString& codecs) {
    return false;
  }

  // Implements WICG Media Capabilities encodingInfo() call for local encoding.
  // https://wicg.github.io/media-capabilities/#media-capabilities-interface
  virtual void EncodingInfo(
      const WebMediaConfiguration&,
      std::unique_ptr<blink::WebMediaCapabilitiesQueryCallbacks>) {}
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_PUBLIC_PLATFORM_WEB_MEDIA_RECORDER_HANDLER_H_
