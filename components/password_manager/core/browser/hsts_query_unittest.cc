// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/password_manager/core/browser/hsts_query.h"

#include <string>

#include "base/bind.h"
#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "base/message_loop/message_loop.h"
#include "base/run_loop.h"
#include "base/threading/thread_task_runner_handle.h"
#include "components/password_manager/core/browser/password_manager_test_utils.h"
#include "net/url_request/url_request_context.h"
#include "net/url_request/url_request_test_util.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

namespace password_manager {
namespace {

// Auxiliary class to automatically set and reset the HSTS state for a given
// host.
class HSTSStateManager {
 public:
  HSTSStateManager(net::TransportSecurityState* state,
                   bool is_hsts,
                   std::string host);
  ~HSTSStateManager();

 private:
  net::TransportSecurityState* state_;
  const bool is_hsts_;
  const std::string host_;

  DISALLOW_COPY_AND_ASSIGN(HSTSStateManager);
};

HSTSStateManager::HSTSStateManager(net::TransportSecurityState* state,
                                   bool is_hsts,
                                   std::string host)
    : state_(state), is_hsts_(is_hsts), host_(std::move(host)) {
  if (is_hsts_) {
    base::Time expiry = base::Time::Max();
    bool include_subdomains = false;
    state_->AddHSTS(host_, expiry, include_subdomains);
  }
}

HSTSStateManager::~HSTSStateManager() {
  if (is_hsts_)
    state_->DeleteDynamicDataForHost(host_);
}

}  // namespace

class HSTSQueryTest : public testing::Test {
 public:
  HSTSQueryTest()
      : request_context_(new net::TestURLRequestContextGetter(
            base::ThreadTaskRunnerHandle::Get())) {}

  const scoped_refptr<net::TestURLRequestContextGetter>& request_context() {
    return request_context_;
  }

 private:
  base::MessageLoop message_loop_;  // Used by request_context_.
  scoped_refptr<net::TestURLRequestContextGetter> request_context_;

  DISALLOW_COPY_AND_ASSIGN(HSTSQueryTest);
};

TEST_F(HSTSQueryTest, TestPostHSTSQueryForHostAndRequestContext) {
  const GURL origin("https://example.org");
  for (bool is_hsts : {false, true}) {
    SCOPED_TRACE(testing::Message()
                 << std::boolalpha << "is_hsts: " << is_hsts);

    HSTSStateManager manager(
        request_context()->GetURLRequestContext()->transport_security_state(),
        is_hsts, origin.host());
    // Post query and ensure callback gets run.
    bool callback_ran = false;
    PostHSTSQueryForHostAndRequestContext(
        origin, request_context(),
        base::Bind(
            [](bool* ran, bool expectation, bool result) {
              *ran = true;
              EXPECT_EQ(expectation, result);
            },
            &callback_ran, is_hsts));
    base::RunLoop().RunUntilIdle();
    EXPECT_TRUE(callback_ran);
  }
}

}  // namespace password_manager
