// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_CORE_SCRIPT_MODULE_SCRIPT_H_
#define THIRD_PARTY_BLINK_RENDERER_CORE_SCRIPT_MODULE_SCRIPT_H_

#include "third_party/blink/renderer/bindings/core/v8/script_module.h"
#include "third_party/blink/renderer/bindings/core/v8/script_value.h"
#include "third_party/blink/renderer/core/core_export.h"
#include "third_party/blink/renderer/core/script/modulator.h"
#include "third_party/blink/renderer/core/script/script.h"
#include "third_party/blink/renderer/platform/bindings/script_wrappable.h"
#include "third_party/blink/renderer/platform/bindings/trace_wrapper_v8_reference.h"
#include "third_party/blink/renderer/platform/heap/handle.h"
#include "third_party/blink/renderer/platform/weborigin/kurl.h"
#include "third_party/blink/renderer/platform/weborigin/kurl_hash.h"
#include "third_party/blink/renderer/platform/wtf/hash_map.h"
#include "third_party/blink/renderer/platform/wtf/text/text_position.h"

namespace blink {

// ModuleScript is a model object for the "module script" spec concept.
// https://html.spec.whatwg.org/multipage/webappapis.html#module-script
class CORE_EXPORT ModuleScript final : public Script, public TraceWrapperBase {
 public:
  // https://html.spec.whatwg.org/multipage/webappapis.html#creating-a-module-script
  static ModuleScript* Create(
      const String& source_text,
      Modulator*,
      const KURL& source_url,
      const KURL& base_url,
      const ScriptFetchOptions&,
      AccessControlStatus,
      const TextPosition& start_position = TextPosition::MinimumPosition());

  // Mostly corresponds to Create() but accepts ScriptModule as the argument
  // and allows null ScriptModule.
  static ModuleScript* CreateForTest(
      Modulator*,
      ScriptModule,
      const KURL& base_url,
      const ScriptFetchOptions& = ScriptFetchOptions());

  ~ModuleScript() override = default;

  ScriptModule Record() const;
  bool HasEmptyRecord() const;

  void SetParseErrorAndClearRecord(ScriptValue error);
  bool HasParseError() const { return !parse_error_.IsEmpty(); }
  ScriptValue CreateParseError() const;

  void SetErrorToRethrow(ScriptValue error);
  bool HasErrorToRethrow() const { return !error_to_rethrow_.IsEmpty(); }
  ScriptValue CreateErrorToRethrow() const;

  const TextPosition& StartPosition() const { return start_position_; }

  // Resolves a module specifier with the module script's base URL.
  KURL ResolveModuleSpecifier(const String& module_request,
                              String* failure_reason = nullptr);

  void Trace(blink::Visitor*) override;
  void TraceWrappers(ScriptWrappableVisitor*) const override;
  const char* NameInHeapSnapshot() const override { return "ModuleScript"; }

 private:
  ModuleScript(Modulator* settings_object,
               ScriptModule record,
               const KURL& source_url,
               const KURL& base_url,
               const ScriptFetchOptions&,
               const String& source_text,
               const TextPosition& start_position);

  static ModuleScript* CreateInternal(const String& source_text,
                                      Modulator*,
                                      ScriptModule,
                                      const KURL& source_url,
                                      const KURL& base_url,
                                      const ScriptFetchOptions&,
                                      const TextPosition&);

  ScriptType GetScriptType() const override { return ScriptType::kModule; }
  void RunScript(LocalFrame*, const SecurityOrigin*) const override;
  String InlineSourceTextForCSP() const override;

  friend class ModulatorImplBase;
  friend class ModuleTreeLinkerTestModulator;

  // https://html.spec.whatwg.org/multipage/webappapis.html#settings-object
  Member<Modulator> settings_object_;

  // https://html.spec.whatwg.org/multipage/webappapis.html#concept-script-record
  TraceWrapperV8Reference<v8::Module> record_;

  // https://html.spec.whatwg.org/multipage/webappapis.html#concept-script-parse-error
  //
  // |record_|, |parse_error_| and |error_to_rethrow_| are TraceWrappers()ed and
  // kept alive via one or more of following reference graphs:
  // * non-inline module script case
  //   DOMWindow -> Modulator/ModulatorImpl -> ModuleMap -> ModuleMap::Entry
  //   -> ModuleScript
  // * inline module script case, before the PendingScript is created.
  //   DOMWindow -> Modulator/ModulatorImpl -> ModuleTreeLinkerRegistry
  //   -> ModuleTreeLinker -> ModuleScript
  // * inline module script case, after the PendingScript is created.
  //   HTMLScriptElement -> ScriptLoader -> ModulePendingScript
  //   -> ModulePendingScriptTreeClient -> ModuleScript.
  // * inline module script case, queued in HTMLParserScriptRunner,
  //   when HTMLScriptElement is removed before execution.
  //   Document -> HTMLDocumentParser -> HTMLParserScriptRunner
  //   -> ModulePendingScript -> ModulePendingScriptTreeClient
  //   -> ModuleScript.
  // * inline module script case, queued in ScriptRunner.
  //   Document -> ScriptRunner -> ScriptLoader -> ModulePendingScript
  //   -> ModulePendingScriptTreeClient -> ModuleScript.
  // All the classes/references on the graphs above should be
  // TraceWrapperBase/TraceWrapperMember<>/etc.,
  //
  // A parse error and an error to rethrow belong to a script, not to a
  // |parse_error_| and |error_to_rethrow_| should belong to a script (i.e.
  // blink::Script) according to the spec, but are put here in ModuleScript,
  // because:
  // - Error handling for classic and module scripts are somehow separated and
  //   there are no urgent motivation for merging the error handling and placing
  //   the errors in Script, and
  // - Classic scripts are handled according to the spec before
  //   https://github.com/whatwg/html/pull/2991. This shouldn't cause any
  //   observable functional changes, and updating the classic script handling
  //   will require moderate code changes (e.g. to move compilation timing).
  TraceWrapperV8Reference<v8::Value> parse_error_;

  // https://html.spec.whatwg.org/multipage/webappapis.html#concept-script-error-to-rethrow
  TraceWrapperV8Reference<v8::Value> error_to_rethrow_;

  // For CSP check.
  const String source_text_;

  const TextPosition start_position_;
  HashMap<String, KURL> specifier_to_url_cache_;
  KURL source_url_;
};

CORE_EXPORT std::ostream& operator<<(std::ostream&, const ModuleScript&);

}  // namespace blink

#endif
