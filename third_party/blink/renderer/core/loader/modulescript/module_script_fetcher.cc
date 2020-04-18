// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/core/loader/modulescript/module_script_fetcher.h"

namespace blink {

void ModuleScriptFetcher::Trace(blink::Visitor* visitor) {
  visitor->Trace(client_);
}

void ModuleScriptFetcher::NotifyFetchFinished(
    const base::Optional<ModuleScriptCreationParams>& params,
    const HeapVector<Member<ConsoleMessage>>& error_messages) {
  client_->NotifyFetchFinished(params, error_messages);
}

void ModuleScriptFetcher::SetClient(Client* client) {
  DCHECK(!client_);
  client_ = client;
}

}  // namespace blink
