// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/public/test/test_browser_context.h"

#include <utility>

#include "base/files/file_path.h"
#include "base/logging.h"
#include "base/single_thread_task_runner.h"
#include "base/test/null_task_runner.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/permission_controller_delegate.h"
#include "content/public/test/mock_resource_context.h"
#include "content/test/mock_background_sync_controller.h"
#include "content/test/mock_ssl_host_state_delegate.h"
#include "net/url_request/url_request_context.h"
#include "net/url_request/url_request_context_getter.h"
#include "net/url_request/url_request_test_util.h"
#include "storage/browser/quota/special_storage_policy.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace {

class TestContextURLRequestContextGetter : public net::URLRequestContextGetter {
 public:
  TestContextURLRequestContextGetter()
      : null_task_runner_(new base::NullTaskRunner) {
  }

  net::URLRequestContext* GetURLRequestContext() override { return &context_; }

  scoped_refptr<base::SingleThreadTaskRunner> GetNetworkTaskRunner()
      const override {
    return null_task_runner_;
  }

 private:
  ~TestContextURLRequestContextGetter() override {}

  net::TestURLRequestContext context_;
  scoped_refptr<base::SingleThreadTaskRunner> null_task_runner_;
};

}  // namespace

namespace content {

TestBrowserContext::TestBrowserContext(
    base::FilePath browser_context_dir_path) {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::UI))
      << "Please construct content::TestBrowserTheadBundle before constructing "
      << "TestBrowserContext instances.  "
      << BrowserThread::GetDCheckCurrentlyOnErrorMessage(BrowserThread::UI);

  if (browser_context_dir_path.empty()) {
    EXPECT_TRUE(browser_context_dir_.CreateUniqueTempDir());
  } else {
    EXPECT_TRUE(browser_context_dir_.Set(browser_context_dir_path));
  }
  BrowserContext::Initialize(this, browser_context_dir_.GetPath());
}

TestBrowserContext::~TestBrowserContext() {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::UI))
      << "Please destruct content::TestBrowserContext before destructing "
      << "the TestBrowserThreadBundle instance.  "
      << BrowserThread::GetDCheckCurrentlyOnErrorMessage(BrowserThread::UI);

  NotifyWillBeDestroyed(this);
  ShutdownStoragePartitions();
}

base::FilePath TestBrowserContext::TakePath() {
  return browser_context_dir_.Take();
}

void TestBrowserContext::SetSpecialStoragePolicy(
    storage::SpecialStoragePolicy* policy) {
  special_storage_policy_ = policy;
}

void TestBrowserContext::SetPermissionControllerDelegate(
    std::unique_ptr<PermissionControllerDelegate> delegate) {
  permission_controller_delegate_ = std::move(delegate);
}

net::URLRequestContextGetter* TestBrowserContext::GetRequestContext() {
  if (!request_context_.get()) {
    request_context_ = new TestContextURLRequestContextGetter();
  }
  return request_context_.get();
}

base::FilePath TestBrowserContext::GetPath() const {
  return browser_context_dir_.GetPath();
}

#if !defined(OS_ANDROID)
std::unique_ptr<ZoomLevelDelegate> TestBrowserContext::CreateZoomLevelDelegate(
    const base::FilePath& partition_path) {
  return std::unique_ptr<ZoomLevelDelegate>();
}
#endif  // !defined(OS_ANDROID)

bool TestBrowserContext::IsOffTheRecord() const {
  return is_off_the_record_;
}

DownloadManagerDelegate* TestBrowserContext::GetDownloadManagerDelegate() {
  return nullptr;
}

ResourceContext* TestBrowserContext::GetResourceContext() {
  if (!resource_context_)
    resource_context_.reset(new MockResourceContext(
        GetRequestContext()->GetURLRequestContext()));
  return resource_context_.get();
}

BrowserPluginGuestManager* TestBrowserContext::GetGuestManager() {
  return nullptr;
}

storage::SpecialStoragePolicy* TestBrowserContext::GetSpecialStoragePolicy() {
  return special_storage_policy_.get();
}

PushMessagingService* TestBrowserContext::GetPushMessagingService() {
  return nullptr;
}

SSLHostStateDelegate* TestBrowserContext::GetSSLHostStateDelegate() {
  if (!ssl_host_state_delegate_)
    ssl_host_state_delegate_.reset(new MockSSLHostStateDelegate());
  return ssl_host_state_delegate_.get();
}

PermissionControllerDelegate*
TestBrowserContext::GetPermissionControllerDelegate() {
  return permission_controller_delegate_.get();
}

BackgroundFetchDelegate* TestBrowserContext::GetBackgroundFetchDelegate() {
  return nullptr;
}

BackgroundSyncController* TestBrowserContext::GetBackgroundSyncController() {
  if (!background_sync_controller_)
    background_sync_controller_.reset(new MockBackgroundSyncController());

  return background_sync_controller_.get();
}

BrowsingDataRemoverDelegate*
TestBrowserContext::GetBrowsingDataRemoverDelegate() {
  // Most BrowsingDataRemover tests do not require a delegate
  // (not even a mock one).
  return nullptr;
}

net::URLRequestContextGetter* TestBrowserContext::CreateRequestContext(
      content::ProtocolHandlerMap* protocol_handlers,
      content::URLRequestInterceptorScopedVector request_interceptors) {
  request_interceptors_ = std::move(request_interceptors);
  return GetRequestContext();
}

net::URLRequestContextGetter*
TestBrowserContext::CreateRequestContextForStoragePartition(
    const base::FilePath& partition_path,
    bool in_memory,
    ProtocolHandlerMap* protocol_handlers,
    URLRequestInterceptorScopedVector request_interceptors) {
  request_interceptors_ = std::move(request_interceptors);
  // Simply returns the same RequestContext since no tests is relying on the
  // expected behavior.
  return GetRequestContext();
}

net::URLRequestContextGetter* TestBrowserContext::CreateMediaRequestContext() {
  return nullptr;
}

net::URLRequestContextGetter*
TestBrowserContext::CreateMediaRequestContextForStoragePartition(
    const base::FilePath& partition_path,
    bool in_memory) {
  return nullptr;
}

}  // namespace content
