// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_CORE_LOADER_MODULESCRIPT_MODULE_SCRIPT_LOADER_REGISTRY_H_
#define THIRD_PARTY_BLINK_RENDERER_CORE_LOADER_MODULESCRIPT_MODULE_SCRIPT_LOADER_REGISTRY_H_

#include "third_party/blink/renderer/core/core_export.h"
#include "third_party/blink/renderer/platform/heap/handle.h"
#include "third_party/blink/renderer/platform/weborigin/kurl.h"
#include "third_party/blink/renderer/platform/wtf/hash_set.h"

namespace blink {

class Modulator;
class ModuleScriptFetchRequest;
class ModuleScriptLoader;
class ModuleScriptLoaderClient;
enum class ModuleGraphLevel;

// ModuleScriptLoaderRegistry keeps active ModuleLoaders alive.
class CORE_EXPORT ModuleScriptLoaderRegistry final
    : public GarbageCollected<ModuleScriptLoaderRegistry> {
 public:
  static ModuleScriptLoaderRegistry* Create() {
    return new ModuleScriptLoaderRegistry;
  }
  void Trace(blink::Visitor*);

  ModuleScriptLoader* Fetch(const ModuleScriptFetchRequest&,
                            ModuleGraphLevel,
                            Modulator*,
                            ModuleScriptLoaderClient*);

 private:
  ModuleScriptLoaderRegistry() = default;

  friend class ModuleScriptLoader;
  void ReleaseFinishedLoader(ModuleScriptLoader*);

  HeapHashSet<Member<ModuleScriptLoader>> active_loaders_;
};

}  // namespace blink

#endif
