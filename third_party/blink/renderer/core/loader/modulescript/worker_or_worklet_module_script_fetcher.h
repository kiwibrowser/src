// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_CORE_LOADER_MODULESCRIPT_WORKER_OR_WORKLET_MODULE_SCRIPT_FETCHER_H_
#define THIRD_PARTY_BLINK_RENDERER_CORE_LOADER_MODULESCRIPT_WORKER_OR_WORKLET_MODULE_SCRIPT_FETCHER_H_

#include "base/optional.h"
#include "third_party/blink/renderer/core/loader/modulescript/module_script_fetcher.h"
#include "third_party/blink/renderer/core/workers/worker_or_worklet_module_fetch_coordinator_proxy.h"
#include "third_party/blink/renderer/core/workers/worklet_module_responses_map.h"

namespace blink {

// WorkerOrWorkletModuleScriptFetcher does not initiate module fetch by itself.
// Instead, this delegates it to WorkletModuleResponsesMap on the
// main thread via WorkerOrWorkletModuleFetchCoordinatorProxy.
// TODO(japhet): Rename to WorkletModuleScriptFetcher
class CORE_EXPORT WorkerOrWorkletModuleScriptFetcher final
    : public ModuleScriptFetcher,
      public WorkletModuleResponsesMap::Client {
  USING_GARBAGE_COLLECTED_MIXIN(WorkerOrWorkletModuleScriptFetcher);

 public:
  explicit WorkerOrWorkletModuleScriptFetcher(
      WorkerOrWorkletModuleFetchCoordinatorProxy*);

  // Implements ModuleScriptFetcher.
  void Fetch(FetchParameters&, ModuleScriptFetcher::Client*) override;

  // Implements WorkletModuleResponsesMap::Client.
  void OnFetched(const ModuleScriptCreationParams&) override;
  void OnFailed() override;

  void Trace(blink::Visitor*) override;

 private:
  void Finalize(const base::Optional<ModuleScriptCreationParams>&,
                const HeapVector<Member<ConsoleMessage>>& error_messages);

  Member<WorkerOrWorkletModuleFetchCoordinatorProxy> coordinator_proxy_;
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_CORE_LOADER_MODULESCRIPT_WORKER_OR_WORKLET_MODULE_SCRIPT_FETCHER_H_
