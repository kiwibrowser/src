// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_EXTENSIONS_UPDATER_EXTENSION_UPDATE_CLIENT_BASE_BROWSERTEST_H_
#define CHROME_BROWSER_EXTENSIONS_UPDATER_EXTENSION_UPDATE_CLIENT_BASE_BROWSERTEST_H_

#include "base/memory/ref_counted.h"
#include "chrome/browser/extensions/extension_browsertest.h"
#include "chrome/browser/extensions/updater/chrome_update_client_config.h"
#include "components/update_client/update_client.h"

namespace base {
namespace test {
class ScopedFeatureList;
}  // namespace test
}  // namespace base

namespace content {
class BrowserMainParts;
}  // namespace content

namespace net {
class TestURLRequestInterceptor;
}  // namespace net

namespace update_client {
class URLRequestPostInterceptor;
class URLRequestPostInterceptorFactory;
}  // namespace update_client

namespace extensions {

class ChromeUpdateClientConfig;
class UpdateService;

// Base class to browser test extension updater using UpdateClient.
class ExtensionUpdateClientBaseTest : public ExtensionBrowserTest {
 public:
  using ConfigFactoryCallback = ChromeUpdateClientConfig::FactoryCallback;

  ExtensionUpdateClientBaseTest();
  ~ExtensionUpdateClientBaseTest() override;

  // ExtensionBrowserTest:
  void SetUp() override;
  void SetUpOnMainThread() override;
  void CreatedBrowserMainParts(content::BrowserMainParts* parts) final;

  // Injects a test configurator to the main extension browser client.
  // Override this function to inject your own custom configurator to the
  // extension browser client (through ConfigFactoryCallback).
  virtual ConfigFactoryCallback ChromeUpdateClientConfigFactory() const;

  // Wait for an update on extension |id| to finish.
  // The return value gives the result of the completed update operation
  // (error, update, no update) as defined in
  // |update_client::UpdateClient::Observer::Events|
  update_client::UpdateClient::Observer::Events
  WaitOnComponentUpdaterCompleteEvent(const std::string& id);

  // Creates network interceptors.
  // Override this function to provide your own network interceptors.
  virtual void SetUpNetworkInterceptors();

  virtual std::vector<GURL> GetUpdateUrls() const;
  virtual std::vector<GURL> GetPingUrls() const;

 protected:
  extensions::UpdateService* update_service_ = nullptr;
  std::unique_ptr<net::TestURLRequestInterceptor> get_interceptor_;
  std::unique_ptr<update_client::URLRequestPostInterceptorFactory>
      update_interceptor_factory_;
  std::unique_ptr<update_client::URLRequestPostInterceptorFactory>
      ping_interceptor_factory_;
  scoped_refptr<update_client::URLRequestPostInterceptor> update_interceptor_;
  scoped_refptr<update_client::URLRequestPostInterceptor> ping_interceptor_;

 private:
  base::test::ScopedFeatureList scoped_feature_list_;

  DISALLOW_COPY_AND_ASSIGN(ExtensionUpdateClientBaseTest);
};

}  // namespace extensions

#endif  // CHROME_BROWSER_EXTENSIONS_UPDATER_EXTENSION_UPDATE_CLIENT_BASE_BROWSERTEST_H_
