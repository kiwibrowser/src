// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "android_webview/browser/net/aw_url_request_context_getter.h"

#include <memory>

#include "base/android/jni_android.h"
#include "base/files/scoped_temp_dir.h"
#include "base/memory/ref_counted.h"
#include "components/prefs/testing_pref_service.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/common/url_constants.h"
#include "content/public/test/test_browser_thread_bundle.h"
#include "net/base/net_errors.h"
#include "net/log/net_log.h"
#include "net/proxy_resolution/proxy_config_service.h"
#include "net/proxy_resolution/proxy_resolution_service.h"
#include "net/ssl/ssl_config.h"
#include "net/ssl/ssl_config_service.h"
#include "net/test/url_request/url_request_failed_job.h"
#include "net/url_request/url_request_context.h"
#include "net/url_request/url_request_job_factory.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace android_webview {

namespace {

// A ProtocolHandler that will immediately fail all jobs.
class FailingProtocolHandler
    : public net::URLRequestJobFactory::ProtocolHandler {
 public:
  net::URLRequestJob* MaybeCreateJob(
      net::URLRequest* request,
      net::NetworkDelegate* network_delegate) const override {
    return new net::URLRequestFailedJob(request, network_delegate,
                                        net::URLRequestFailedJob::START,
                                        net::ERR_FAILED);
  }
};

}  // namespace

class AwURLRequestContextGetterTest : public ::testing::Test {
 public:
  AwURLRequestContextGetterTest() = default;

 protected:
  void SetUp() override {
    ASSERT_TRUE(temp_dir_.CreateUniqueTempDir());
    env_ = base::android::AttachCurrentThread();
    ASSERT_TRUE(env_);

    pref_service_ = std::make_unique<TestingPrefServiceSimple>();
    android_webview::AwURLRequestContextGetter::RegisterPrefs(
        pref_service_->registry());

    getter_ = base::MakeRefCounted<android_webview::AwURLRequestContextGetter>(
        temp_dir_.GetPath(), temp_dir_.GetPath().AppendASCII("ChannelID"),
        net::ProxyResolutionService::CreateSystemProxyConfigService(
            content::BrowserThread::GetTaskRunnerForThread(
                content::BrowserThread::IO)),
        pref_service_.get(), &net_log_);

    // AwURLRequestContextGetter implicitly depends on having protocol handlers
    // provided for url::kBlobScheme, url::kFileSystemScheme, and
    // content::kChromeUIScheme, so provide testing values here.
    content::ProtocolHandlerMap fake_handlers;
    fake_handlers[url::kBlobScheme].reset(new FailingProtocolHandler());
    fake_handlers[url::kFileSystemScheme].reset(new FailingProtocolHandler());
    fake_handlers[content::kChromeUIScheme].reset(new FailingProtocolHandler());
    content::URLRequestInterceptorScopedVector interceptors;
    getter_->SetHandlersAndInterceptors(&fake_handlers,
                                        std::move(interceptors));
  }

  content::TestBrowserThreadBundle thread_bundle_;
  base::ScopedTempDir temp_dir_;
  JNIEnv* env_;
  net::NetLog net_log_;
  std::unique_ptr<TestingPrefServiceSimple> pref_service_;
  scoped_refptr<android_webview::AwURLRequestContextGetter> getter_;
};

// Tests that constraints on trust for Symantec-issued certificates are not
// enforced for the AwURLRequestContext(Getter), as it should behave like
// the Android system.
TEST_F(AwURLRequestContextGetterTest, SymantecPoliciesExempted) {
  net::URLRequestContext* context = getter_->GetURLRequestContext();
  ASSERT_TRUE(context);
  net::SSLConfigService* config_service = context->ssl_config_service();
  ASSERT_TRUE(config_service);

  net::SSLConfig config;
  EXPECT_FALSE(config.symantec_enforcement_disabled);
  config_service->GetSSLConfig(&config);
  EXPECT_TRUE(config.symantec_enforcement_disabled);
}

// Tests that SHA-1 is still allowed for locally-installed trust anchors,
// including those in application manifests, as it should behave like
// the Android system.
TEST_F(AwURLRequestContextGetterTest, SHA1LocalAnchorsAllowed) {
  net::URLRequestContext* context = getter_->GetURLRequestContext();
  ASSERT_TRUE(context);
  net::SSLConfigService* config_service = context->ssl_config_service();
  ASSERT_TRUE(config_service);

  net::SSLConfig config;
  EXPECT_FALSE(config.sha1_local_anchors_enabled);
  config_service->GetSSLConfig(&config);
  EXPECT_TRUE(config.sha1_local_anchors_enabled);
}

}  // namespace android_webview
