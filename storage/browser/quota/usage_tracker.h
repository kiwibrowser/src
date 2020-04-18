// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef STORAGE_BROWSER_QUOTA_USAGE_TRACKER_H_
#define STORAGE_BROWSER_QUOTA_USAGE_TRACKER_H_

#include <stdint.h>

#include <map>
#include <memory>
#include <set>
#include <string>

#include "base/callback.h"
#include "base/containers/flat_map.h"
#include "base/macros.h"
#include "storage/browser/quota/quota_callbacks.h"
#include "storage/browser/quota/quota_client.h"
#include "storage/browser/quota/quota_task.h"
#include "storage/browser/quota/special_storage_policy.h"
#include "storage/browser/storage_browser_export.h"
#include "third_party/blink/public/mojom/quota/quota_types.mojom.h"
#include "url/gurl.h"

namespace storage {

class ClientUsageTracker;
class StorageMonitor;

// A helper class that gathers and tracks the amount of data stored in
// all quota clients.
// An instance of this class is created per storage type.
class STORAGE_EXPORT UsageTracker : public QuotaTaskObserver {
 public:
  UsageTracker(const QuotaClientList& clients,
               blink::mojom::StorageType type,
               SpecialStoragePolicy* special_storage_policy,
               StorageMonitor* storage_monitor);
  ~UsageTracker() override;

  blink::mojom::StorageType type() const { return type_; }
  ClientUsageTracker* GetClientTracker(QuotaClient::ID client_id);

  void GetGlobalLimitedUsage(UsageCallback callback);
  void GetGlobalUsage(GlobalUsageCallback callback);
  void GetHostUsage(const std::string& host, UsageCallback callback);
  void GetHostUsageWithBreakdown(const std::string& host,
                                 UsageWithBreakdownCallback callback);
  void UpdateUsageCache(QuotaClient::ID client_id,
                        const GURL& origin,
                        int64_t delta);
  int64_t GetCachedUsage() const;
  void GetCachedHostsUsage(std::map<std::string, int64_t>* host_usage) const;
  void GetCachedOriginsUsage(std::map<GURL, int64_t>* origin_usage) const;
  void GetCachedOrigins(std::set<GURL>* origins) const;
  bool IsWorking() const {
    return global_usage_callbacks_.HasCallbacks() ||
           host_usage_callbacks_.HasAnyCallbacks();
  }

  void SetUsageCacheEnabled(QuotaClient::ID client_id,
                            const GURL& origin,
                            bool enabled);

 private:
  struct AccumulateInfo {
    AccumulateInfo();
    ~AccumulateInfo();
    int pending_clients = 0;
    int64_t usage = 0;
    int64_t unlimited_usage = 0;
    base::flat_map<QuotaClient::ID, int64_t> usage_breakdown;
  };

  using UsageCallbackQueue = CallbackQueue<UsageCallback, int64_t>;
  using GlobalUsageCallbackQueue =
      CallbackQueue<GlobalUsageCallback, int64_t, int64_t>;
  using HostUsageCallbackMap =
      CallbackQueueMap<UsageWithBreakdownCallback,
                       std::string,
                       int64_t,
                       base::flat_map<QuotaClient::ID, int64_t>>;

  friend class ClientUsageTracker;
  void AccumulateClientGlobalLimitedUsage(AccumulateInfo* info,
                                          int64_t limited_usage);
  void AccumulateClientGlobalUsage(AccumulateInfo* info,
                                   int64_t usage,
                                   int64_t unlimited_usage);
  void AccumulateClientHostUsage(const base::Closure& barrier,
                                 AccumulateInfo* info,
                                 const std::string& host,
                                 QuotaClient::ID client,
                                 int64_t usage);
  void FinallySendHostUsageWithBreakdown(AccumulateInfo* info,
                                         const std::string& host);

  const blink::mojom::StorageType type_;
  std::map<QuotaClient::ID, std::unique_ptr<ClientUsageTracker>>
      client_tracker_map_;

  UsageCallbackQueue global_limited_usage_callbacks_;
  GlobalUsageCallbackQueue global_usage_callbacks_;
  HostUsageCallbackMap host_usage_callbacks_;

  StorageMonitor* storage_monitor_;

  base::WeakPtrFactory<UsageTracker> weak_factory_;
  DISALLOW_COPY_AND_ASSIGN(UsageTracker);
};

}  // namespace storage

#endif  // STORAGE_BROWSER_QUOTA_USAGE_TRACKER_H_
