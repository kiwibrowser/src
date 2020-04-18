/*
 * Copyright (C) 2013 Google Inc. All rights reserved.
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

#ifndef THIRD_PARTY_BLINK_RENDERER_CORE_WORKERS_WORKER_CONTENT_SETTINGS_CLIENT_H_
#define THIRD_PARTY_BLINK_RENDERER_CORE_WORKERS_WORKER_CONTENT_SETTINGS_CLIENT_H_

#include <memory>
#include "third_party/blink/public/platform/web_content_settings_client.h"
#include "third_party/blink/renderer/core/core_export.h"
#include "third_party/blink/renderer/core/workers/worker_clients.h"
#include "third_party/blink/renderer/platform/wtf/forward.h"

namespace blink {

class ExecutionContext;
class KURL;
class SecurityOrigin;
class WebString;

class CORE_EXPORT WorkerContentSettingsClient final
    : public GarbageCollectedFinalized<WorkerContentSettingsClient>,
      public Supplement<WorkerClients> {
  USING_GARBAGE_COLLECTED_MIXIN(WorkerContentSettingsClient);

 public:
  static const char kSupplementName[];

  static WorkerContentSettingsClient* Create(
      std::unique_ptr<WebContentSettingsClient>);
  virtual ~WorkerContentSettingsClient();

  bool RequestFileSystemAccessSync();
  bool AllowIndexedDB(const WebString& name);
  bool AllowRunningInsecureContent(bool enabled_per_settings,
                                   const SecurityOrigin*,
                                   const KURL&);
  bool AllowScriptFromSource(bool enabled_per_settings, const KURL&);

  static WorkerContentSettingsClient* From(ExecutionContext&);

  WebContentSettingsClient* GetWebContentSettingsClient() {
    return client_.get();
  }

  void Trace(blink::Visitor* visitor) override {
    Supplement<WorkerClients>::Trace(visitor);
  }

 private:
  explicit WorkerContentSettingsClient(
      std::unique_ptr<WebContentSettingsClient>);

  std::unique_ptr<WebContentSettingsClient> client_;
};

void CORE_EXPORT
ProvideContentSettingsClientToWorker(WorkerClients*,
                                     std::unique_ptr<WebContentSettingsClient>);

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_CORE_WORKERS_WORKER_CONTENT_SETTINGS_CLIENT_H_
