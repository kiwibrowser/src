// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_CORE_FETCH_GLOBAL_FETCH_H_
#define THIRD_PARTY_BLINK_RENDERER_CORE_FETCH_GLOBAL_FETCH_H_

#include "third_party/blink/renderer/bindings/core/v8/script_promise.h"
#include "third_party/blink/renderer/core/core_export.h"
#include "third_party/blink/renderer/core/fetch/request.h"

namespace blink {

class Dictionary;
class LocalDOMWindow;
class ExceptionState;
class ScriptState;
class WorkerGlobalScope;

class CORE_EXPORT GlobalFetch {
  STATIC_ONLY(GlobalFetch);

 public:
  class CORE_EXPORT ScopedFetcher : public GarbageCollectedMixin {
   public:
    virtual ~ScopedFetcher();

    virtual ScriptPromise Fetch(ScriptState*,
                                const RequestInfo&,
                                const Dictionary&,
                                ExceptionState&) = 0;

    static ScopedFetcher* From(LocalDOMWindow&);
    static ScopedFetcher* From(WorkerGlobalScope&);

    void Trace(blink::Visitor*) override;
  };

  static ScriptPromise fetch(ScriptState*,
                             LocalDOMWindow&,
                             const RequestInfo&,
                             const Dictionary&,
                             ExceptionState&);
  static ScriptPromise fetch(ScriptState*,
                             WorkerGlobalScope&,
                             const RequestInfo&,
                             const Dictionary&,
                             ExceptionState&);
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_CORE_FETCH_GLOBAL_FETCH_H_
