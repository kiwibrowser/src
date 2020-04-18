// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/core/script/modulator.h"

#include "third_party/blink/renderer/bindings/core/v8/v8_binding_for_core.h"
#include "third_party/blink/renderer/core/dom/document.h"
#include "third_party/blink/renderer/core/frame/local_dom_window.h"
#include "third_party/blink/renderer/core/frame/local_frame.h"
#include "third_party/blink/renderer/core/script/document_modulator_impl.h"
#include "third_party/blink/renderer/core/script/layered_api.h"
#include "third_party/blink/renderer/core/script/worker_modulator_impl.h"
#include "third_party/blink/renderer/core/script/worklet_modulator_impl.h"
#include "third_party/blink/renderer/core/workers/worklet_global_scope.h"
#include "third_party/blink/renderer/platform/bindings/script_state.h"
#include "third_party/blink/renderer/platform/bindings/v8_per_context_data.h"

namespace blink {

namespace {
const char kPerContextDataKey[] = "Modulator";
}  // namespace

Modulator* Modulator::From(ScriptState* script_state) {
  if (!script_state)
    return nullptr;

  V8PerContextData* per_context_data = script_state->PerContextData();
  if (!per_context_data)
    return nullptr;

  Modulator* modulator =
      static_cast<Modulator*>(per_context_data->GetData(kPerContextDataKey));
  if (modulator)
    return modulator;
  ExecutionContext* execution_context = ExecutionContext::From(script_state);
  if (execution_context->IsDocument()) {
    Document* document = ToDocument(execution_context);
    modulator =
        DocumentModulatorImpl::Create(script_state, document->Fetcher());
    Modulator::SetModulator(script_state, modulator);

    // See comment in LocalDOMWindow::modulator_ for this workaround.
    LocalDOMWindow* window = document->ExecutingWindow();
    window->SetModulator(modulator);
  } else if (execution_context->IsWorkletGlobalScope()) {
    modulator = WorkletModulatorImpl::Create(script_state);
    Modulator::SetModulator(script_state, modulator);

    // See comment in WorkerOrWorkletGlobalScope::modulator_ for this
    // workaround.
    ToWorkerOrWorkletGlobalScope(execution_context)->SetModulator(modulator);
  } else if (execution_context->IsWorkerGlobalScope()) {
    modulator = WorkerModulatorImpl::Create(script_state);
    Modulator::SetModulator(script_state, modulator);

    // See comment in WorkerOrWorkletGlobalScope::modulator_ for this
    // workaround.
    ToWorkerOrWorkletGlobalScope(execution_context)->SetModulator(modulator);
  } else {
    NOTREACHED();
  }
  return modulator;
}

Modulator::~Modulator() {}

void Modulator::SetModulator(ScriptState* script_state, Modulator* modulator) {
  DCHECK(script_state);
  V8PerContextData* per_context_data = script_state->PerContextData();
  DCHECK(per_context_data);
  per_context_data->AddData(kPerContextDataKey, modulator);
}

void Modulator::ClearModulator(ScriptState* script_state) {
  DCHECK(script_state);
  V8PerContextData* per_context_data = script_state->PerContextData();
  DCHECK(per_context_data);
  per_context_data->ClearData(kPerContextDataKey);
}

// https://html.spec.whatwg.org/multipage/webappapis.html#resolve-a-module-specifier
KURL Modulator::ResolveModuleSpecifier(const String& module_request,
                                       const KURL& base_url,
                                       String* failure_reason) {
  // <spec step="1">Apply the URL parser to specifier. If the result is not
  // failure, return the result.</spec>
  KURL url(NullURL(), module_request);
  if (url.IsValid()) {
    // <spec
    // href="https://github.com/drufball/layered-apis/blob/master/spec.md#resolve-a-module-specifier"
    // step="1">Let parsed be the result of applying the URL parser to
    // specifier. If parsed is not failure, then return the layered API fetching
    // URL given parsed and script's base URL.</spec>
    if (RuntimeEnabledFeatures::LayeredAPIEnabled())
      return blink::layered_api::ResolveFetchingURL(url, base_url);

    return url;
  }

  // <spec step="2">If specifier does not start with the character U+002F
  // SOLIDUS (/), the two-character sequence U+002E FULL STOP, U+002F SOLIDUS
  // (./), or the three-character sequence U+002E FULL STOP, U+002E FULL STOP,
  // U+002F SOLIDUS (../), return failure.</spec>
  //
  // (../), return failure and abort these steps." [spec text]
  if (!module_request.StartsWith("/") && !module_request.StartsWith("./") &&
      !module_request.StartsWith("../")) {
    if (failure_reason) {
      *failure_reason =
          "Relative references must start with either \"/\", \"./\", or "
          "\"../\".";
    }
    return KURL();
  }

  // <spec step="3">Return the result of applying the URL parser to specifier
  // with script's base URL as the base URL.</spec>
  DCHECK(base_url.IsValid());
  KURL absolute_url(base_url, module_request);
  if (absolute_url.IsValid())
    return absolute_url;

  if (failure_reason) {
    *failure_reason = "Invalid relative url or base scheme isn't hierarchical.";
  }
  return KURL();
}

}  // namespace blink
