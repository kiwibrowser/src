// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_CORE_LOADER_MODULESCRIPT_DOCUMENT_MODULE_SCRIPT_FETCHER_H_
#define THIRD_PARTY_BLINK_RENDERER_CORE_LOADER_MODULESCRIPT_DOCUMENT_MODULE_SCRIPT_FETCHER_H_

#include "base/optional.h"
#include "third_party/blink/renderer/core/loader/modulescript/module_script_creation_params.h"
#include "third_party/blink/renderer/core/loader/modulescript/module_script_fetcher.h"
#include "third_party/blink/renderer/core/loader/resource/script_resource.h"
#include "third_party/blink/renderer/core/script/modulator.h"
#include "third_party/blink/renderer/platform/loader/fetch/fetch_parameters.h"
#include "third_party/blink/renderer/platform/loader/fetch/resource_fetcher.h"
#include "third_party/blink/renderer/platform/weborigin/security_origin.h"

namespace blink {

class ConsoleMessage;

// DocumentModuleScriptFetcher is used to fetch module scripts used in main
// documents and workers (but not worklets).
//
// DocumentModuleScriptFetcher emits FetchParameters to ResourceFetcher
// (via ScriptResource::Fetch). Then, it keeps track of the fetch progress by
// being a ResourceClient. Finally, it returns its client a fetched resource as
// ModuleScriptCreationParams.
class CORE_EXPORT DocumentModuleScriptFetcher : public ModuleScriptFetcher,
                                                public ResourceClient {
  USING_GARBAGE_COLLECTED_MIXIN(DocumentModuleScriptFetcher);

 public:
  explicit DocumentModuleScriptFetcher(ResourceFetcher*);

  void Fetch(FetchParameters&, ModuleScriptFetcher::Client*) final;

  // Implements ResourceClient
  void NotifyFinished(Resource*) final;
  String DebugName() const final { return "DocumentModuleScriptFetcher"; }

  void Trace(blink::Visitor*) override;

 private:
  void Finalize(const base::Optional<ModuleScriptCreationParams>&,
                const HeapVector<Member<ConsoleMessage>>& error_messages);

  // Returns true if loaded as Layered API.
  bool FetchIfLayeredAPI(FetchParameters&);

  Member<ResourceFetcher> fetcher_;
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_CORE_LOADER_MODULESCRIPT_DOCUMENT_MODULE_SCRIPT_FETCHER_H_
