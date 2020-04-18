// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chromeos/arc/input_method_manager/arc_input_method_manager_bridge.h"

#include <memory>

#include "base/macros.h"
#include "components/arc/arc_service_manager.h"
#include "components/arc/test/test_browser_context.h"
#include "content/public/test/test_browser_thread_bundle.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace arc {
namespace {

class ArcInputMethodManagerBridgeTest : public testing::Test {
 protected:
  ArcInputMethodManagerBridgeTest()
      : arc_service_manager_(std::make_unique<ArcServiceManager>()),
        context_(std::make_unique<TestBrowserContext>()),
        service_(ArcInputMethodManagerBridge::GetForBrowserContextForTesting(
            context_.get())) {}
  ~ArcInputMethodManagerBridgeTest() override { service_->Shutdown(); }

  ArcInputMethodManagerBridge* service() { return service_; }

 private:
  content::TestBrowserThreadBundle thread_bundle_;
  std::unique_ptr<ArcServiceManager> arc_service_manager_;
  std::unique_ptr<TestBrowserContext> context_;

  ArcInputMethodManagerBridge* const service_;

  DISALLOW_COPY_AND_ASSIGN(ArcInputMethodManagerBridgeTest);
};

}  // anonymous namespace

TEST_F(ArcInputMethodManagerBridgeTest, ConstructAndDestruct) {
  // These two method are not implemented yet.
  ASSERT_TRUE(service() != nullptr);
  service()->OnActiveImeChanged("");
  service()->OnImeInfoChanged({});
  SUCCEED();
}

}  // namespace arc
