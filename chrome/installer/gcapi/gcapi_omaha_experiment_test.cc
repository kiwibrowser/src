// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/installer/gcapi/gcapi_omaha_experiment.h"

#include <stdint.h>

#include "base/strings/utf_string_conversions.h"
#include "base/test/test_reg_util_win.h"
#include "chrome/installer/gcapi/gcapi.h"
#include "chrome/installer/util/google_update_constants.h"
#include "chrome/installer/util/google_update_settings.h"
#include "components/variations/variations_experiment_util.h"
#include "testing/gtest/include/gtest/gtest.h"

using base::ASCIIToUTF16;

namespace {

const wchar_t kBrand[] = L"ABCD";
const uint16_t kUserLevel = GCAPI_INVOKED_STANDARD_SHELL;

const wchar_t kSomeExperiments[] = L"myexp=1|Aug 2;yourexp=2|Sep 5";
const wchar_t kSomeOtherExperiments[] = L"anotherexp=joe|Jun 7 2008";
const wchar_t kSomeMoreExperiments[] = L"moreexp=foo|Jul 31 1999";

class GCAPIOmahaExperimentTest : public ::testing::Test {
 protected:
  GCAPIOmahaExperimentTest()
      : brand_(kBrand),
        reactivation_label_(gcapi_internals::GetGCAPIExperimentLabel(
            kBrand, gcapi_internals::kReactivationLabel)),
        relaunch_label_(gcapi_internals::GetGCAPIExperimentLabel(
            kBrand, gcapi_internals::kRelaunchLabel)) {
  }

  void SetUp() override {
    ASSERT_NO_FATAL_FAILURE(
        override_manager_.OverrideRegistry(HKEY_CURRENT_USER));
    ASSERT_NO_FATAL_FAILURE(
        override_manager_.OverrideRegistry(HKEY_LOCAL_MACHINE));
  }

  void VerifyExperimentLabels(const base::string16& expected_labels) {
    base::string16 actual_labels;
    EXPECT_TRUE(GoogleUpdateSettings::ReadExperimentLabels(false,
                                                           &actual_labels));
    EXPECT_EQ(expected_labels, actual_labels);
  }

