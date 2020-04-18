// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_APPCACHE_APPCACHE_NAVIGATION_HANDLE_CORE_H_
#define CONTENT_BROWSER_APPCACHE_APPCACHE_NAVIGATION_HANDLE_CORE_H_

#include <memory>

#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "base/memory/weak_ptr.h"
#include "content/common/appcache_interfaces.h"

namespace content {

class AppCacheNavigationHandle;
class AppCacheHost;
class AppCacheServiceImpl;
class ChromeAppCacheService;

// PlzNavigate
// This class is used to manage the lifetime of AppCacheHosts
// created during navigations. This class is created on the UI thread, but
// should only be accessed from the IO thread afterwards. It is the IO thread
// pendant of AppCacheNavigationHandle. See the
// AppCacheNavigationHandle header for more details about the lifetime of
// both classes.
class AppCacheNavigationHandleCore : public AppCacheFrontend {
 public:
  AppCacheNavigationHandleCore(
      base::WeakPtr<AppCacheNavigationHandle> ui_handle,
      ChromeAppCacheService* appcache_service,
      int appcache_host_id);
  ~AppCacheNavigationHandleCore() override;

  // Returns the raw AppCacheHost pointer. Ownership remains with this class.
  AppCacheHost* host() { return precreated_host_.get(); }

  // Initializes this instance. Should be called on the IO thread.
  void Initialize();

  // Returns the precreated AppCacheHost pointer. Ownership of the host is
  // released here.
  static std::unique_ptr<AppCacheHost> GetPrecreatedHost(int host_id);

  AppCacheServiceImpl* GetAppCacheService();

 protected:
  // AppCacheFrontend methods
  // We don't expect calls on the AppCacheFrontend methods while the
  // AppCacheHost is not registered with the AppCacheBackend.
  void OnCacheSelected(int host_id, const AppCacheInfo& info) override;
  void OnStatusChanged(const std::vector<int>& host_ids,
                       AppCacheStatus status) override;
  void OnEventRaised(const std::vector<int>& host_ids,
                     AppCacheEventID event_id) override;
  void OnProgressEventRaised(const std::vector<int>& host_ids,
                             const GURL& url,
                             int num_total,
                             int num_complete) override;
  void OnErrorEventRaised(const std::vector<int>& host_ids,
                          const AppCacheErrorDetails& details) override;
  void OnLogMessage(int host_id,
                    AppCacheLogLevel log_level,
                    const std::string& message) override;
  void OnContentBlocked(int host_id, const GURL& manifest_url) override;
  void OnSetSubresourceFactory(
      int host_id,
      network::mojom::URLLoaderFactoryPtr url_loader_factory) override;

 private:
  std::unique_ptr<AppCacheHost> precreated_host_;
  scoped_refptr<ChromeAppCacheService> appcache_service_;
  int appcache_host_id_;
  base::WeakPtr<AppCacheNavigationHandle> ui_handle_;

  DISALLOW_COPY_AND_ASSIGN(AppCacheNavigationHandleCore);
};

}  // namespace content

#endif  // CONTENT_BROWSER_APPCACHE_APPCACHE_NAVIGATION_HANDLE_CORE_H_
