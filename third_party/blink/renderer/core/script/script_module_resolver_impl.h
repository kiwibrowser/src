// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_CORE_SCRIPT_SCRIPT_MODULE_RESOLVER_IMPL_H_
#define THIRD_PARTY_BLINK_RENDERER_CORE_SCRIPT_SCRIPT_MODULE_RESOLVER_IMPL_H_

#include "third_party/blink/renderer/bindings/core/v8/script_module.h"
#include "third_party/blink/renderer/core/core_export.h"
#include "third_party/blink/renderer/core/dom/context_lifecycle_observer.h"
#include "third_party/blink/renderer/core/script/script_module_resolver.h"
#include "third_party/blink/renderer/platform/heap/handle.h"
#include "third_party/blink/renderer/platform/wtf/hash_map.h"
#include "third_party/blink/renderer/platform/wtf/text/wtf_string.h"

namespace blink {

class Modulator;
class ModuleScript;
class ScriptModule;

// The ScriptModuleResolverImpl implements ScriptModuleResolver interface
// and implements "HostResolveImportedModule" HTML spec algorithm to bridge
// ModuleMap (via Modulator) and V8 bindings.
class CORE_EXPORT ScriptModuleResolverImpl final
    : public ScriptModuleResolver,
      public ContextLifecycleObserver {
 public:
  static ScriptModuleResolverImpl* Create(Modulator* modulator,
                                          ExecutionContext* execution_context) {
    return new ScriptModuleResolverImpl(modulator, execution_context);
  }

  void Trace(blink::Visitor*) override;
  USING_GARBAGE_COLLECTED_MIXIN(ScriptModuleResolverImpl);

 private:
  explicit ScriptModuleResolverImpl(Modulator* modulator,
                                    ExecutionContext* execution_context)
      : ContextLifecycleObserver(execution_context), modulator_(modulator) {}

  // Implements ScriptModuleResolver:

  void RegisterModuleScript(ModuleScript*) final;
  void UnregisterModuleScript(ModuleScript*) final;
  ModuleScript* GetHostDefined(const ScriptModule&) const final;

  // Implements "Runtime Semantics: HostResolveImportedModule" per HTML spec.
  // https://html.spec.whatwg.org/multipage/webappapis.html#hostresolveimportedmodule(referencingscriptormodule,-specifier))
  ScriptModule Resolve(const String& specifier,
                       const ScriptModule& referrer,
                       ExceptionState&) final;

  // Implements ContextLifecycleObserver:
  void ContextDestroyed(ExecutionContext*) final;

  // Corresponds to the spec concept "referencingModule.[[HostDefined]]".
  // crbug.com/725816 : ScriptModule contains strong ref to v8::Module thus we
  // should not use ScriptModule as the map key. We currently rely on Detach()
  // to clear the refs, but we should implement a key type which keeps a
  // weak-ref to v8::Module.
  HeapHashMap<ScriptModule, Member<ModuleScript>> record_to_module_script_map_;
  Member<Modulator> modulator_;
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_CORE_SCRIPT_SCRIPT_MODULE_RESOLVER_IMPL_H_
