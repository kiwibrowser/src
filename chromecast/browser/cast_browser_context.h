// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROMECAST_BROWSER_CAST_BROWSER_CONTEXT_H_
#define CHROMECAST_BROWSER_CAST_BROWSER_CONTEXT_H_

#include "base/files/file_path.h"
#include "base/macros.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/content_browser_client.h"

namespace chromecast {
namespace shell {

class URLRequestContextFactory;

// Chromecast does not currently support multiple profiles.  So there is a
// single BrowserContext for all chromecast renderers.
// There is no support for PartitionStorage.
class CastBrowserContext final : public content::BrowserContext {
 public:
  explicit CastBrowserContext(
      URLRequestContextFactory* url_request_context_factory);
  ~CastBrowserContext() override;

  // BrowserContext implementation:
#if !defined(OS_ANDROID)
  std::unique_ptr<content::ZoomLevelDelegate> CreateZoomLevelDelegate(
      const base::FilePath& partition_path) override;
#endif  // !defined(OS_ANDROID)
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

  net::URLRequestContextGetter* GetSystemRequestContext();

 private:
  class CastResourceContext;

  // Performs initialization of the CastBrowserContext while IO is still
  // allowed on the current thread.
  void InitWhileIOAllowed();

  URLRequestContextFactory* const url_request_context_factory_;
  base::FilePath path_;
  std::unique_ptr<CastResourceContext> resource_context_;
  std::unique_ptr<content::PermissionManager> permission_manager_;

  DISALLOW_COPY_AND_ASSIGN(CastBrowserContext);
};

}  // namespace shell
}  // namespace chromecast

#endif  // CHROMECAST_BROWSER_CAST_BROWSER_CONTEXT_H_
