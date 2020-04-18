// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_CORE_LOADER_MODULESCRIPT_MODULE_TREE_LINKER_REGISTRY_H_
#define THIRD_PARTY_BLINK_RENDERER_CORE_LOADER_MODULESCRIPT_MODULE_TREE_LINKER_REGISTRY_H_

#include "third_party/blink/public/platform/web_url_request.h"
#include "third_party/blink/renderer/core/core_export.h"
#include "third_party/blink/renderer/platform/bindings/script_wrappable.h"
#include "third_party/blink/renderer/platform/bindings/trace_wrapper_member.h"
#include "third_party/blink/renderer/platform/heap/handle.h"

namespace blink {

class KURL;
class Modulator;
class ModuleTreeClient;
class ModuleTreeLinker;
class ModuleScript;
class ScriptFetchOptions;

// ModuleTreeLinkerRegistry keeps active ModuleTreeLinkers alive.
class CORE_EXPORT ModuleTreeLinkerRegistry
    : public GarbageCollected<ModuleTreeLinkerRegistry>,
      public TraceWrapperBase {
 public:
  static ModuleTreeLinkerRegistry* Create() {
    return new ModuleTreeLinkerRegistry;
  }
  void Trace(blink::Visitor*);
  void TraceWrappers(ScriptWrappableVisitor*) const override;
  const char* NameInHeapSnapshot() const override {
    return "ModuleTreeLinkerRegistry";
  }

  ModuleTreeLinker* Fetch(const KURL&,
                          const KURL& base_url,
                          WebURLRequest::RequestContext destination,
                          const ScriptFetchOptions&,
                          Modulator*,
                          ModuleTreeClient*);
  ModuleTreeLinker* FetchDescendantsForInlineScript(
      ModuleScript*,
      WebURLRequest::RequestContext destination,
      Modulator*,
      ModuleTreeClient*);

 private:
  ModuleTreeLinkerRegistry() = default;

  friend class ModuleTreeLinker;
  void ReleaseFinishedFetcher(ModuleTreeLinker*);

  HeapHashSet<TraceWrapperMember<ModuleTreeLinker>> active_tree_linkers_;
};

}  // namespace blink

#endif
