// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/service_worker/service_worker_context_core.h"

#include <memory>

#include "content/browser/service_worker/embedded_worker_test_helper.h"
#include "content/browser/service_worker/service_worker_test_utils.h"
#include "content/public/test/test_browser_thread_bundle.h"
#include "content/public/test/test_utils.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace content {

class ServiceWorkerContextCoreTest : public testing::Test {
 protected:
  ServiceWorkerContextCoreTest()
      : thread_bundle_(TestBrowserThreadBundle::IO_MAINLOOP) {}

  void SetUp() override {
    helper_.reset(new EmbeddedWorkerTestHelper(base::FilePath()));
  }

  void TearDown() override { helper_.reset(); }

  ServiceWorkerContextCore* context() { return helper_->context(); }

  TestBrowserThreadBundle thread_bundle_;
  std::unique_ptr<EmbeddedWorkerTestHelper> helper_;

 private:
  DISALLOW_COPY_AND_ASSIGN(ServiceWorkerContextCoreTest);
};

TEST_F(ServiceWorkerContextCoreTest, FailureInfo) {
  const int64_t kVersionId = 55;  // dummy value

  EXPECT_EQ(0, context()->GetVersionFailureCount(kVersionId));
  context()->UpdateVersionFailureCount(kVersionId, SERVICE_WORKER_OK);
  context()->UpdateVersionFailureCount(kVersionId,
                                       SERVICE_WORKER_ERROR_DISALLOWED);
  EXPECT_EQ(0, context()->GetVersionFailureCount(kVersionId));

  context()->UpdateVersionFailureCount(kVersionId,
                                       SERVICE_WORKER_ERROR_NETWORK);
  EXPECT_EQ(1, context()->GetVersionFailureCount(kVersionId));
  EXPECT_EQ(SERVICE_WORKER_ERROR_NETWORK,
            context()->failure_counts_[kVersionId].last_failure);

  context()->UpdateVersionFailureCount(kVersionId, SERVICE_WORKER_ERROR_ABORT);
  EXPECT_EQ(2, context()->GetVersionFailureCount(kVersionId));
  EXPECT_EQ(SERVICE_WORKER_ERROR_ABORT,
            context()->failure_counts_[kVersionId].last_failure);

  context()->UpdateVersionFailureCount(kVersionId, SERVICE_WORKER_OK);
  EXPECT_EQ(0, context()->GetVersionFailureCount(kVersionId));
  EXPECT_FALSE(base::ContainsKey(context()->failure_counts_, kVersionId));
}

}  // namespace content
