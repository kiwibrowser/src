// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_CORE_SCRIPT_MODULE_PENDING_SCRIPT_H_
#define THIRD_PARTY_BLINK_RENDERER_CORE_SCRIPT_MODULE_PENDING_SCRIPT_H_

#include "third_party/blink/renderer/core/script/modulator.h"
#include "third_party/blink/renderer/core/script/module_script.h"
#include "third_party/blink/renderer/core/script/pending_script.h"
#include "third_party/blink/renderer/platform/bindings/script_wrappable.h"
#include "third_party/blink/renderer/platform/bindings/trace_wrapper_member.h"

namespace blink {

class ModulePendingScript;

// ModulePendingScriptTreeClient is used to connect from Modulator::FetchTree()
// to ModulePendingScript. Because ModulePendingScript is created after
// Modulator::FetchTree() is called, ModulePendingScriptTreeClient is
// registered as ModuleTreeClient to FetchTree() first, and later
// ModulePendingScript is supplied to ModulePendingScriptTreeClient via
// SetPendingScript() and is notified of module tree load finish.
class ModulePendingScriptTreeClient final : public ModuleTreeClient {
 public:
  static ModulePendingScriptTreeClient* Create() {
    return new ModulePendingScriptTreeClient();
  }
  ~ModulePendingScriptTreeClient() override = default;

  void SetPendingScript(ModulePendingScript* client);

  ModuleScript* GetModuleScript() const { return module_script_; }

  void Trace(blink::Visitor*) override;
  void TraceWrappers(ScriptWrappableVisitor*) const override;

 private:
  ModulePendingScriptTreeClient();

  // Implements ModuleTreeClient
  void NotifyModuleTreeLoadFinished(ModuleScript*) override;

  bool finished_ = false;
  TraceWrapperMember<ModuleScript> module_script_;
  TraceWrapperMember<ModulePendingScript> pending_script_;
};

// PendingScript for a module script
// https://html.spec.whatwg.org/multipage/webappapis.html#module-script.
class CORE_EXPORT ModulePendingScript : public PendingScript {
 public:
  static ModulePendingScript* Create(ScriptElementBase* element,
                                     ModulePendingScriptTreeClient* client,
                                     bool is_external) {
    return new ModulePendingScript(element, client, is_external);
  }

  ~ModulePendingScript() override;

  void NotifyModuleTreeLoadFinished();

  ModuleScript* GetModuleScript() const {
    return module_tree_client_->GetModuleScript();
  }

  void Trace(blink::Visitor*) override;
  void TraceWrappers(ScriptWrappableVisitor*) const override;

 private:
  ModulePendingScript(ScriptElementBase*,
                      ModulePendingScriptTreeClient*,
                      bool is_external);

  // PendingScript
  ScriptType GetScriptType() const override { return ScriptType::kModule; }
  Script* GetSource(const KURL& document_url,
                    bool& error_occurred) const override;
  bool IsReady() const override { return ready_; }
  bool IsExternal() const override { return is_external_; }
  bool ErrorOccurred() const override;
  bool WasCanceled() const override { return false; }

  bool StartStreamingIfPossible(ScriptStreamer::Type,
                                base::OnceClosure) override {
    return false;
  }
  bool IsCurrentlyStreaming() const override { return false; }

  KURL UrlForTracing() const override { return NullURL(); }

  void DisposeInternal() override;

  void CheckState() const override {}

  TraceWrapperMember<ModulePendingScriptTreeClient> module_tree_client_;
  bool ready_ = false;
  const bool is_external_;
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_CORE_SCRIPT_MODULE_PENDING_SCRIPT_H_
