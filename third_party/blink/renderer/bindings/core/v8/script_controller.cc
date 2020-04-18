/*
 * Copyright (C) 2008, 2009 Google Inc. All rights reserved.
 * Copyright (C) 2009 Apple Inc. All rights reserved.
 * Copyright (C) 2014 Opera Software ASA. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *     * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "third_party/blink/renderer/bindings/core/v8/script_controller.h"

#include "third_party/blink/public/web/web_settings.h"
#include "third_party/blink/renderer/bindings/core/v8/script_source_code.h"
#include "third_party/blink/renderer/bindings/core/v8/v8_binding_for_core.h"
#include "third_party/blink/renderer/bindings/core/v8/v8_gc_controller.h"
#include "third_party/blink/renderer/bindings/core/v8/v8_script_runner.h"
#include "third_party/blink/renderer/bindings/core/v8/window_proxy.h"
#include "third_party/blink/renderer/core/dom/document.h"
#include "third_party/blink/renderer/core/dom/scriptable_document_parser.h"
#include "third_party/blink/renderer/core/dom/user_gesture_indicator.h"
#include "third_party/blink/renderer/core/exported/web_plugin_container_impl.h"
#include "third_party/blink/renderer/core/frame/csp/content_security_policy.h"
#include "third_party/blink/renderer/core/frame/local_frame.h"
#include "third_party/blink/renderer/core/frame/local_frame_client.h"
#include "third_party/blink/renderer/core/frame/settings.h"
#include "third_party/blink/renderer/core/frame/use_counter.h"
#include "third_party/blink/renderer/core/html/html_plugin_element.h"
#include "third_party/blink/renderer/core/inspector/console_message.h"
#include "third_party/blink/renderer/core/inspector/inspector_trace_events.h"
#include "third_party/blink/renderer/core/inspector/main_thread_debugger.h"
#include "third_party/blink/renderer/core/loader/document_loader.h"
#include "third_party/blink/renderer/core/loader/frame_loader.h"
#include "third_party/blink/renderer/core/loader/navigation_scheduler.h"
#include "third_party/blink/renderer/core/loader/progress_tracker.h"
#include "third_party/blink/renderer/core/probe/core_probes.h"
#include "third_party/blink/renderer/platform/histogram.h"
#include "third_party/blink/renderer/platform/instrumentation/tracing/trace_event.h"
#include "third_party/blink/renderer/platform/weborigin/security_origin.h"
#include "third_party/blink/renderer/platform/wtf/assertions.h"
#include "third_party/blink/renderer/platform/wtf/std_lib_extras.h"
#include "third_party/blink/renderer/platform/wtf/text/cstring.h"
#include "third_party/blink/renderer/platform/wtf/text/string_builder.h"
#include "third_party/blink/renderer/platform/wtf/time.h"

namespace blink {

void ScriptController::Trace(blink::Visitor* visitor) {
  visitor->Trace(frame_);
  visitor->Trace(window_proxy_manager_);
}

void ScriptController::ClearForClose() {
  window_proxy_manager_->ClearForClose();
  MainThreadDebugger::Instance()->DidClearContextsForFrame(GetFrame());
}

void ScriptController::UpdateSecurityOrigin(
    const SecurityOrigin* security_origin) {
  window_proxy_manager_->UpdateSecurityOrigin(security_origin);
}

namespace {

V8CacheOptions CacheOptions(const CachedMetadataHandler* cache_handler,
                            const Settings* settings) {
  V8CacheOptions v8_cache_options(kV8CacheOptionsDefault);
  if (settings) {
    v8_cache_options = settings->GetV8CacheOptions();
    if (v8_cache_options == kV8CacheOptionsNone)
      return kV8CacheOptionsNone;
  }
  // If the resource is served from CacheStorage, generate the V8 code cache in
  // the first load.
  if (cache_handler && cache_handler->IsServedFromCacheStorage())
    return kV8CacheOptionsCodeWithoutHeatCheck;
  return v8_cache_options;
}

}  // namespace

v8::Local<v8::Value> ScriptController::ExecuteScriptAndReturnValue(
    v8::Local<v8::Context> context,
    const ScriptSourceCode& source,
    const KURL& base_url,
    const ScriptFetchOptions& fetch_options,
    AccessControlStatus access_control_status) {
  TRACE_EVENT1(
      "devtools.timeline", "EvaluateScript", "data",
      InspectorEvaluateScriptEvent::Data(GetFrame(), source.Url().GetString(),
                                         source.StartPosition()));
  v8::Local<v8::Value> result;
  {
    CachedMetadataHandler* cache_handler = source.CacheHandler();

    V8CacheOptions v8_cache_options =
        CacheOptions(cache_handler, GetFrame()->GetSettings());

    // Isolate exceptions that occur when compiling and executing
    // the code. These exceptions should not interfere with
    // javascript code we might evaluate from C++ when returning
    // from here.
    v8::TryCatch try_catch(GetIsolate());
    try_catch.SetVerbose(true);

    // Omit storing base URL if it is same as source URL.
    // Note: This improves chance of getting into a fast path in
    //       ReferrerScriptInfo::ToV8HostDefinedOptions.
    KURL stored_base_url = (base_url == source.Url()) ? KURL() : base_url;
    const ReferrerScriptInfo referrer_info(stored_base_url, fetch_options);

    v8::Local<v8::Script> script;

    v8::ScriptCompiler::CompileOptions compile_options;
    V8ScriptRunner::ProduceCacheOptions produce_cache_options;
    v8::ScriptCompiler::NoCacheReason no_cache_reason;
    std::tie(compile_options, produce_cache_options, no_cache_reason) =
        V8ScriptRunner::GetCompileOptions(v8_cache_options, source);
    if (!V8ScriptRunner::CompileScript(ScriptState::From(context), source,
                                       access_control_status, compile_options,
                                       no_cache_reason, referrer_info)
             .ToLocal(&script))
      return result;

    v8::MaybeLocal<v8::Value> maybe_result;
    maybe_result = V8ScriptRunner::RunCompiledScript(GetIsolate(), script,
                                                     GetFrame()->GetDocument());
    V8ScriptRunner::ProduceCache(GetIsolate(), script, source,
                                 produce_cache_options, compile_options);

    if (!maybe_result.ToLocal(&result)) {
      return result;
    }
  }

  return result;
}

bool ScriptController::ShouldBypassMainWorldCSP() {
  v8::HandleScope handle_scope(GetIsolate());
  v8::Local<v8::Context> context = GetIsolate()->GetCurrentContext();
  if (context.IsEmpty() || !ToLocalDOMWindow(context))
    return false;
  DOMWrapperWorld& world = DOMWrapperWorld::Current(GetIsolate());
  return world.IsIsolatedWorld() ? world.IsolatedWorldHasContentSecurityPolicy()
                                 : false;
}

TextPosition ScriptController::EventHandlerPosition() const {
  ScriptableDocumentParser* parser =
      GetFrame()->GetDocument()->GetScriptableDocumentParser();
  if (parser)
    return parser->GetTextPosition();
  return TextPosition::MinimumPosition();
}

void ScriptController::EnableEval() {
  v8::HandleScope handle_scope(GetIsolate());
  v8::Local<v8::Context> v8_context =
      window_proxy_manager_->MainWorldProxyMaybeUninitialized()
          ->ContextIfInitialized();
  if (v8_context.IsEmpty())
    return;
  v8_context->AllowCodeGenerationFromStrings(true);
}

void ScriptController::DisableEval(const String& error_message) {
  v8::HandleScope handle_scope(GetIsolate());
  v8::Local<v8::Context> v8_context =
      window_proxy_manager_->MainWorldProxyMaybeUninitialized()
          ->ContextIfInitialized();
  if (v8_context.IsEmpty())
    return;
  v8_context->AllowCodeGenerationFromStrings(false);
  v8_context->SetErrorMessageForCodeGenerationFromStrings(
      V8String(GetIsolate(), error_message));
}

V8Extensions& ScriptController::RegisteredExtensions() {
  DEFINE_STATIC_LOCAL(V8Extensions, extensions, ());
  return extensions;
}

void ScriptController::RegisterExtensionIfNeeded(v8::Extension* extension) {
  const V8Extensions& extensions = RegisteredExtensions();
  for (size_t i = 0; i < extensions.size(); ++i) {
    if (extensions[i] == extension)
      return;
  }
  v8::RegisterExtension(extension);
  RegisteredExtensions().push_back(extension);
}

void ScriptController::ClearWindowProxy() {
  // V8 binding expects ScriptController::clearWindowProxy only be called when a
  // frame is loading a new page. This creates a new context for the new page.
  window_proxy_manager_->ClearForNavigation();
  MainThreadDebugger::Instance()->DidClearContextsForFrame(GetFrame());
}

void ScriptController::UpdateDocument() {
  window_proxy_manager_->MainWorldProxyMaybeUninitialized()->UpdateDocument();
  EnableEval();
}

bool ScriptController::ExecuteScriptIfJavaScriptURL(const KURL& url,
                                                    Element* element) {
  if (!url.ProtocolIsJavaScript())
    return false;

  const int kJavascriptSchemeLength = sizeof("javascript:") - 1;
  String script_source = DecodeURLEscapeSequences(url.GetString())
                             .Substring(kJavascriptSchemeLength);

  bool should_bypass_main_world_content_security_policy =
      ContentSecurityPolicy::ShouldBypassMainWorld(GetFrame()->GetDocument());
  if (!GetFrame()->GetPage() ||
      (!should_bypass_main_world_content_security_policy &&
       !GetFrame()
            ->GetDocument()
            ->GetContentSecurityPolicy()
            ->AllowJavaScriptURLs(element, script_source,
                                  GetFrame()->GetDocument()->Url(),
                                  EventHandlerPosition().line_))) {
    return true;
  }

  bool progress_notifications_needed =
      GetFrame()->Loader().StateMachine()->IsDisplayingInitialEmptyDocument() &&
      !GetFrame()->IsLoading();
  if (progress_notifications_needed)
    GetFrame()->Loader().Progress().ProgressStarted(kFrameLoadTypeStandard);

  Document* owner_document = GetFrame()->GetDocument();

  bool location_change_before =
      GetFrame()->GetNavigationScheduler().LocationChangePending();

  v8::HandleScope handle_scope(GetIsolate());

  // https://html.spec.whatwg.org/multipage/browsing-the-web.html#navigate
  // Step 12.8 "Let base URL be settings object's API base URL." [spec text]
  KURL base_url = owner_document->BaseURL();

  // Step 12.9 "Let script be result of creating a classic script given script
  // source, settings, base URL, and the default classic script fetch options."
  // [spec text]
  v8::Local<v8::Value> result = EvaluateScriptInMainWorld(
      ScriptSourceCode(script_source, ScriptSourceLocationType::kJavascriptUrl),
      base_url, ScriptFetchOptions(), kNotSharableCrossOrigin,
      kDoNotExecuteScriptWhenScriptsDisabled);

  // If executing script caused this frame to be removed from the page, we
  // don't want to try to replace its document!
  if (!GetFrame()->GetPage())
    return true;

  if (result.IsEmpty() || !result->IsString()) {
    if (progress_notifications_needed)
      GetFrame()->Loader().Progress().ProgressCompleted();
    return true;
  }
  String script_result = ToCoreString(v8::Local<v8::String>::Cast(result));

  // We're still in a frame, so there should be a DocumentLoader.
  DCHECK(GetFrame()->GetDocument()->Loader());
  if (!location_change_before &&
      GetFrame()->GetNavigationScheduler().LocationChangePending())
    return true;

  GetFrame()->Loader().ReplaceDocumentWhileExecutingJavaScriptURL(
      script_result, owner_document);
  return true;
}

void ScriptController::ExecuteScriptInMainWorld(
    const String& script,
    ScriptSourceLocationType source_location_type,
    ExecuteScriptPolicy policy) {
  v8::HandleScope handle_scope(GetIsolate());
  EvaluateScriptInMainWorld(ScriptSourceCode(script, source_location_type),
                            KURL(), ScriptFetchOptions(),
                            kNotSharableCrossOrigin, policy);
}

void ScriptController::ExecuteScriptInMainWorld(
    const ScriptSourceCode& source_code,
    const KURL& base_url,
    const ScriptFetchOptions& fetch_options,
    AccessControlStatus access_control_status) {
  v8::HandleScope handle_scope(GetIsolate());
  EvaluateScriptInMainWorld(source_code, base_url, fetch_options,
                            access_control_status,
                            kDoNotExecuteScriptWhenScriptsDisabled);
}

v8::Local<v8::Value> ScriptController::ExecuteScriptInMainWorldAndReturnValue(
    const ScriptSourceCode& source_code,
    const KURL& base_url,
    const ScriptFetchOptions& fetch_options,
    ExecuteScriptPolicy policy) {
  return EvaluateScriptInMainWorld(source_code, base_url, fetch_options,
                                   kNotSharableCrossOrigin, policy);
}

v8::Local<v8::Value> ScriptController::EvaluateScriptInMainWorld(
    const ScriptSourceCode& source_code,
    const KURL& base_url,
    const ScriptFetchOptions& fetch_options,
    AccessControlStatus access_control_status,
    ExecuteScriptPolicy policy) {
  if (policy == kDoNotExecuteScriptWhenScriptsDisabled &&
      !GetFrame()->GetDocument()->CanExecuteScripts(kAboutToExecuteScript))
    return v8::Local<v8::Value>();

  // TODO(dcheng): Clean this up to not use ScriptState, to match
  // executeScriptInIsolatedWorld.
  ScriptState* script_state = ToScriptStateForMainWorld(GetFrame());
  if (!script_state)
    return v8::Local<v8::Value>();
  v8::EscapableHandleScope handle_scope(GetIsolate());
  ScriptState::Scope scope(script_state);

  if (GetFrame()->Loader().StateMachine()->IsDisplayingInitialEmptyDocument())
    GetFrame()->Loader().DidAccessInitialDocument();

  v8::Local<v8::Value> object = ExecuteScriptAndReturnValue(
      script_state->GetContext(), source_code, base_url, fetch_options,
      access_control_status);

  if (object.IsEmpty())
    return v8::Local<v8::Value>();

  return handle_scope.Escape(object);
}

void ScriptController::ExecuteScriptInIsolatedWorld(
    int world_id,
    const HeapVector<ScriptSourceCode>& sources,
    Vector<v8::Local<v8::Value>>* results) {
  DCHECK_GT(world_id, 0);

  scoped_refptr<DOMWrapperWorld> world =
      DOMWrapperWorld::EnsureIsolatedWorld(GetIsolate(), world_id);
  LocalWindowProxy* isolated_world_window_proxy = WindowProxy(*world);
  // TODO(dcheng): Context must always be initialized here, due to the call to
  // windowProxy() on the previous line. Add a helper which makes that obvious?
  v8::Local<v8::Context> context =
      isolated_world_window_proxy->ContextIfInitialized();
  v8::Context::Scope scope(context);
  v8::Local<v8::Array> result_array =
      v8::Array::New(GetIsolate(), sources.size());

  for (size_t i = 0; i < sources.size(); ++i) {
    v8::Local<v8::Value> evaluation_result =
        ExecuteScriptAndReturnValue(context, sources[i]);
    if (evaluation_result.IsEmpty())
      evaluation_result =
          v8::Local<v8::Value>::New(GetIsolate(), v8::Undefined(GetIsolate()));
    bool did_create;
    if (!result_array->CreateDataProperty(context, i, evaluation_result)
             .To(&did_create) ||
        !did_create)
      return;
  }

  if (results) {
    for (size_t i = 0; i < result_array->Length(); ++i) {
      v8::Local<v8::Value> value;
      if (!result_array->Get(context, i).ToLocal(&value))
        return;
      results->push_back(value);
    }
  }
}

scoped_refptr<DOMWrapperWorld>
ScriptController::CreateNewInspectorIsolatedWorld(const String& world_name) {
  scoped_refptr<DOMWrapperWorld> world = DOMWrapperWorld::Create(
      GetIsolate(), DOMWrapperWorld::WorldType::kInspectorIsolated);
  // Bail out if we could not create an isolated world.
  if (!world)
    return nullptr;
  if (!world_name.IsEmpty()) {
    DOMWrapperWorld::SetNonMainWorldHumanReadableName(world->GetWorldId(),
                                                      world_name);
  }
  // Make sure the execution context exists.
  WindowProxy(*world);
  return world;
}

}  // namespace blink
