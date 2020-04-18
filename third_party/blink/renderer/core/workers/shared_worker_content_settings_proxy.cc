// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/core/workers/shared_worker_content_settings_proxy.h"

#include <memory>
#include <utility>

namespace blink {

SharedWorkerContentSettingsProxy::SharedWorkerContentSettingsProxy(
    mojom::blink::WorkerContentSettingsProxyPtrInfo host_info)
    : host_info_(std::move(host_info)) {}
SharedWorkerContentSettingsProxy::~SharedWorkerContentSettingsProxy() = default;

bool SharedWorkerContentSettingsProxy::AllowIndexedDB(
    const WebString& name,
    const WebSecurityOrigin& origin) {
  bool result = false;
  GetService()->AllowIndexedDB(name, &result);
  return result;
}

bool SharedWorkerContentSettingsProxy::RequestFileSystemAccessSync() {
  bool result = false;
  GetService()->RequestFileSystemAccessSync(&result);
  return result;
}

// Use ThreadSpecific to ensure that |content_settings_instance_host| is
// destructed on worker thread.
// Each worker has a dedicated thread so this is safe.
mojom::blink::WorkerContentSettingsProxyPtr&
SharedWorkerContentSettingsProxy::GetService() {
  DEFINE_THREAD_SAFE_STATIC_LOCAL(
      ThreadSpecific<mojom::blink::WorkerContentSettingsProxyPtr>,
      content_settings_instance_host, ());
  if (!content_settings_instance_host.IsSet()) {
    DCHECK(host_info_.is_valid());
    content_settings_instance_host->Bind(std::move(host_info_));
  }
  return *content_settings_instance_host;
}

}  // namespace blink
