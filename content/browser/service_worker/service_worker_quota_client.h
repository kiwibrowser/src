// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_SERVICE_WORKER_SERVICE_WORKER_QUOTA_CLIENT_H_
#define CONTENT_BROWSER_SERVICE_WORKER_SERVICE_WORKER_QUOTA_CLIENT_H_

#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "content/common/content_export.h"
#include "storage/browser/quota/quota_client.h"
#include "third_party/blink/public/mojom/quota/quota_types.mojom.h"
#include "url/origin.h"

namespace content {
class ServiceWorkerContextWrapper;

class ServiceWorkerQuotaClient : public storage::QuotaClient {
 public:
  ~ServiceWorkerQuotaClient() override;

  // QuotaClient method overrides
  ID id() const override;
  void OnQuotaManagerDestroyed() override;
  void GetOriginUsage(const url::Origin& origin,
                      blink::mojom::StorageType type,
                      GetUsageCallback callback) override;
  void GetOriginsForType(blink::mojom::StorageType type,
                         GetOriginsCallback callback) override;
  void GetOriginsForHost(blink::mojom::StorageType type,
                         const std::string& host,
                         GetOriginsCallback callback) override;
  void DeleteOriginData(const url::Origin& origin,
                        blink::mojom::StorageType type,
                        DeletionCallback callback) override;
  bool DoesSupport(blink::mojom::StorageType type) const override;

 private:
  friend class ServiceWorkerContextWrapper;
  friend class ServiceWorkerQuotaClientTest;

  CONTENT_EXPORT explicit ServiceWorkerQuotaClient(
      ServiceWorkerContextWrapper* context);

  scoped_refptr<ServiceWorkerContextWrapper> context_;

  DISALLOW_COPY_AND_ASSIGN(ServiceWorkerQuotaClient);
};

}  // namespace content

#endif  // CONTENT_BROWSER_SERVICE_WORKER_SERVICE_WORKER_QUOTA_CLIENT_H_
