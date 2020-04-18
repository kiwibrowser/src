// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_SERVICE_WORKER_SERVICE_WORKER_CONTENT_SETTINGS_PROXY_IMPL_H_
#define CONTENT_BROWSER_SERVICE_WORKER_SERVICE_WORKER_CONTENT_SETTINGS_PROXY_IMPL_H_

#include "content/browser/service_worker/service_worker_context_core.h"
#include "content/browser/service_worker/service_worker_context_wrapper.h"
#include "content/common/content_export.h"
#include "mojo/public/cpp/bindings/binding.h"
#include "third_party/blink/public/web/worker_content_settings_proxy.mojom.h"
#include "url/origin.h"

namespace content {

class ServiceWorkerContextCore;

// ServiceWorkerContentSettingsProxyImpl passes content settings to its renderer
// counterpart blink::ServiceWorkerContentSettingsProxy
// Created on EmbeddedWorkerInstance::SendStartWorker() and connects to the
// counterpart at the moment.
// EmbeddedWorkerInstance owns this class, so the lifetime of this class is
// strongly associated to it.
class ServiceWorkerContentSettingsProxyImpl final
    : public blink::mojom::WorkerContentSettingsProxy {
 public:
  ServiceWorkerContentSettingsProxyImpl(
      const GURL& script_url,
      base::WeakPtr<ServiceWorkerContextCore> context,
      blink::mojom::WorkerContentSettingsProxyRequest request);

  ~ServiceWorkerContentSettingsProxyImpl() override;

  // blink::mojom::WorkerContentSettingsProxy implementation
  void AllowIndexedDB(const base::string16& name,
                      AllowIndexedDBCallback callback) override;
  void RequestFileSystemAccessSync(
      RequestFileSystemAccessSyncCallback callback) override;

 private:

  const url::Origin origin_;
  base::WeakPtr<ServiceWorkerContextCore> context_;
  mojo::Binding<blink::mojom::WorkerContentSettingsProxy> binding_;
};

}  // namespace content

#endif  // CONTENT_BROWSER_SERVICE_WORKER_SERVICE_WORKER_CONTENT_SETTINGS_PROXY_IMPL_H_
