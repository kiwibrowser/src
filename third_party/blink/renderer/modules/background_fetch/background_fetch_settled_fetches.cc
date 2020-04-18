// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/modules/background_fetch/background_fetch_settled_fetches.h"

#include "third_party/blink/renderer/core/fetch/request.h"
#include "third_party/blink/renderer/core/fetch/response.h"

namespace blink {

BackgroundFetchSettledFetches::BackgroundFetchSettledFetches(
    ScriptState* script_state,
    const WebVector<WebBackgroundFetchSettledFetch>& fetches) {
  fetches_.ReserveInitialCapacity(fetches.size());
  for (const WebBackgroundFetchSettledFetch& fetch : fetches) {
    auto* settled_fetch = BackgroundFetchSettledFetch::Create(
        Request::Create(script_state, fetch.request),
        Response::Create(script_state, fetch.response));
    fetches_.push_back(settled_fetch);
  }
}

ScriptPromise BackgroundFetchSettledFetches::match(
    ScriptState* script_state,
    const RequestOrUSVString& request) {
  for (const auto& fetch : fetches_) {
    if (request.IsNull())
      continue;

    String request_string = request.IsUSVString()
                                ? request.GetAsUSVString()
                                : request.GetAsRequest()->url().GetString();

    // TODO(crbug.com/824765): Update the resolve condition once behavior of
    // match is defined.
    if (request_string == fetch->request()->url())
      return ScriptPromise::Cast(script_state, ToV8(fetch, script_state));
  }
  return ScriptPromise::Cast(script_state,
                             v8::Null(script_state->GetIsolate()));
}

ScriptPromise BackgroundFetchSettledFetches::values(ScriptState* script_state) {
  return ScriptPromise::Cast(script_state, ToV8(fetches_, script_state));
}

void BackgroundFetchSettledFetches::Trace(blink::Visitor* visitor) {
  visitor->Trace(fetches_);
  ScriptWrappable::Trace(visitor);
}

}  // namespace blink
