// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_CORE_TESTING_DUMMY_MODULATOR_H_
#define THIRD_PARTY_BLINK_RENDERER_CORE_TESTING_DUMMY_MODULATOR_H_

#include "base/single_thread_task_runner.h"
#include "third_party/blink/renderer/bindings/core/v8/script_module.h"
#include "third_party/blink/renderer/core/script/modulator.h"
#include "third_party/blink/renderer/platform/heap/handle.h"

namespace blink {

class ModuleScriptLoaderClient;
class ScriptModuleResolver;
class ModuleScriptFetchRequest;

// DummyModulator provides empty Modulator interface implementation w/
// NOTREACHED().
//
// DummyModulator is useful for unit-testing.
// Not all module implementation components require full-blown Modulator
// implementation. Unit tests can implement a subset of Modulator interface
// which is exercised from unit-under-test.
class DummyModulator : public Modulator {
  DISALLOW_COPY_AND_ASSIGN(DummyModulator);

 public:
  DummyModulator();
  ~DummyModulator() override;
  void Trace(blink::Visitor*) override;

  ScriptModuleResolver* GetScriptModuleResolver() override;
  base::SingleThreadTaskRunner* TaskRunner() override;
  ReferrerPolicy GetReferrerPolicy() override;
  const SecurityOrigin* GetSecurityOriginForFetch() override;
  ScriptState* GetScriptState() override;

  void FetchTree(const KURL&,
                 WebURLRequest::RequestContext destination,
                 const ScriptFetchOptions&,
                 ModuleTreeClient*) override;
  void FetchSingle(const ModuleScriptFetchRequest&,
                   ModuleGraphLevel,
                   SingleModuleClient*) override;
  void FetchDescendantsForInlineScript(
      ModuleScript*,
      WebURLRequest::RequestContext destination,
      ModuleTreeClient*) override;
  ModuleScript* GetFetchedModuleScript(const KURL&) override;
  void FetchNewSingleModule(const ModuleScriptFetchRequest&,
                            ModuleGraphLevel,
                            ModuleScriptLoaderClient*) override;
  bool HasValidContext() override;
  void ResolveDynamically(const String& specifier,
                          const KURL&,
                          const ReferrerScriptInfo&,
                          ScriptPromiseResolver*) override;
  ModuleImportMeta HostGetImportMetaProperties(ScriptModule) const override;
  ScriptModule CompileModule(const String& script,
                             const KURL& source_url,
                             const KURL& base_url,
                             const ScriptFetchOptions&,
                             AccessControlStatus,
                             const TextPosition&,
                             ExceptionState&) override;
  ScriptValue InstantiateModule(ScriptModule) override;
  Vector<ModuleRequest> ModuleRequestsFromScriptModule(ScriptModule) override;
  ScriptValue ExecuteModule(const ModuleScript*, CaptureEvalErrorFlag) override;
  ModuleScriptFetcher* CreateModuleScriptFetcher() override;

  Member<ScriptModuleResolver> resolver_;
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_CORE_TESTING_DUMMY_MODULATOR_H_
