// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chromecast/browser/cast_browser_context.h"

#include <memory>

#include "base/command_line.h"
#include "base/files/file_util.h"
#include "base/macros.h"
#include "base/path_service.h"
#include "build/build_config.h"
#include "chromecast/base/cast_paths.h"
#include "chromecast/browser/cast_download_manager_delegate.h"
#include "chromecast/browser/cast_permission_manager.h"
#include "chromecast/browser/url_request_context_factory.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/resource_context.h"
#include "content/public/browser/storage_partition.h"
#include "content/public/common/content_switches.h"
#include "net/url_request/url_request_context.h"
#include "net/url_request/url_request_context_getter.h"

namespace chromecast {
namespace shell {

namespace {
const void* const kDownloadManagerDelegateKey = &kDownloadManagerDelegateKey;
}

class CastBrowserContext::CastResourceContext :
    public content::ResourceContext {
 public:
  explicit CastResourceContext(
      URLRequestContextFactory* url_request_context_factory) :
    url_request_context_factory_(url_request_context_factory) {}
  ~CastResourceContext() override {}

  // ResourceContext implementation:
  net::HostResolver* GetHostResolver() override {
    return url_request_context_factory_->GetMainGetter()->
        GetURLRequestContext()->host_resolver();
  }

  net::URLRequestContext* GetRequestContext() override {
    return url_request_context_factory_->GetMainGetter()->
        GetURLRequestContext();
  }

 private:
  URLRequestContextFactory* url_request_context_factory_;

  DISALLOW_COPY_AND_ASSIGN(CastResourceContext);
};

CastBrowserContext::CastBrowserContext(
    URLRequestContextFactory* url_request_context_factory)
    : url_request_context_factory_(url_request_context_factory),
      resource_context_(new CastResourceContext(url_request_context_factory)) {
  InitWhileIOAllowed();
}

CastBrowserContext::~CastBrowserContext() {
  BrowserContext::NotifyWillBeDestroyed(this);
  ShutdownStoragePartitions();
  content::BrowserThread::DeleteSoon(
      content::BrowserThread::IO,
      FROM_HERE,
      resource_context_.release());
}

void CastBrowserContext::InitWhileIOAllowed() {
#if defined(OS_ANDROID)
  CHECK(base::PathService::Get(base::DIR_ANDROID_APP_DATA, &path_));
  path_ = path_.Append(FILE_PATH_LITERAL("cast_shell"));

  if (!base::PathExists(path_))
    base::CreateDirectory(path_);
#else
  // Chromecast doesn't support user profiles nor does it have
  // incognito mode.  This means that all of the persistent
  // data (currently only cookies and local storage) will be
  // shared in a single location as defined here.
  CHECK(base::PathService::Get(DIR_CAST_HOME, &path_));
#endif  // defined(OS_ANDROID)
  BrowserContext::Initialize(this, path_);
}

#if !defined(OS_ANDROID)
std::unique_ptr<content::ZoomLevelDelegate>
CastBrowserContext::CreateZoomLevelDelegate(
    const base::FilePath& partition_path) {
  return nullptr;
}
#endif  // !defined(OS_ANDROID)

base::FilePath CastBrowserContext::GetPath() const {
  return path_;
}

bool CastBrowserContext::IsOffTheRecord() const {
  return false;
}

net::URLRequestContextGetter* CastBrowserContext::GetSystemRequestContext() {
  return url_request_context_factory_->GetSystemGetter();
}

content::ResourceContext* CastBrowserContext::GetResourceContext() {
  return resource_context_.get();
}

content::DownloadManagerDelegate*
CastBrowserContext::GetDownloadManagerDelegate() {
  if (!GetUserData(kDownloadManagerDelegateKey)) {
    SetUserData(kDownloadManagerDelegateKey,
                std::make_unique<CastDownloadManagerDelegate>());
  }
  return static_cast<CastDownloadManagerDelegate*>(
      GetUserData(kDownloadManagerDelegateKey));
}

content::BrowserPluginGuestManager* CastBrowserContext::GetGuestManager() {
  return nullptr;
}

storage::SpecialStoragePolicy* CastBrowserContext::GetSpecialStoragePolicy() {
  return nullptr;
}

content::PushMessagingService* CastBrowserContext::GetPushMessagingService() {
  return nullptr;
}

content::SSLHostStateDelegate* CastBrowserContext::GetSSLHostStateDelegate() {
  return nullptr;
}

content::PermissionManager* CastBrowserContext::GetPermissionManager() {
  if (!permission_manager_.get())
    permission_manager_.reset(new CastPermissionManager());
  return permission_manager_.get();
}

content::BackgroundFetchDelegate*
CastBrowserContext::GetBackgroundFetchDelegate() {
  return nullptr;
}

content::BackgroundSyncController*
CastBrowserContext::GetBackgroundSyncController() {
  return nullptr;
}

content::BrowsingDataRemoverDelegate*
CastBrowserContext::GetBrowsingDataRemoverDelegate() {
  return nullptr;
}

net::URLRequestContextGetter* CastBrowserContext::CreateRequestContext(
    content::ProtocolHandlerMap* protocol_handlers,
    content::URLRequestInterceptorScopedVector request_interceptors) {
  return url_request_context_factory_->CreateMainGetter(
      this, protocol_handlers, std::move(request_interceptors));
}

net::URLRequestContextGetter*
CastBrowserContext::CreateRequestContextForStoragePartition(
    const base::FilePath& partition_path,
    bool in_memory,
    content::ProtocolHandlerMap* protocol_handlers,
    content::URLRequestInterceptorScopedVector request_interceptors) {
  return nullptr;
}

net::URLRequestContextGetter* CastBrowserContext::CreateMediaRequestContext() {
  return url_request_context_factory_->GetMediaGetter();
}

net::URLRequestContextGetter*
CastBrowserContext::CreateMediaRequestContextForStoragePartition(
    const base::FilePath& partition_path, bool in_memory) {
  return nullptr;
}

}  // namespace shell
}  // namespace chromecast