  registry_util::RegistryOverrideManager override_manager_;
  base::string16 brand_;
  base::string16 reactivation_label_;
  base::string16 relaunch_label_;
};

TEST_F(GCAPIOmahaExperimentTest, SetReactivationLabelFromEmptyExperiments) {
  ASSERT_TRUE(SetReactivationExperimentLabels(kBrand, kUserLevel));
  VerifyExperimentLabels(reactivation_label_);
}

// Test the relaunch label once; all other tests go more in depth, but since
// both labels use the same logic underneath there is no need to test both in
// depth.
TEST_F(GCAPIOmahaExperimentTest, SetRelaunchLabelFromEmptyExperiments) {
  ASSERT_TRUE(SetRelaunchExperimentLabels(kBrand, kUserLevel));
  VerifyExperimentLabels(relaunch_label_);
}

TEST_F(GCAPIOmahaExperimentTest, SetReactivationLabelWithExistingExperiments) {
  GoogleUpdateSettings::SetExperimentLabels(false, kSomeExperiments);

  ASSERT_TRUE(SetReactivationExperimentLabels(kBrand, kUserLevel));

  base::string16 expected_labels(kSomeExperiments);
  expected_labels += variations::kExperimentLabelSeparator;
  expected_labels.append(reactivation_label_);
  VerifyExperimentLabels(expected_labels);
}

TEST_F(GCAPIOmahaExperimentTest,
       SetReactivationLabelWithExistingIdenticalExperiment) {
  base::string16 previous_labels(kSomeExperiments);
  previous_labels += variations::kExperimentLabelSeparator;
  previous_labels.append(reactivation_label_);
  previous_labels += variations::kExperimentLabelSeparator;
  previous_labels.append(kSomeOtherExperiments);
  GoogleUpdateSettings::SetExperimentLabels(false, previous_labels);

  ASSERT_TRUE(SetReactivationExperimentLabels(kBrand, kUserLevel));

  base::string16 expected_labels(kSomeExperiments);
  expected_labels += variations::kExperimentLabelSeparator;
  expected_labels.append(kSomeOtherExperiments);
  expected_labels += variations::kExperimentLabelSeparator;
  expected_labels.append(reactivation_label_);
  VerifyExperimentLabels(expected_labels);
}

TEST_F(GCAPIOmahaExperimentTest,
       SetReactivationLabelWithExistingIdenticalAtBeginning) {
  base::string16 previous_labels(reactivation_label_);
  previous_labels += variations::kExperimentLabelSeparator;
  previous_labels.append(kSomeExperiments);
  GoogleUpdateSettings::SetExperimentLabels(false, previous_labels);

  ASSERT_TRUE(SetReactivationExperimentLabels(kBrand, kUserLevel));

  base::string16 expected_labels(kSomeExperiments);
  expected_labels += variations::kExperimentLabelSeparator;
  expected_labels.append(reactivation_label_);
  VerifyExperimentLabels(expected_labels);
}

TEST_F(GCAPIOmahaExperimentTest,
       SetReactivationLabelWithFakeMatchInAnExperiment) {
  base::string16 previous_labels(kSomeExperiments);
  previous_labels += variations::kExperimentLabelSeparator;
  previous_labels.append(L"blah_");
  // Shouldn't match deletion criteria.
  previous_labels.append(reactivation_label_);
  previous_labels += variations::kExperimentLabelSeparator;
  previous_labels.append(kSomeOtherExperiments);
  previous_labels += variations::kExperimentLabelSeparator;
  // Should match the deletion criteria.
  previous_labels.append(reactivation_label_);
  previous_labels += variations::kExperimentLabelSeparator;
  previous_labels.append(kSomeMoreExperiments);
  GoogleUpdateSettings::SetExperimentLabels(false, previous_labels);

  ASSERT_TRUE(SetReactivationExperimentLabels(kBrand, kUserLevel));

  base::string16 expected_labels(kSomeExperiments);
  expected_labels += variations::kExperimentLabelSeparator;
  expected_labels.append(L"blah_");
  expected_labels.append(reactivation_label_);
  expected_labels += variations::kExperimentLabelSeparator;
  expected_labels.append(kSomeOtherExperiments);
  expected_labels += variations::kExperimentLabelSeparator;
  expected_labels.append(kSomeMoreExperiments);
  expected_labels += variations::kExperimentLabelSeparator;
  expected_labels.append(reactivation_label_);
  VerifyExperimentLabels(expected_labels);
}

TEST_F(GCAPIOmahaExperimentTest,
       SetReactivationLabelWithFakeMatchInAnExperimentAndNoRealMatch) {
  base::string16 previous_labels(kSomeExperiments);
  previous_labels += variations::kExperimentLabelSeparator;
  previous_labels.append(L"blah_");
  // Shouldn't match deletion criteria.
  previous_labels.append(reactivation_label_);
  previous_labels += variations::kExperimentLabelSeparator;
  previous_labels.append(kSomeOtherExperiments);
  GoogleUpdateSettings::SetExperimentLabels(false, previous_labels);

  ASSERT_TRUE(SetReactivationExperimentLabels(kBrand, kUserLevel));

  base::string16 expected_labels(kSomeExperiments);
  expected_labels += variations::kExperimentLabelSeparator;
  expected_labels.append(L"blah_");
  expected_labels.append(reactivation_label_);
  expected_labels += variations::kExperimentLabelSeparator;
  expected_labels.append(kSomeOtherExperiments);
  expected_labels += variations::kExperimentLabelSeparator;
  expected_labels.append(reactivation_label_);
  VerifyExperimentLabels(expected_labels);
}

TEST_F(GCAPIOmahaExperimentTest,
       SetReactivationLabelWithExistingEntryWithLabelAsPrefix) {
  base::string16 previous_labels(kSomeExperiments);
  previous_labels += variations::kExperimentLabelSeparator;
  // Append prefix matching the label, but not followed by '='.
  previous_labels.append(gcapi_internals::kReactivationLabel);
  // Shouldn't match deletion criteria.
  previous_labels.append(kSomeOtherExperiments);
  GoogleUpdateSettings::SetExperimentLabels(false, previous_labels);

  ASSERT_TRUE(SetReactivationExperimentLabels(kBrand, kUserLevel));

  base::string16 expected_labels(kSomeExperiments);
  expected_labels += variations::kExperimentLabelSeparator;
  expected_labels.append(gcapi_internals::kReactivationLabel);
  expected_labels.append(kSomeOtherExperiments);
  expected_labels += variations::kExperimentLabelSeparator;
  expected_labels.append(reactivation_label_);
  VerifyExperimentLabels(expected_labels);
}

}  // namespace
