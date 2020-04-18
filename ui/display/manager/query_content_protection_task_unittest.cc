// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/display/manager/query_content_protection_task.h"

#include <stdint.h>
#include <memory>
#include <utility>
#include <vector>

#include "base/bind.h"
#include "base/macros.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "ui/display/manager/display_layout_manager.h"
#include "ui/display/manager/fake_display_snapshot.h"
#include "ui/display/manager/test/action_logger_util.h"
#include "ui/display/manager/test/test_display_layout_manager.h"
#include "ui/display/manager/test/test_native_display_delegate.h"

namespace display {
namespace test {

namespace {

std::unique_ptr<DisplaySnapshot> CreateDisplaySnapshot(
    int64_t id,
    DisplayConnectionType type) {
  return FakeDisplaySnapshot::Builder()
      .SetId(id)
      .SetNativeMode(gfx::Size(1024, 768))
      .SetType(type)
      .Build();
}

}  // namespace

class QueryContentProtectionTaskTest : public testing::Test {
 public:
  QueryContentProtectionTaskTest()
      : display_delegate_(&log_), has_response_(false) {}
  ~QueryContentProtectionTaskTest() override {}

  void ResponseCallback(QueryContentProtectionTask::Response response) {
    has_response_ = true;
    response_ = response;
  }

 protected:
  ActionLogger log_;
  TestNativeDisplayDelegate display_delegate_;

  bool has_response_;
  QueryContentProtectionTask::Response response_;

