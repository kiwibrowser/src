// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_CORE_SCRIPT_SCRIPT_MODULE_RESOLVER_H_
#define THIRD_PARTY_BLINK_RENDERER_CORE_SCRIPT_SCRIPT_MODULE_RESOLVER_H_

#include "third_party/blink/renderer/bindings/core/v8/exception_state.h"
#include "third_party/blink/renderer/core/core_export.h"
#include "third_party/blink/renderer/platform/heap/handle.h"
#include "third_party/blink/renderer/platform/wtf/text/wtf_string.h"

namespace blink {

class ScriptModule;
class ModuleScript;

// The ScriptModuleResolver interface is used from V8 module bindings
// when it need the ScriptModule's descendants.
//
// When a module writes import 'x', the module is called the referrer, 'x' is
// the specifier, and the module identified by 'x' is the descendant.
// ScriptModuleResolver, given a referrer and specifier, can look up the
// descendant.
class CORE_EXPORT ScriptModuleResolver
    : public GarbageCollectedFinalized<ScriptModuleResolver> {
 public:
  virtual ~ScriptModuleResolver() = default;
  virtual void Trace(blink::Visitor* visitor) {}

  // Notifies the ScriptModuleResolver that a ModuleScript exists.
  // This hook gives a chance for the resolver impl to populate module record
  // identifier -> ModuleScript mapping entry.
  virtual void RegisterModuleScript(ModuleScript*) = 0;

  // Notifies the ScriptModuleResolver to clear its ModuleScript mapping.
  virtual void UnregisterModuleScript(ModuleScript*) = 0;

  // Corresponds to the spec concept "[[HostDefined]]".
  virtual ModuleScript* GetHostDefined(const ScriptModule&) const = 0;

  // Implements "Runtime Semantics: HostResolveImportedModule"
  // https://tc39.github.io/ecma262/#sec-hostresolveimportedmodule
  // This returns a null ScriptModule when an exception is thrown.
  virtual ScriptModule Resolve(const String& specifier,
                               const ScriptModule& referrer,
                               ExceptionState&) = 0;
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_CORE_SCRIPT_SCRIPT_MODULE_RESOLVER_H_
