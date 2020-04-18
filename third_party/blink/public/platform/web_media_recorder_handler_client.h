// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_PUBLIC_PLATFORM_WEB_MEDIA_RECORDER_HANDLER_CLIENT_H_
#define THIRD_PARTY_BLINK_PUBLIC_PLATFORM_WEB_MEDIA_RECORDER_HANDLER_CLIENT_H_

#include "third_party/blink/public/platform/web_common.h"

namespace blink {

class WebString;

// Interface used by a MediaRecorder to get errors and recorded data delivered.
class WebMediaRecorderHandlerClient {
 public:
  virtual void WriteData(const char* data,
                         size_t length,
                         bool last_inslice,
                         double timecode) = 0;

  virtual void OnError(const WebString& message) = 0;
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_PUBLIC_PLATFORM_WEB_MEDIA_RECORDER_HANDLER_CLIENT_H_
