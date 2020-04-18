// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_SHARED_WORKER_SHARED_WORKER_CONTENT_SETTING_PROXY_IMPL_H_
#define CONTENT_BROWSER_SHARED_WORKER_SHARED_WORKER_CONTENT_SETTING_PROXY_IMPL_H_

#include "base/callback.h"
#include "base/strings/string16.h"
#include "content/common/content_export.h"
#include "content/public/browser/resource_context.h"
#include "mojo/public/cpp/bindings/binding.h"
#include "third_party/blink/public/web/worker_content_settings_proxy.mojom.h"
#include "url/origin.h"

namespace content {

class SharedWorkerHost;

// SharedWorkerContentSettingsProxyImpl passes content settings to its renderer
// counterpart blink::SharedWorkerContentSettingsProxy.
// Created on SharedWorker::Start() and connects to the counterpart
// at the moment.
// SharedWorkerHost owns this class, so the lifetime of this class is strongly
// associated to it.
class CONTENT_EXPORT SharedWorkerContentSettingsProxyImpl
    : public blink::mojom::WorkerContentSettingsProxy {
 public:
  SharedWorkerContentSettingsProxyImpl(
      const GURL& script_url,
      SharedWorkerHost* owner,
      blink::mojom::WorkerContentSettingsProxyRequest request);

  ~SharedWorkerContentSettingsProxyImpl() override;

  // blink::mojom::WorkerContentSettingsProxy implementation.
  void AllowIndexedDB(const base::string16& name,
                      AllowIndexedDBCallback callback) override;
  void RequestFileSystemAccessSync(
      RequestFileSystemAccessSyncCallback callback) override;

 private:
  const url::Origin origin_;
  SharedWorkerHost* owner_;
  mojo::Binding<blink::mojom::WorkerContentSettingsProxy> binding_;

  DISALLOW_COPY_AND_ASSIGN(SharedWorkerContentSettingsProxyImpl);
};

}  // namespace content

#endif  // CONTENT_BROWSER_SHARED_WORKER_SHARED_WORKER_CONTENT_SETTING_PROXY_IMPL_H_
