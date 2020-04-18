// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/core/script/script_module_resolver_impl.h"

#include "third_party/blink/renderer/bindings/core/v8/script_module.h"
#include "third_party/blink/renderer/core/script/modulator.h"
#include "third_party/blink/renderer/core/script/module_script.h"

namespace blink {

void ScriptModuleResolverImpl::RegisterModuleScript(
    ModuleScript* module_script) {
  DCHECK(module_script);
  if (module_script->Record().IsNull())
    return;

  DVLOG(1) << "ScriptModuleResolverImpl::RegisterModuleScript(url="
           << module_script->BaseURL().GetString()
           << ", hash=" << ScriptModuleHash::GetHash(module_script->Record())
           << ")";

  auto result =
      record_to_module_script_map_.Set(module_script->Record(), module_script);
  DCHECK(result.is_new_entry);
}

void ScriptModuleResolverImpl::UnregisterModuleScript(
    ModuleScript* module_script) {
  DCHECK(module_script);
  if (module_script->Record().IsNull())
    return;

  DVLOG(1) << "ScriptModuleResolverImpl::UnregisterModuleScript(url="
           << module_script->BaseURL().GetString()
           << ", hash=" << ScriptModuleHash::GetHash(module_script->Record())
           << ")";

  record_to_module_script_map_.erase(module_script->Record());
}

ModuleScript* ScriptModuleResolverImpl::GetHostDefined(
    const ScriptModule& record) const {
  const auto it = record_to_module_script_map_.find(record);
  CHECK_NE(it, record_to_module_script_map_.end())
      << "Failed to find ModuleScript corresponding to the "
         "record.[[HostDefined]]";
  CHECK(it->value);
  return it->value;
}

// https://html.spec.whatwg.org/multipage/webappapis.html#hostresolveimportedmodule(referencingscriptormodule,-specifier)
ScriptModule ScriptModuleResolverImpl::Resolve(
    const String& specifier,
    const ScriptModule& referrer,
    ExceptionState& exception_state) {
  DVLOG(1) << "ScriptModuleResolverImpl::resolve(specifier=\"" << specifier
           << ", referrer.hash=" << ScriptModuleHash::GetHash(referrer) << ")";

  // Step 1. Let referencing module script be referencingModule.[[HostDefined]].
  ModuleScript* referrer_module = GetHostDefined(referrer);

  // Step 2. Let moduleMap be referencing module script's settings object's
  // module map. Note: Blink finds out "module script's settings object"
  // (Modulator) from context where HostResolveImportedModule was called.

  // Step 3. Let url be the result of resolving a module specifier given
  // referencing module script and specifier.
  KURL url = referrer_module->ResolveModuleSpecifier(specifier);

  // Step 4. Assert: url is never failure, because resolving a module specifier
  // must have been previously successful with these same two arguments.
  DCHECK(url.IsValid());

  // Step 5. Let resolved module script be moduleMap[url]. (This entry must
  // exist for us to have gotten to this point.)
  ModuleScript* module_script = modulator_->GetFetchedModuleScript(url);

  // Step 6. Assert: resolved module script is a module script (i.e., is not
  // null or "fetching").
  // Step 7. Assert: resolved module script's record is not null.
  DCHECK(module_script);
  CHECK(!module_script->Record().IsNull());

  // Step 8. Return resolved module script's module record.
  return module_script->Record();
}

void ScriptModuleResolverImpl::ContextDestroyed(ExecutionContext*) {
  // crbug.com/725816 : What we should really do is to make the map key
  // weak reference to v8::Module.
  record_to_module_script_map_.clear();
}

void ScriptModuleResolverImpl::Trace(blink::Visitor* visitor) {
  ScriptModuleResolver::Trace(visitor);
  ContextLifecycleObserver::Trace(visitor);
  visitor->Trace(record_to_module_script_map_);
  visitor->Trace(modulator_);
}

}  // namespace blink
