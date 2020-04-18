// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/core/script/document_modulator_impl.h"

#include "third_party/blink/renderer/core/loader/modulescript/document_module_script_fetcher.h"

namespace blink {

ModulatorImplBase* DocumentModulatorImpl::Create(
    scoped_refptr<ScriptState> script_state,
    ResourceFetcher* resource_fetcher) {
  return new DocumentModulatorImpl(std::move(script_state), resource_fetcher);
}

ModuleScriptFetcher* DocumentModulatorImpl::CreateModuleScriptFetcher() {
  return new DocumentModuleScriptFetcher(fetcher_);
}

void DocumentModulatorImpl::Trace(blink::Visitor* visitor) {
  visitor->Trace(fetcher_);
  ModulatorImplBase::Trace(visitor);
}

DocumentModulatorImpl::DocumentModulatorImpl(
    scoped_refptr<ScriptState> script_state,
    ResourceFetcher* resource_fetcher)
    : ModulatorImplBase(std::move(script_state)), fetcher_(resource_fetcher) {
  DCHECK(fetcher_);
}

bool DocumentModulatorImpl::IsDynamicImportForbidden(String* reason) {
  return false;
}

}  // namespace blink