 private:
  DISALLOW_COPY_AND_ASSIGN(QueryContentProtectionTaskTest);
};

TEST_F(QueryContentProtectionTaskTest, QueryWithNoHDCPCapableDisplay) {
  std::vector<std::unique_ptr<DisplaySnapshot>> displays;
  displays.push_back(
      CreateDisplaySnapshot(1, DISPLAY_CONNECTION_TYPE_INTERNAL));
  TestDisplayLayoutManager layout_manager(std::move(displays),
                                          MULTIPLE_DISPLAY_STATE_SINGLE);

  QueryContentProtectionTask task(
      &layout_manager, &display_delegate_, 1,
      base::Bind(&QueryContentProtectionTaskTest::ResponseCallback,
                 base::Unretained(this)));
  task.Run();

  EXPECT_TRUE(has_response_);
  EXPECT_TRUE(response_.success);
  EXPECT_EQ(DISPLAY_CONNECTION_TYPE_INTERNAL, response_.link_mask);
  EXPECT_EQ(0u, response_.enabled);
  EXPECT_EQ(0u, response_.unfulfilled);
}

TEST_F(QueryContentProtectionTaskTest, QueryWithUnknownDisplay) {
  std::vector<std::unique_ptr<DisplaySnapshot>> displays;
  displays.push_back(CreateDisplaySnapshot(1, DISPLAY_CONNECTION_TYPE_UNKNOWN));
  TestDisplayLayoutManager layout_manager(std::move(displays),
                                          MULTIPLE_DISPLAY_STATE_SINGLE);

  QueryContentProtectionTask task(
      &layout_manager, &display_delegate_, 1,
      base::Bind(&QueryContentProtectionTaskTest::ResponseCallback,
                 base::Unretained(this)));
  task.Run();

  EXPECT_TRUE(has_response_);
  EXPECT_FALSE(response_.success);
  EXPECT_EQ(DISPLAY_CONNECTION_TYPE_UNKNOWN, response_.link_mask);
  EXPECT_EQ(0u, response_.enabled);
  EXPECT_EQ(0u, response_.unfulfilled);
}

TEST_F(QueryContentProtectionTaskTest, FailQueryWithHDMIDisplay) {
  std::vector<std::unique_ptr<DisplaySnapshot>> displays;
  displays.push_back(CreateDisplaySnapshot(1, DISPLAY_CONNECTION_TYPE_HDMI));
  TestDisplayLayoutManager layout_manager(std::move(displays),
                                          MULTIPLE_DISPLAY_STATE_SINGLE);
  display_delegate_.set_get_hdcp_state_expectation(false);

  QueryContentProtectionTask task(
      &layout_manager, &display_delegate_, 1,
      base::Bind(&QueryContentProtectionTaskTest::ResponseCallback,
                 base::Unretained(this)));
  task.Run();

  EXPECT_TRUE(has_response_);
  EXPECT_FALSE(response_.success);
  EXPECT_EQ(DISPLAY_CONNECTION_TYPE_HDMI, response_.link_mask);
}

TEST_F(QueryContentProtectionTaskTest, QueryWithHDMIDisplayAndUnfulfilled) {
  std::vector<std::unique_ptr<DisplaySnapshot>> displays;
  displays.push_back(CreateDisplaySnapshot(1, DISPLAY_CONNECTION_TYPE_HDMI));
  TestDisplayLayoutManager layout_manager(std::move(displays),
                                          MULTIPLE_DISPLAY_STATE_SINGLE);

  QueryContentProtectionTask task(
      &layout_manager, &display_delegate_, 1,
      base::Bind(&QueryContentProtectionTaskTest::ResponseCallback,
                 base::Unretained(this)));
  task.Run();

  EXPECT_TRUE(has_response_);
  EXPECT_TRUE(response_.success);
  EXPECT_EQ(DISPLAY_CONNECTION_TYPE_HDMI, response_.link_mask);
  EXPECT_EQ(0u, response_.enabled);
  EXPECT_EQ(CONTENT_PROTECTION_METHOD_HDCP, response_.unfulfilled);
}

TEST_F(QueryContentProtectionTaskTest, QueryWithHDMIDisplayAndFulfilled) {
  std::vector<std::unique_ptr<DisplaySnapshot>> displays;
  displays.push_back(CreateDisplaySnapshot(1, DISPLAY_CONNECTION_TYPE_HDMI));
  TestDisplayLayoutManager layout_manager(std::move(displays),
                                          MULTIPLE_DISPLAY_STATE_SINGLE);
  display_delegate_.set_hdcp_state(HDCP_STATE_ENABLED);

  QueryContentProtectionTask task(
      &layout_manager, &display_delegate_, 1,
      base::Bind(&QueryContentProtectionTaskTest::ResponseCallback,
                 base::Unretained(this)));
  task.Run();

  EXPECT_TRUE(has_response_);
  EXPECT_TRUE(response_.success);
  EXPECT_EQ(DISPLAY_CONNECTION_TYPE_HDMI, response_.link_mask);
  EXPECT_EQ(CONTENT_PROTECTION_METHOD_HDCP, response_.enabled);
  EXPECT_EQ(0u, response_.unfulfilled);
}

TEST_F(QueryContentProtectionTaskTest, QueryWith2HDCPDisplays) {
  std::vector<std::unique_ptr<DisplaySnapshot>> displays;
  displays.push_back(CreateDisplaySnapshot(1, DISPLAY_CONNECTION_TYPE_HDMI));
  displays.push_back(CreateDisplaySnapshot(2, DISPLAY_CONNECTION_TYPE_DVI));
  TestDisplayLayoutManager layout_manager(
      std::move(displays), MULTIPLE_DISPLAY_STATE_MULTI_EXTENDED);

  QueryContentProtectionTask task(
      &layout_manager, &display_delegate_, 1,
      base::Bind(&QueryContentProtectionTaskTest::ResponseCallback,
                 base::Unretained(this)));
  task.Run();

  EXPECT_TRUE(has_response_);
  EXPECT_TRUE(response_.success);
  EXPECT_EQ(DISPLAY_CONNECTION_TYPE_HDMI, response_.link_mask);
  EXPECT_EQ(0u, response_.enabled);
  EXPECT_EQ(CONTENT_PROTECTION_METHOD_HDCP, response_.unfulfilled);
}

TEST_F(QueryContentProtectionTaskTest, QueryWithMirrorHDCPDisplays) {
  std::vector<std::unique_ptr<DisplaySnapshot>> displays;
  displays.push_back(CreateDisplaySnapshot(1, DISPLAY_CONNECTION_TYPE_HDMI));
  displays.push_back(CreateDisplaySnapshot(2, DISPLAY_CONNECTION_TYPE_DVI));
  TestDisplayLayoutManager layout_manager(std::move(displays),
                                          MULTIPLE_DISPLAY_STATE_DUAL_MIRROR);

  QueryContentProtectionTask task(
      &layout_manager, &display_delegate_, 1,
      base::Bind(&QueryContentProtectionTaskTest::ResponseCallback,
                 base::Unretained(this)));
  task.Run();

  EXPECT_TRUE(has_response_);
  EXPECT_TRUE(response_.success);
  EXPECT_EQ(static_cast<uint32_t>(DISPLAY_CONNECTION_TYPE_HDMI |
                                  DISPLAY_CONNECTION_TYPE_DVI),
            response_.link_mask);
  EXPECT_EQ(0u, response_.enabled);
  EXPECT_EQ(CONTENT_PROTECTION_METHOD_HDCP, response_.unfulfilled);
}

}  // namespace test
}  // namespace display
