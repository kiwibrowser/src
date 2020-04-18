// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_MODULES_BACKGROUND_FETCH_BACKGROUND_FETCH_SETTLED_FETCH_H_
#define THIRD_PARTY_BLINK_RENDERER_MODULES_BACKGROUND_FETCH_BACKGROUND_FETCH_SETTLED_FETCH_H_

#include "third_party/blink/renderer/modules/background_fetch/background_fetch_fetch.h"
#include "third_party/blink/renderer/platform/heap/handle.h"

namespace blink {

class Request;
class Response;

// Interface for providing developers access to the Request/Response pairs when
// a background fetch has settled, either through the `backgroundfetched` event
// in case of success, or `backgroundfetchfail` in case of failure.
class BackgroundFetchSettledFetch final : public BackgroundFetchFetch {
  DEFINE_WRAPPERTYPEINFO();

 public:
  static BackgroundFetchSettledFetch* Create(Request* request,
                                             Response* response) {
    return new BackgroundFetchSettledFetch(request, response);
  }

  // Web Exposed attribute defined in the IDL file.
  Response* response() const;

  void Trace(blink::Visitor* visitor) override;

 private:
  BackgroundFetchSettledFetch(Request* request, Response* response);

  Member<Response> response_;
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_MODULES_BACKGROUND_FETCH_BACKGROUND_FETCH_SETTLED_FETCH_H_
