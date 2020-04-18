// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/modules/serviceworkers/service_worker_installed_scripts_manager.h"

#include <memory>
#include <utility>

#include "third_party/blink/renderer/core/html/parser/text_resource_decoder.h"
#include "third_party/blink/renderer/modules/serviceworkers/service_worker_thread.h"
#include "third_party/blink/renderer/platform/wtf/text/string_builder.h"

namespace blink {

ServiceWorkerInstalledScriptsManager::ServiceWorkerInstalledScriptsManager(
    std::unique_ptr<WebServiceWorkerInstalledScriptsManager> manager)
    : manager_(std::move(manager)) {
  DCHECK(manager_);
}

bool ServiceWorkerInstalledScriptsManager::IsScriptInstalled(
    const KURL& script_url) const {
  return manager_->IsScriptInstalled(script_url);
}

InstalledScriptsManager::ScriptStatus
ServiceWorkerInstalledScriptsManager::GetScriptData(
    const KURL& script_url,
    InstalledScriptsManager::ScriptData* out_script_data) {
  DCHECK(!IsMainThread());
  // This blocks until the script is received from the browser.
  std::unique_ptr<WebServiceWorkerInstalledScriptsManager::RawScriptData>
      raw_script_data = manager_->GetRawScriptData(script_url);
  DCHECK(raw_script_data);
  if (!raw_script_data->IsValid()) {
    *out_script_data = InstalledScriptsManager::ScriptData();
    return ScriptStatus::kFailed;
  }

  // This is from WorkerClassicScriptLoader::DidReceiveData.
  std::unique_ptr<TextResourceDecoder> decoder =
      TextResourceDecoder::Create(TextResourceDecoderOptions(
          TextResourceDecoderOptions::kPlainTextContent,
          raw_script_data->Encoding().IsEmpty()
              ? UTF8Encoding()
              : WTF::TextEncoding(raw_script_data->Encoding())));

  StringBuilder source_text_builder;
  for (const auto& chunk : raw_script_data->ScriptTextChunks())
    source_text_builder.Append(decoder->Decode(chunk.Data(), chunk.size()));

  std::unique_ptr<Vector<char>> meta_data;
  if (raw_script_data->MetaDataChunks().size() > 0) {
    size_t total_metadata_size = 0;
    for (const auto& chunk : raw_script_data->MetaDataChunks())
      total_metadata_size += chunk.size();
    meta_data = std::make_unique<Vector<char>>();
    meta_data->ReserveInitialCapacity(total_metadata_size);
    for (const auto& chunk : raw_script_data->MetaDataChunks())
      meta_data->Append(chunk.Data(), chunk.size());
  }

  InstalledScriptsManager::ScriptData script_data(
      script_url, source_text_builder.ToString(), std::move(meta_data),
      raw_script_data->TakeHeaders());
  *out_script_data = std::move(script_data);
  return ScriptStatus::kSuccess;
}

}  // namespace blink
