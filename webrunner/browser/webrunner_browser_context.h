// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef WEBRUNNER_BROWSER_WEBRUNNER_BROWSER_CONTEXT_H_
#define WEBRUNNER_BROWSER_WEBRUNNER_BROWSER_CONTEXT_H_

#include "base/macros.h"
#include "content/public/browser/browser_context.h"

namespace webrunner {

class WebRunnerNetLog;
class WebRunnerURLRequestContextGetter;

class WebRunnerBrowserContext : public content::BrowserContext {
 public:
  WebRunnerBrowserContext();
  ~WebRunnerBrowserContext() override;

  // BrowserContext implementation.
  std::unique_ptr<content::ZoomLevelDelegate> CreateZoomLevelDelegate(
      const base::FilePath& partition_path) override;
  base::FilePath GetPath() const override;
  bool IsOffTheRecord() const override;
  content::ResourceContext* GetResourceContext() override;
  content::DownloadManagerDelegate* GetDownloadManagerDelegate() override;
  content::BrowserPluginGuestManager* GetGuestManager() override;
  storage::SpecialStoragePolicy* GetSpecialStoragePolicy() override;
  content::PushMessagingService* GetPushMessagingService() override;
  content::SSLHostStateDelegate* GetSSLHostStateDelegate() override;
  content::PermissionManager* GetPermissionManager() override;
  content::BackgroundFetchDelegate* GetBackgroundFetchDelegate() override;
  content::BackgroundSyncController* GetBackgroundSyncController() override;
  content::BrowsingDataRemoverDelegate* GetBrowsingDataRemoverDelegate()
      override;
  net::URLRequestContextGetter* CreateRequestContext(
      content::ProtocolHandlerMap* protocol_handlers,
      content::URLRequestInterceptorScopedVector request_interceptors) override;
  net::URLRequestContextGetter* CreateRequestContextForStoragePartition(
      const base::FilePath& partition_path,
      bool in_memory,
      content::ProtocolHandlerMap* protocol_handlers,
      content::URLRequestInterceptorScopedVector request_interceptors) override;
  net::URLRequestContextGetter* CreateMediaRequestContext() override;
  net::URLRequestContextGetter* CreateMediaRequestContextForStoragePartition(
      const base::FilePath& partition_path,
      bool in_memory) override;

 private:
  // Contains URLRequestContextGetter required for resource loading.
  class ResourceContext;

  std::unique_ptr<WebRunnerNetLog> net_log_;
  scoped_refptr<WebRunnerURLRequestContextGetter> url_request_getter_;
  std::unique_ptr<ResourceContext> resource_context_;

  DISALLOW_COPY_AND_ASSIGN(WebRunnerBrowserContext);
};

}  // namespace webrunner

#endif  // WEBRUNNER_BROWSER_WEBRUNNER_BROWSER_CONTEXT_H_
