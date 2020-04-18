// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/public/platform/modules/serviceworker/web_service_worker_installed_scripts_manager.h"

#include <memory>
#include <utility>

#include "base/memory/ptr_util.h"

namespace blink {

// static
std::unique_ptr<WebServiceWorkerInstalledScriptsManager::RawScriptData>
WebServiceWorkerInstalledScriptsManager::RawScriptData::Create(
    WebString encoding,
    WebVector<BytesChunk> script_text,
    WebVector<BytesChunk> meta_data) {
  return base::WrapUnique(
      new RawScriptData(std::move(encoding), std::move(script_text),
                        std::move(meta_data), true /* is_valid */));
}

// static
std::unique_ptr<WebServiceWorkerInstalledScriptsManager::RawScriptData>
WebServiceWorkerInstalledScriptsManager::RawScriptData::
    CreateInvalidInstance() {
  return base::WrapUnique(
      new RawScriptData(WebString() /* encoding */, WebVector<BytesChunk>(),
                        WebVector<BytesChunk>(), false /* is_valid */));
}

WebServiceWorkerInstalledScriptsManager::RawScriptData::RawScriptData(
    WebString encoding,
    WebVector<BytesChunk> script_text,
    WebVector<BytesChunk> meta_data,
    bool is_valid)
    : is_valid_(is_valid),
      encoding_(std::move(encoding)),
      script_text_(std::move(script_text)),
      meta_data_(std::move(meta_data)),
      headers_(std::make_unique<CrossThreadHTTPHeaderMapData>()) {}

WebServiceWorkerInstalledScriptsManager::RawScriptData::~RawScriptData() =
    default;

void WebServiceWorkerInstalledScriptsManager::RawScriptData::AddHeader(
    const WebString& key,
    const WebString& value) {
  headers_->emplace_back(key, value);
}

}  // namespace blink
