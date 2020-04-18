// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_QUOTA_MOCK_QUOTA_MANAGER_PROXY_H_
#define CONTENT_BROWSER_QUOTA_MOCK_QUOTA_MANAGER_PROXY_H_

#include <stdint.h>

#include "base/macros.h"
#include "storage/browser/quota/quota_client.h"
#include "storage/browser/quota/quota_manager_proxy.h"
#include "storage/browser/test/mock_quota_manager.h"
#include "third_party/blink/public/mojom/quota/quota_types.mojom.h"
#include "url/gurl.h"

using storage::QuotaManagerProxy;

namespace content {

class MockQuotaManager;

class MockQuotaManagerProxy : public QuotaManagerProxy {
 public:
  // It is ok to give NULL to |quota_manager|.
  MockQuotaManagerProxy(MockQuotaManager* quota_manager,
                        base::SingleThreadTaskRunner* task_runner);

  void RegisterClient(QuotaClient* client) override;

  virtual void SimulateQuotaManagerDestroyed();

  // We don't mock them.
  void NotifyOriginInUse(const url::Origin& origin) override {}
  void NotifyOriginNoLongerInUse(const url::Origin& origin) override {}
  void SetUsageCacheEnabled(QuotaClient::ID client_id,
                            const url::Origin& origin,
                            blink::mojom::StorageType type,
                            bool enabled) override {}
  void GetUsageAndQuota(base::SequencedTaskRunner* original_task_runner,
                        const url::Origin& origin,
                        blink::mojom::StorageType type,
                        QuotaManager::UsageAndQuotaCallback callback) override;

  // Validates the |client_id| and updates the internal access count
  // which can be accessed via notify_storage_accessed_count().
  // The also records the |origin| and |type| in last_notified_origin_ and
  // last_notified_type_.
  void NotifyStorageAccessed(QuotaClient::ID client_id,
                             const url::Origin& origin,
                             blink::mojom::StorageType type) override;

  // Records the |origin|, |type| and |delta| as last_notified_origin_,
  // last_notified_type_ and last_notified_delta_ respecitvely.
  // If non-null MockQuotaManager is given to the constructor this also
  // updates the manager's internal usage information.
  void NotifyStorageModified(QuotaClient::ID client_id,
                             const url::Origin& origin,
                             blink::mojom::StorageType type,
                             int64_t delta) override;

  int notify_storage_accessed_count() const { return storage_accessed_count_; }
  int notify_storage_modified_count() const { return storage_modified_count_; }
  url::Origin last_notified_origin() const { return last_notified_origin_; }
  blink::mojom::StorageType last_notified_type() const {
    return last_notified_type_;
  }
  int64_t last_notified_delta() const { return last_notified_delta_; }

 protected:
  ~MockQuotaManagerProxy() override;

 private:
  MockQuotaManager* mock_manager() const {
    return static_cast<MockQuotaManager*>(quota_manager());
  }

  int storage_accessed_count_;
  int storage_modified_count_;
  url::Origin last_notified_origin_;
  blink::mojom::StorageType last_notified_type_;
  int64_t last_notified_delta_;

  QuotaClient* registered_client_;

  DISALLOW_COPY_AND_ASSIGN(MockQuotaManagerProxy);
};

}  // namespace content

#endif  // CONTENT_BROWSER_QUOTA_MOCK_QUOTA_MANAGER_PROXY_H_
