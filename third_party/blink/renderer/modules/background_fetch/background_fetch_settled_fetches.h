// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_MODULES_BACKGROUND_FETCH_BACKGROUND_FETCH_SETTLED_FETCHES_H_
#define THIRD_PARTY_BLINK_RENDERER_MODULES_BACKGROUND_FETCH_BACKGROUND_FETCH_SETTLED_FETCHES_H_

#include "third_party/blink/public/platform/modules/background_fetch/web_background_fetch_settled_fetch.h"
#include "third_party/blink/public/platform/web_vector.h"
#include "third_party/blink/renderer/bindings/core/v8/request_or_usv_string.h"
#include "third_party/blink/renderer/bindings/core/v8/script_promise.h"
#include "third_party/blink/renderer/modules/background_fetch/background_fetch_settled_fetch.h"
#include "third_party/blink/renderer/modules/modules_export.h"
#include "third_party/blink/renderer/platform/bindings/script_state.h"
#include "third_party/blink/renderer/platform/bindings/script_wrappable.h"
#include "third_party/blink/renderer/platform/heap/handle.h"

namespace blink {

class MODULES_EXPORT BackgroundFetchSettledFetches final
    : public ScriptWrappable {
  DEFINE_WRAPPERTYPEINFO();

 public:
  static BackgroundFetchSettledFetches* Create(
      ScriptState* script_state,
      const WebVector<WebBackgroundFetchSettledFetch>& fetches) {
    return new BackgroundFetchSettledFetches(script_state, fetches);
  }

  ~BackgroundFetchSettledFetches() override = default;

  // Web Exposed functions defined in the IDL file.
  ScriptPromise match(ScriptState* script_state,
                      const RequestOrUSVString& request);
  ScriptPromise values(ScriptState* script_state);

  void Trace(blink::Visitor* visitor) override;

 private:
  BackgroundFetchSettledFetches(
      ScriptState* script_state,
      const WebVector<WebBackgroundFetchSettledFetch>& fetches);

  HeapVector<Member<BackgroundFetchSettledFetch>> fetches_;
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_MODULES_BACKGROUND_FETCH_BACKGROUND_FETCH_SETTLED_FETCHES_H_
