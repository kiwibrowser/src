// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_MODULES_BACKGROUND_FETCH_BACKGROUND_FETCH_FETCH_H_
#define THIRD_PARTY_BLINK_RENDERER_MODULES_BACKGROUND_FETCH_BACKGROUND_FETCH_FETCH_H_

#include "third_party/blink/renderer/platform/bindings/script_wrappable.h"
#include "third_party/blink/renderer/platform/heap/handle.h"

namespace blink {

class Request;

// Base interface for providing developers with access to the fetch
// information associated with a background fetch.
class BackgroundFetchFetch : public ScriptWrappable {
  DEFINE_WRAPPERTYPEINFO();

 public:
  // Web Exposed attribute defined in the IDL file.
  Request* request() const;

  void Trace(blink::Visitor* visitor) override;

 protected:
  explicit BackgroundFetchFetch(Request* request);

 private:
  Member<Request> request_;
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_MODULES_BACKGROUND_FETCH_BACKGROUND_FETCH_FETCH_H_
