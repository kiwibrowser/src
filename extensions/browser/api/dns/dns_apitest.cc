// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <memory>

#include "base/memory/ref_counted.h"
#include "base/values.h"
#include "content/public/browser/notification_service.h"
#include "content/public/test/test_utils.h"
#include "extensions/browser/api/dns/dns_api.h"
#include "extensions/browser/api/dns/host_resolver_wrapper.h"
#include "extensions/browser/api/dns/mock_host_resolver_creator.h"
#include "extensions/browser/api_test_utils.h"
#include "extensions/browser/notification_types.h"
#include "extensions/common/extension.h"
#include "extensions/common/extension_builder.h"
#include "extensions/shell/test/shell_apitest.h"
#include "net/base/net_errors.h"

using extensions::api_test_utils::RunFunctionAndReturnSingleResult;

namespace extensions {

class DnsApiTest : public ShellApiTest {
 public:
  DnsApiTest() : resolver_creator_(new MockHostResolverCreator()) {}

 private:
  void SetUpOnMainThread() override {
    ShellApiTest::SetUpOnMainThread();
    HostResolverWrapper::GetInstance()->SetHostResolverForTesting(
        resolver_creator_->CreateMockHostResolver());
  }

  void TearDownOnMainThread() override {
    HostResolverWrapper::GetInstance()->SetHostResolverForTesting(NULL);
    resolver_creator_->DeleteMockHostResolver();
    ShellApiTest::TearDownOnMainThread();
  }

  // The MockHostResolver asserts that it's used on the same thread on which
  // it's created, which is actually a stronger rule than its real counterpart.
  // But that's fine; it's good practice.
  scoped_refptr<MockHostResolverCreator> resolver_creator_;
};

IN_PROC_BROWSER_TEST_F(DnsApiTest, DnsResolveIPLiteral) {
  scoped_refptr<DnsResolveFunction> resolve_function(new DnsResolveFunction());
  scoped_refptr<Extension> empty_extension = ExtensionBuilder("Test").Build();

  resolve_function->set_extension(empty_extension.get());
  resolve_function->set_has_callback(true);

  std::unique_ptr<base::Value> result(RunFunctionAndReturnSingleResult(
      resolve_function.get(), "[\"127.0.0.1\"]", browser_context()));
  base::DictionaryValue* dict = NULL;
  ASSERT_TRUE(result->GetAsDictionary(&dict));

  int result_code = 0;
  EXPECT_TRUE(dict->GetInteger("resultCode", &result_code));
  EXPECT_EQ(net::OK, result_code);

  std::string address;
  EXPECT_TRUE(dict->GetString("address", &address));
  EXPECT_EQ("127.0.0.1", address);
}

IN_PROC_BROWSER_TEST_F(DnsApiTest, DnsResolveHostname) {
  scoped_refptr<DnsResolveFunction> resolve_function(new DnsResolveFunction());
  scoped_refptr<Extension> empty_extension = ExtensionBuilder("Test").Build();

  resolve_function->set_extension(empty_extension.get());
  resolve_function->set_has_callback(true);

  std::string function_arguments("[\"");
  function_arguments += MockHostResolverCreator::kHostname;
  function_arguments += "\"]";
  std::unique_ptr<base::Value> result(RunFunctionAndReturnSingleResult(
      resolve_function.get(), function_arguments, browser_context()));
  base::DictionaryValue* dict = NULL;
  ASSERT_TRUE(result->GetAsDictionary(&dict));

  int result_code = 0;
  EXPECT_TRUE(dict->GetInteger("resultCode", &result_code));
  EXPECT_EQ(net::OK, result_code);

  std::string address;
  EXPECT_TRUE(dict->GetString("address", &address));
  EXPECT_EQ(MockHostResolverCreator::kAddress, address);
}

IN_PROC_BROWSER_TEST_F(DnsApiTest, DnsExtension) {
  ASSERT_TRUE(RunAppTest("api_test/dns/api")) << message_;
}

}  // namespace extensions
