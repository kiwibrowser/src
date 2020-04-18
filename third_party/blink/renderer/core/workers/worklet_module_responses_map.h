// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_CORE_WORKERS_WORKLET_MODULE_RESPONSES_MAP_H_
#define THIRD_PARTY_BLINK_RENDERER_CORE_WORKERS_WORKLET_MODULE_RESPONSES_MAP_H_

#include "third_party/blink/renderer/core/core_export.h"
#include "third_party/blink/renderer/core/loader/modulescript/module_script_creation_params.h"
#include "third_party/blink/renderer/platform/heap/heap.h"
#include "third_party/blink/renderer/platform/heap/heap_allocator.h"
#include "third_party/blink/renderer/platform/loader/fetch/fetch_parameters.h"
#include "third_party/blink/renderer/platform/loader/fetch/resource_fetcher.h"
#include "third_party/blink/renderer/platform/weborigin/kurl.h"
#include "third_party/blink/renderer/platform/weborigin/kurl_hash.h"

namespace blink {

// WorkletModuleResponsesMap implements the module responses map concept and the
// "fetch a worklet script" algorithm:
// https://drafts.css-houdini.org/worklets/#module-responses-map
// https://drafts.css-houdini.org/worklets/#fetch-a-worklet-script
//
// This acts as a cache for creation params (including source code) of module
// scripts, but also performs fetch when needed. The creation params are added
// and retrieved using Fetch(). If a module script for a given URL has already
// been fetched, Fetch() returns the cached creation params. Otherwise, Fetch()
// internally creates DocumentModuleScriptFetcher with thie ResourceFetcher that
// is given to its ctor. Once the module script is fetched, its creation params
// are cached and Fetch() returns it.
//
// TODO(nhiroki): Rename this to WorkletModuleFetchCoordinator, and revise the
// class-level comment.
class CORE_EXPORT WorkletModuleResponsesMap
    : public GarbageCollectedFinalized<WorkletModuleResponsesMap> {
 public:
  // Used for notifying results of Fetch().
  class CORE_EXPORT Client : public GarbageCollectedMixin {
   public:
    virtual ~Client() = default;
    virtual void OnFetched(const ModuleScriptCreationParams&) = 0;
    virtual void OnFailed() = 0;
  };

  explicit WorkletModuleResponsesMap(ResourceFetcher*);

  // Fetches a module script. If the script is already fetched, synchronously
  // calls Client::OnFetched(). Otherwise, it's called on the completion of the
  // fetch. See also the class-level comment.
  void Fetch(FetchParameters&, Client*);

  // Invalidates an inflight module script fetch, and calls OnFailed() for
  // waiting clients.
  void Invalidate(const KURL&);

  // Called when the associated document is destroyed. Aborts all waiting
  // clients and clears the map. Following Fetch() calls are simply ignored.
  void Dispose();

  void Trace(blink::Visitor*);

 private:
  class Entry;

  bool is_available_ = true;

  Member<ResourceFetcher> fetcher_;

  // TODO(nhiroki): Keep the insertion order of top-level modules to replay
  // addModule() calls for a newly created global scope.
  // See https://drafts.css-houdini.org/worklets/#creating-a-workletglobalscope
  HeapHashMap<KURL, Member<Entry>> entries_;
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_CORE_WORKERS_WORKLET_MODULE_RESPONSES_MAP_H_
