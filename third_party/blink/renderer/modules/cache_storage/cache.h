// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_MODULES_CACHE_STORAGE_CACHE_H_
#define THIRD_PARTY_BLINK_RENDERER_MODULES_CACHE_STORAGE_CACHE_H_

#include <memory>
#include "base/macros.h"

#include "third_party/blink/public/platform/modules/cache_storage/cache_storage.mojom-blink.h"
#include "third_party/blink/renderer/bindings/core/v8/script_promise.h"
#include "third_party/blink/renderer/core/fetch/global_fetch.h"
#include "third_party/blink/renderer/modules/cache_storage/cache_query_options.h"
#include "third_party/blink/renderer/modules/modules_export.h"
#include "third_party/blink/renderer/platform/bindings/script_wrappable.h"
#include "third_party/blink/renderer/platform/wtf/forward.h"
#include "third_party/blink/renderer/platform/wtf/text/wtf_string.h"
#include "third_party/blink/renderer/platform/wtf/vector.h"

namespace blink {

class ExceptionState;
class Response;
class Request;
class ScriptState;

typedef RequestOrUSVString RequestInfo;

class MODULES_EXPORT Cache final : public ScriptWrappable {
  DEFINE_WRAPPERTYPEINFO();

 public:
  static Cache* Create(GlobalFetch::ScopedFetcher*,
                       mojom::blink::CacheStorageCacheAssociatedPtrInfo);

  // From Cache.idl:
  ScriptPromise match(ScriptState*,
                      const RequestInfo&,
                      const CacheQueryOptions&,
                      ExceptionState&);
  ScriptPromise matchAll(ScriptState*, ExceptionState&);
  ScriptPromise matchAll(ScriptState*,
                         const RequestInfo&,
                         const CacheQueryOptions&,
                         ExceptionState&);
  ScriptPromise add(ScriptState*, const RequestInfo&, ExceptionState&);
  ScriptPromise addAll(ScriptState*,
                       const HeapVector<RequestInfo>&,
                       ExceptionState&);
  ScriptPromise Delete(ScriptState*,
                       const RequestInfo&,
                       const CacheQueryOptions&,
                       ExceptionState&);
  ScriptPromise put(ScriptState*,
                    const RequestInfo&,
                    Response*,
                    ExceptionState&);
  ScriptPromise keys(ScriptState*, ExceptionState&);
  ScriptPromise keys(ScriptState*,
                     const RequestInfo&,
                     const CacheQueryOptions&,
                     ExceptionState&);

  static mojom::blink::QueryParamsPtr ToQueryParams(const CacheQueryOptions&);

  void Trace(blink::Visitor*) override;

 private:
  class BarrierCallbackForPut;
  class BlobHandleCallbackForPut;
  class CodeCacheHandleCallbackForPut;
  class FetchResolvedForAdd;
  friend class FetchResolvedForAdd;
  Cache(GlobalFetch::ScopedFetcher*,
        mojom::blink::CacheStorageCacheAssociatedPtrInfo);

  ScriptPromise MatchImpl(ScriptState*,
                          const Request*,
                          const CacheQueryOptions&);
  ScriptPromise MatchAllImpl(ScriptState*,
                             const Request*,
                             const CacheQueryOptions&);
  ScriptPromise AddAllImpl(ScriptState*,
                           const HeapVector<Member<Request>>&,
                           ExceptionState&);
  ScriptPromise DeleteImpl(ScriptState*,
                           const Request*,
                           const CacheQueryOptions&);
  ScriptPromise PutImpl(ScriptState*,
                        const HeapVector<Member<Request>>&,
                        const HeapVector<Member<Response>>&);
  ScriptPromise KeysImpl(ScriptState*,
                         const Request*,
                         const CacheQueryOptions&);

  Member<GlobalFetch::ScopedFetcher> scoped_fetcher_;

  mojom::blink::CacheStorageCacheAssociatedPtr cache_ptr_;

  DISALLOW_COPY_AND_ASSIGN(Cache);
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_MODULES_CACHE_STORAGE_CACHE_H_
