// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/autofill/core/browser/autofill_experiments.h"

#include "base/test/scoped_feature_list.h"
#include "components/autofill/core/browser/test_sync_service.h"
#include "components/autofill/core/common/autofill_pref_names.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/testing_pref_service.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace autofill {

class AutofillExperimentsTest : public testing::Test {
 public:
  AutofillExperimentsTest() {}

 protected:
  void SetUp() override {
    pref_service_.registry()->RegisterBooleanPref(
        prefs::kAutofillWalletImportEnabled, true);
  }

  bool IsCreditCardUploadEnabled() {
    return IsCreditCardUploadEnabled("john.smith@gmail.com");
  }

  bool IsCreditCardUploadEnabled(const std::string& user_email) {
    return autofill::IsCreditCardUploadEnabled(&pref_service_, &sync_service_,
                                               user_email);
  }

  base::test::ScopedFeatureList scoped_feature_list_;
  TestingPrefServiceSimple pref_service_;
  TestSyncService sync_service_;
};

TEST_F(AutofillExperimentsTest, DenyUpload_FeatureEnabled) {
  scoped_feature_list_.InitAndEnableFeature(kAutofillUpstream);
  EXPECT_TRUE(IsCreditCardUploadEnabled());
}

TEST_F(AutofillExperimentsTest, DenyUpload_FeatureDisabled) {
  scoped_feature_list_.InitAndDisableFeature(kAutofillUpstream);
  EXPECT_FALSE(IsCreditCardUploadEnabled());
}

TEST_F(AutofillExperimentsTest, DenyUpload_SyncServiceCannotStart) {
  scoped_feature_list_.InitAndEnableFeature(kAutofillUpstream);
  sync_service_.SetCanSyncStart(false);
  EXPECT_FALSE(IsCreditCardUploadEnabled());
}

TEST_F(AutofillExperimentsTest, DenyUpload_AuthError) {
  scoped_feature_list_.InitAndEnableFeature(kAutofillUpstream);
  sync_service_.SetInAuthError(true);
  EXPECT_FALSE(IsCreditCardUploadEnabled());
}

TEST_F(AutofillExperimentsTest,
       DenyUpload_SyncServiceDoesNotHaveAutofillProfilePreferredDataType) {
  scoped_feature_list_.InitAndEnableFeature(kAutofillUpstream);
  sync_service_.SetPreferredDataTypes(syncer::ModelTypeSet());
  EXPECT_FALSE(IsCreditCardUploadEnabled());
}

TEST_F(AutofillExperimentsTest, DenyUpload_SyncCycleNotComplete) {
  scoped_feature_list_.InitAndEnableFeature(kAutofillUpstream);
  sync_service_.SetSyncCycleComplete(false);
  EXPECT_FALSE(IsCreditCardUploadEnabled());
}

TEST_F(AutofillExperimentsTest, DenyUpload_SyncConfigurationNotDone) {
  scoped_feature_list_.InitAndEnableFeature(kAutofillUpstream);
  sync_service_.SetConfigurationDone(false);
  EXPECT_FALSE(IsCreditCardUploadEnabled());
}

TEST_F(AutofillExperimentsTest,
       DenyUpload_SyncServiceUsingSecondaryPassphrase) {
  scoped_feature_list_.InitAndEnableFeature(kAutofillUpstream);
  sync_service_.SetIsUsingSecondaryPassphrase(true);
  EXPECT_FALSE(IsCreditCardUploadEnabled());
}

TEST_F(AutofillExperimentsTest,
       DenyUpload_AutofillWalletImportEnabledPrefIsDisabled) {
  scoped_feature_list_.InitAndEnableFeature(kAutofillUpstream);
  pref_service_.SetBoolean(prefs::kAutofillWalletImportEnabled, false);
  EXPECT_FALSE(IsCreditCardUploadEnabled());
}

TEST_F(AutofillExperimentsTest, DenyUpload_EmptyUserEmail) {
  scoped_feature_list_.InitAndEnableFeature(kAutofillUpstream);
  EXPECT_FALSE(IsCreditCardUploadEnabled(""));
}

TEST_F(AutofillExperimentsTest, AllowUpload_UserEmailWithGoogleDomain) {
  scoped_feature_list_.InitAndEnableFeature(kAutofillUpstream);
  EXPECT_TRUE(IsCreditCardUploadEnabled("john.smith@gmail.com"));
  EXPECT_TRUE(IsCreditCardUploadEnabled("googler@google.com"));
  EXPECT_TRUE(IsCreditCardUploadEnabled("old.school@googlemail.com"));
  EXPECT_TRUE(IsCreditCardUploadEnabled("code.committer@chromium.org"));
}

TEST_F(AutofillExperimentsTest, DenyUpload_UserEmailWithNonGoogleDomain) {
  scoped_feature_list_.InitAndEnableFeature(kAutofillUpstream);
  EXPECT_FALSE(IsCreditCardUploadEnabled("cool.user@hotmail.com"));
  EXPECT_FALSE(IsCreditCardUploadEnabled("john.smith@johnsmith.com"));
  EXPECT_FALSE(IsCreditCardUploadEnabled("fake.googler@google.net"));
  EXPECT_FALSE(IsCreditCardUploadEnabled("fake.committer@chromium.com"));
}

TEST_F(AutofillExperimentsTest,
       AllowUpload_UserEmailWithNonGoogleDomainIfExperimentEnabled) {
  scoped_feature_list_.InitWithFeatures(
      {kAutofillUpstream, kAutofillUpstreamAllowAllEmailDomains}, {});
  EXPECT_TRUE(IsCreditCardUploadEnabled("cool.user@hotmail.com"));
  EXPECT_TRUE(IsCreditCardUploadEnabled("john.smith@johnsmith.com"));
  EXPECT_TRUE(IsCreditCardUploadEnabled("fake.googler@google.net"));
  EXPECT_TRUE(IsCreditCardUploadEnabled("fake.committer@chromium.com"));
}

}  // namespace autofill
