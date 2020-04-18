// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_PUBLIC_PLATFORM_MODULES_PRESENTATION_WEB_PRESENTATION_CLIENT_H_
#define THIRD_PARTY_BLINK_PUBLIC_PLATFORM_MODULES_PRESENTATION_WEB_PRESENTATION_CLIENT_H_

namespace blink {

class WebPresentationReceiver;

// The implementation the embedder has to provide for the Presentation API to
// work.
// It is expected this class will be removed when Presentation API is fully
// onion souped (crbug.com/749327).
class WebPresentationClient {
 public:
  virtual ~WebPresentationClient() = default;

  // Passes the Blink-side delegate to the embedder.
  virtual void SetReceiver(WebPresentationReceiver*) = 0;
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_PUBLIC_PLATFORM_MODULES_PRESENTATION_WEB_PRESENTATION_CLIENT_H_
