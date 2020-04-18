// Copyright (c) 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <stddef.h>
#include <stdint.h>

#include <sstream>

#include "base/macros.h"
#include "services/device/hid/test_report_descriptors.h"
#include "services/device/public/cpp/hid/hid_report_descriptor.h"
#include "services/device/public/mojom/hid.mojom.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace device {

class HidReportDescriptorTest : public testing::Test {
 protected:
  using HidUsageAndPage = mojom::HidUsageAndPage;
  using HidCollectionInfo = mojom::HidCollectionInfo;
  using HidCollectionInfoPtr = mojom::HidCollectionInfoPtr;

  void SetUp() override { descriptor_ = NULL; }

  void TearDown() override {
    if (descriptor_) {
      delete descriptor_;
    }
  }

 public:
  void ValidateDetails(
      const std::vector<HidCollectionInfoPtr>& expected_collections,
      const bool expected_has_report_id,
      const size_t expected_max_input_report_size,
      const size_t expected_max_output_report_size,
      const size_t expected_max_feature_report_size,
      const uint8_t* bytes,
      size_t size) {
    descriptor_ =
        new HidReportDescriptor(std::vector<uint8_t>(bytes, bytes + size));

    std::vector<HidCollectionInfoPtr> actual_collections;
    bool actual_has_report_id;
    size_t actual_max_input_report_size;
    size_t actual_max_output_report_size;
    size_t actual_max_feature_report_size;
    descriptor_->GetDetails(&actual_collections, &actual_has_report_id,
                            &actual_max_input_report_size,
                            &actual_max_output_report_size,
                            &actual_max_feature_report_size);

    ASSERT_EQ(expected_collections.size(), actual_collections.size());

    auto actual_collections_iter = actual_collections.begin();
    auto expected_collections_iter = expected_collections.begin();

    while (expected_collections_iter != expected_collections.end() &&
           actual_collections_iter != actual_collections.end()) {
      const HidCollectionInfoPtr& expected_collection =
          *expected_collections_iter;
      const HidCollectionInfoPtr& actual_collection = *actual_collections_iter;

      ASSERT_EQ(expected_collection->usage->usage_page,
                actual_collection->usage->usage_page);
      ASSERT_EQ(expected_collection->usage->usage,
                actual_collection->usage->usage);
      ASSERT_THAT(actual_collection->report_ids,
                  testing::ContainerEq(expected_collection->report_ids));

      expected_collections_iter++;
      actual_collections_iter++;
    }

    ASSERT_EQ(expected_has_report_id, actual_has_report_id);
    ASSERT_EQ(expected_max_input_report_size, actual_max_input_report_size);
    ASSERT_EQ(expected_max_output_report_size, actual_max_output_report_size);
    ASSERT_EQ(expected_max_feature_report_size, actual_max_feature_report_size);
  }

 private:
  HidReportDescriptor* descriptor_;
};

TEST_F(HidReportDescriptorTest, ValidateDetails_Digitizer) {
  auto digitizer = HidCollectionInfo::New();
  digitizer->usage = HidUsageAndPage::New(0x01, mojom::kPageDigitizer);
  digitizer->report_ids.push_back(1);
  digitizer->report_ids.push_back(2);
  digitizer->report_ids.push_back(3);
  std::vector<HidCollectionInfoPtr> expected;
  expected.push_back(std::move(digitizer));
  ValidateDetails(expected, true, 6, 0, 0, kDigitizer, kDigitizerSize);
}

TEST_F(HidReportDescriptorTest, ValidateDetails_Keyboard) {
  auto keyboard = HidCollectionInfo::New();
  keyboard->usage = HidUsageAndPage::New(0x06, mojom::kPageGenericDesktop);
  std::vector<HidCollectionInfoPtr> expected;
  expected.push_back(std::move(keyboard));
  ValidateDetails(expected, false, 8, 1, 0, kKeyboard, kKeyboardSize);
}

TEST_F(HidReportDescriptorTest, ValidateDetails_Monitor) {
  auto monitor = HidCollectionInfo::New();
  monitor->usage = HidUsageAndPage::New(0x01, mojom::kPageMonitor0);
  monitor->report_ids.push_back(1);
  monitor->report_ids.push_back(2);
  monitor->report_ids.push_back(3);
  monitor->report_ids.push_back(4);
  monitor->report_ids.push_back(5);
  std::vector<HidCollectionInfoPtr> expected;
  expected.push_back(std::move(monitor));
  ValidateDetails(expected, true, 0, 0, 243, kMonitor, kMonitorSize);
}

TEST_F(HidReportDescriptorTest, ValidateDetails_Mouse) {
  auto mouse = HidCollectionInfo::New();
  mouse->usage = HidUsageAndPage::New(0x02, mojom::kPageGenericDesktop);
  std::vector<HidCollectionInfoPtr> expected;
  expected.push_back(std::move(mouse));
  ValidateDetails(expected, false, 3, 0, 0, kMouse, kMouseSize);
}

TEST_F(HidReportDescriptorTest, ValidateDetails_LogitechUnifyingReceiver) {
  auto hidpp_short = HidCollectionInfo::New();
  hidpp_short->usage = HidUsageAndPage::New(0x01, mojom::kPageVendor);
  hidpp_short->report_ids.push_back(0x10);
  auto hidpp_long = HidCollectionInfo::New();
  hidpp_long->usage = HidUsageAndPage::New(0x02, mojom::kPageVendor);
  hidpp_long->report_ids.push_back(0x11);
  auto hidpp_dj = HidCollectionInfo::New();
  hidpp_dj->usage = HidUsageAndPage::New(0x04, mojom::kPageVendor);
  hidpp_dj->report_ids.push_back(0x20);
  hidpp_dj->report_ids.push_back(0x21);

  std::vector<HidCollectionInfoPtr> expected;
  expected.push_back(std::move(hidpp_short));
  expected.push_back(std::move(hidpp_long));
  expected.push_back(std::move(hidpp_dj));
  ValidateDetails(expected, true, 31, 31, 0, kLogitechUnifyingReceiver,
                  kLogitechUnifyingReceiverSize);
}

}  // namespace device
