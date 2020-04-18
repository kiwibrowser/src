// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/password_manager/core/browser/password_autofill_manager.h"

#include <memory>

#include "base/command_line.h"
#include "base/compiler_specific.h"
#include "base/message_loop/message_loop.h"
#include "base/strings/string_util.h"
#include "base/strings/utf_string_conversions.h"
#include "base/test/histogram_tester.h"
#include "base/test/scoped_feature_list.h"
#include "base/test/user_action_tester.h"
#include "build/build_config.h"
#include "components/autofill/core/browser/popup_item_ids.h"
#include "components/autofill/core/browser/suggestion_test_helpers.h"
#include "components/autofill/core/browser/test_autofill_client.h"
#include "components/autofill/core/browser/test_autofill_driver.h"
#include "components/autofill/core/common/autofill_constants.h"
#include "components/autofill/core/common/autofill_switches.h"
#include "components/autofill/core/common/form_field_data.h"
#include "components/autofill/core/common/password_form_fill_data.h"
#include "components/password_manager/core/browser/password_manager.h"
#include "components/password_manager/core/browser/password_manager_metrics_util.h"
#include "components/password_manager/core/browser/stub_password_manager_client.h"
#include "components/password_manager/core/browser/stub_password_manager_driver.h"
#include "components/password_manager/core/common/password_manager_features.h"
#include "components/security_state/core/security_state.h"
#include "components/strings/grit/components_strings.h"
#include "components/sync/driver/fake_sync_service.h"
#include "components/ukm/test_ukm_recorder.h"
#include "services/metrics/public/cpp/ukm_builders.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/gfx/geometry/rect_f.h"

#if defined(OS_ANDROID)
#include "base/android/build_info.h"
#endif

// The name of the username/password element in the form.
const char kUsernameName[] = "username";
const char kInvalidUsername[] = "no-username";
const char kPasswordName[] = "password";

const char kAliceUsername[] = "alice";
const char kAlicePassword[] = "password";

using autofill::Suggestion;
using autofill::SuggestionVectorIdsAre;
using autofill::SuggestionVectorValuesAre;
using autofill::SuggestionVectorLabelsAre;
using testing::_;

using UkmEntry = ukm::builders::PageWithPassword;

namespace autofill {
class AutofillPopupDelegate;
}

namespace password_manager {

namespace {

constexpr char kMainFrameUrl[] = "https://example.com/";

class MockPasswordManagerDriver : public StubPasswordManagerDriver {
 public:
  MOCK_METHOD2(FillSuggestion,
               void(const base::string16&, const base::string16&));
  MOCK_METHOD2(PreviewSuggestion,
               void(const base::string16&, const base::string16&));
  MOCK_METHOD0(GetPasswordManager, PasswordManager*());
};

class TestPasswordManagerClient : public StubPasswordManagerClient {
 public:
  TestPasswordManagerClient() : main_frame_url_(kMainFrameUrl) {}
  ~TestPasswordManagerClient() override = default;

  MockPasswordManagerDriver* mock_driver() { return &driver_; }
  const GURL& GetMainFrameURL() const override { return main_frame_url_; }

 private:
  MockPasswordManagerDriver driver_;
  GURL main_frame_url_;
};

class MockSyncService : public syncer::FakeSyncService {
 public:
  MockSyncService() {}
  ~MockSyncService() override {}
  MOCK_CONST_METHOD0(IsFirstSetupComplete, bool());
  MOCK_CONST_METHOD0(IsSyncActive, bool());
  MOCK_CONST_METHOD0(IsUsingSecondaryPassphrase, bool());
  MOCK_CONST_METHOD0(GetActiveDataTypes, syncer::ModelTypeSet());
};

class MockAutofillClient : public autofill::TestAutofillClient {
 public:
  MockAutofillClient() : sync_service_(nullptr) {}
  MockAutofillClient(MockSyncService* sync_service)
      : sync_service_(sync_service) {
    LOG(ERROR) << "init mpck client";
  }
  MOCK_METHOD4(ShowAutofillPopup,
               void(const gfx::RectF& element_bounds,
                    base::i18n::TextDirection text_direction,
                    const std::vector<Suggestion>& suggestions,
                    base::WeakPtr<autofill::AutofillPopupDelegate> delegate));
  MOCK_METHOD0(HideAutofillPopup, void());
  MOCK_METHOD1(ExecuteCommand, void(int));

  syncer::SyncService* GetSyncService() override { return sync_service_; }

 private:
  MockSyncService* sync_service_;
};

bool IsPreLollipopAndroid() {
#if defined(OS_ANDROID)
  return (base::android::BuildInfo::GetInstance()->sdk_int() <
          base::android::SDK_VERSION_LOLLIPOP);
#else
  return false;
#endif
}

}  // namespace

class PasswordAutofillManagerTest : public testing::Test {
 protected:
  PasswordAutofillManagerTest()
      : test_username_(base::ASCIIToUTF16(kAliceUsername)),
        test_password_(base::ASCIIToUTF16(kAlicePassword)),
        fill_data_id_(0) {}

  void SetUp() override {
    // Add a preferred login and an additional login to the FillData.
    autofill::FormFieldData username_field;
    username_field.name = base::ASCIIToUTF16(kUsernameName);
    username_field.value = test_username_;
    fill_data_.username_field = username_field;

    autofill::FormFieldData password_field;
    password_field.name = base::ASCIIToUTF16(kPasswordName);
    password_field.value = test_password_;
    fill_data_.password_field = password_field;
  }

  void InitializePasswordAutofillManager(
      TestPasswordManagerClient* client,
      autofill::AutofillClient* autofill_client) {
    password_autofill_manager_.reset(new PasswordAutofillManager(
        client->mock_driver(), autofill_client, client));
    password_autofill_manager_->OnAddPasswordFormMapping(fill_data_id_,
                                                         fill_data_);
  }

 protected:
  int fill_data_id() { return fill_data_id_; }
  autofill::PasswordFormFillData& fill_data() { return fill_data_; }

  void SetManualFallbacksForFilling(bool enabled) {
    if (enabled) {
      scoped_feature_list_.InitAndEnableFeature(
          password_manager::features::kManualFallbacksFilling);
    } else {
      scoped_feature_list_.InitAndDisableFeature(
          password_manager::features::kManualFallbacksFilling);
    }
  }

  void SetManualFallbacksForFillingStandalone(bool enabled) {
    if (enabled) {
      scoped_feature_list_.InitAndEnableFeature(
          password_manager::features::kEnableManualFallbacksFillingStandalone);
    } else {
      scoped_feature_list_.InitAndDisableFeature(
          password_manager::features::kEnableManualFallbacksFillingStandalone);
    }
  }

  static bool IsManualFallbackForFillingEnabled() {
    return base::FeatureList::IsEnabled(
               password_manager::features::kManualFallbacksFilling) &&
           !IsPreLollipopAndroid();
  }

  void SetManualFallbacks(bool enabled) {
    std::vector<std::string> features = {
        password_manager::features::kManualFallbacksFilling.name,
        password_manager::features::kEnableManualFallbacksFillingStandalone
            .name,
        password_manager::features::kEnableManualFallbacksGeneration.name};
    if (enabled) {
      scoped_feature_list_.InitFromCommandLine(base::JoinString(features, ","),
                                               std::string());
    } else {
      scoped_feature_list_.InitFromCommandLine(std::string(),
                                               base::JoinString(features, ","));
    }
  }

  void TestGenerationFallback(bool custom_passphrase_enabled) {
    MockSyncService mock_sync_service;
    EXPECT_CALL(mock_sync_service, IsFirstSetupComplete())
        .WillRepeatedly(::testing::Return(true));
    EXPECT_CALL(mock_sync_service, IsSyncActive())
        .WillRepeatedly(::testing::Return(true));
    EXPECT_CALL(mock_sync_service, GetActiveDataTypes())
        .Times(::testing::AnyNumber())
        .WillRepeatedly(
            ::testing::Return(syncer::ModelTypeSet(syncer::PASSWORDS)));
    EXPECT_CALL(mock_sync_service, IsUsingSecondaryPassphrase())
        .WillRepeatedly(::testing::Return(custom_passphrase_enabled));
    std::unique_ptr<TestPasswordManagerClient> client(
        new TestPasswordManagerClient);
    std::unique_ptr<MockAutofillClient> autofill_client(
        new MockAutofillClient(&mock_sync_service));
    InitializePasswordAutofillManager(client.get(), autofill_client.get());

    gfx::RectF element_bounds;
    autofill::PasswordFormFillData data;
    data.username_field.value = test_username_;
    data.password_field.value = test_password_;
    data.origin = GURL("https://foo.test");

    int dummy_key = 0;
    password_autofill_manager_->OnAddPasswordFormMapping(dummy_key, data);
    SetManualFallbacks(true);

    std::vector<base::string16> elements = {
        l10n_util::GetStringUTF16(
            IDS_AUTOFILL_PASSWORD_FIELD_SUGGESTIONS_TITLE),
        test_username_};
    if (!IsPreLollipopAndroid() || !custom_passphrase_enabled) {
#if !defined(OS_ANDROID)
      elements.push_back(base::string16());
#endif
      elements.push_back(
          l10n_util::GetStringUTF16(IDS_AUTOFILL_SHOW_ALL_SAVED_FALLBACK));
      if (!custom_passphrase_enabled) {
#if !defined(OS_ANDROID)
        elements.push_back(base::string16());
#endif
        elements.push_back(
            l10n_util::GetStringUTF16(IDS_AUTOFILL_GENERATE_PASSWORD_FALLBACK));
      }
    }

    EXPECT_CALL(
        *autofill_client,
        ShowAutofillPopup(
            element_bounds, _,
            SuggestionVectorValuesAre(testing::ElementsAreArray(elements)), _));

    password_autofill_manager_->OnShowPasswordSuggestions(
        dummy_key, base::i18n::RIGHT_TO_LEFT, test_username_,
        autofill::IS_PASSWORD_FIELD, element_bounds);
  }

  std::unique_ptr<PasswordAutofillManager> password_autofill_manager_;

  base::string16 test_username_;
  base::string16 test_password_;

 private:
  autofill::PasswordFormFillData fill_data_;
  const int fill_data_id_;
  base::test::ScopedFeatureList scoped_feature_list_;

  // The TestAutofillDriver uses a SequencedWorkerPool which expects the
  // existence of a MessageLoop.
  base::MessageLoop message_loop_;
};

TEST_F(PasswordAutofillManagerTest, FillSuggestion) {
  std::unique_ptr<TestPasswordManagerClient> client(
      new TestPasswordManagerClient);
  InitializePasswordAutofillManager(client.get(), nullptr);

  EXPECT_CALL(*client->mock_driver(),
              FillSuggestion(test_username_, test_password_));
  EXPECT_TRUE(password_autofill_manager_->FillSuggestionForTest(
      fill_data_id(), test_username_));
  testing::Mock::VerifyAndClearExpectations(client->mock_driver());

  EXPECT_CALL(*client->mock_driver(), FillSuggestion(_, _)).Times(0);
  EXPECT_FALSE(password_autofill_manager_->FillSuggestionForTest(
      fill_data_id(), base::ASCIIToUTF16(kInvalidUsername)));

  const int invalid_fill_data_id = fill_data_id() + 1;

  EXPECT_FALSE(password_autofill_manager_->FillSuggestionForTest(
      invalid_fill_data_id, test_username_));

  password_autofill_manager_->DidNavigateMainFrame();
  EXPECT_FALSE(password_autofill_manager_->FillSuggestionForTest(
      fill_data_id(), test_username_));
}

TEST_F(PasswordAutofillManagerTest, PreviewSuggestion) {
  std::unique_ptr<TestPasswordManagerClient> client(
      new TestPasswordManagerClient);
  InitializePasswordAutofillManager(client.get(), nullptr);

  EXPECT_CALL(*client->mock_driver(),
              PreviewSuggestion(test_username_, test_password_));
  EXPECT_TRUE(password_autofill_manager_->PreviewSuggestionForTest(
      fill_data_id(), test_username_));
  testing::Mock::VerifyAndClearExpectations(client->mock_driver());

  EXPECT_CALL(*client->mock_driver(), PreviewSuggestion(_, _)).Times(0);
  EXPECT_FALSE(password_autofill_manager_->PreviewSuggestionForTest(
      fill_data_id(), base::ASCIIToUTF16(kInvalidUsername)));

  const int invalid_fill_data_id = fill_data_id() + 1;

  EXPECT_FALSE(password_autofill_manager_->PreviewSuggestionForTest(
      invalid_fill_data_id, test_username_));

  password_autofill_manager_->DidNavigateMainFrame();
  EXPECT_FALSE(password_autofill_manager_->PreviewSuggestionForTest(
      fill_data_id(), test_username_));
}

// Test that the popup is marked as visible after recieving password
// suggestions.
TEST_F(PasswordAutofillManagerTest, ExternalDelegatePasswordSuggestions) {
  for (bool is_suggestion_on_password_field : {false, true}) {
    std::unique_ptr<TestPasswordManagerClient> client(
        new TestPasswordManagerClient);
    std::unique_ptr<MockAutofillClient> autofill_client(new MockAutofillClient);
    InitializePasswordAutofillManager(client.get(), autofill_client.get());

    gfx::RectF element_bounds;
    autofill::PasswordFormFillData data;
    data.username_field.value = test_username_;
    data.password_field.value = test_password_;
    data.preferred_realm = "http://foo.com/";
    int dummy_key = 0;
    password_autofill_manager_->OnAddPasswordFormMapping(dummy_key, data);

    EXPECT_CALL(*client->mock_driver(),
                FillSuggestion(test_username_, test_password_));

    std::vector<autofill::PopupItemId> ids = {
        autofill::POPUP_ITEM_ID_USERNAME_ENTRY};
    if (is_suggestion_on_password_field) {
      ids = {autofill::POPUP_ITEM_ID_TITLE,
             autofill::POPUP_ITEM_ID_PASSWORD_ENTRY};
      if (IsManualFallbackForFillingEnabled()) {
#if !defined(OS_ANDROID)
        ids.push_back(autofill::POPUP_ITEM_ID_SEPARATOR);
#endif
        ids.push_back(autofill::POPUP_ITEM_ID_ALL_SAVED_PASSWORDS_ENTRY);
      }
    }
    EXPECT_CALL(
        *autofill_client,
        ShowAutofillPopup(
            _, _, SuggestionVectorIdsAre(testing::ElementsAreArray(ids)), _));

    int show_suggestion_options =
        is_suggestion_on_password_field ? autofill::IS_PASSWORD_FIELD : 0;
    password_autofill_manager_->OnShowPasswordSuggestions(
        dummy_key, base::i18n::RIGHT_TO_LEFT, base::string16(),
        show_suggestion_options, element_bounds);

    // Accepting a suggestion should trigger a call to hide the popup.
    EXPECT_CALL(*autofill_client, HideAutofillPopup());
    password_autofill_manager_->DidAcceptSuggestion(
        test_username_, is_suggestion_on_password_field
                            ? autofill::POPUP_ITEM_ID_PASSWORD_ENTRY
                            : autofill::POPUP_ITEM_ID_USERNAME_ENTRY,
        1);
  }
}

// Test that OnShowPasswordSuggestions correctly matches the given FormFieldData
// to the known PasswordFormFillData, and extracts the right suggestions.
TEST_F(PasswordAutofillManagerTest, ExtractSuggestions) {
  std::unique_ptr<TestPasswordManagerClient> client(
      new TestPasswordManagerClient);
  std::unique_ptr<MockAutofillClient> autofill_client(new MockAutofillClient);
  InitializePasswordAutofillManager(client.get(), autofill_client.get());

  gfx::RectF element_bounds;
  autofill::PasswordFormFillData data;
  data.username_field.value = test_username_;
  data.password_field.value = test_password_;
  data.preferred_realm = "http://foo.com/";

  autofill::PasswordAndRealm additional;
  additional.realm = "https://foobarrealm.org";
  base::string16 additional_username(base::ASCIIToUTF16("John Foo"));
  data.additional_logins[additional_username] = additional;

  int dummy_key = 0;
  password_autofill_manager_->OnAddPasswordFormMapping(dummy_key, data);

  // First, simulate displaying suggestions matching an empty prefix. Also
  // verify that both the values and labels are filled correctly. The 'value'
  // should be the user name; the 'label' should be the realm.
  EXPECT_CALL(*autofill_client,
              ShowAutofillPopup(
                  element_bounds, _,
                  testing::AllOf(
                      SuggestionVectorValuesAre(testing::UnorderedElementsAre(
                          test_username_, additional_username)),
                      SuggestionVectorLabelsAre(testing::UnorderedElementsAre(
                          base::UTF8ToUTF16(data.preferred_realm),
                          base::UTF8ToUTF16(additional.realm)))),
                  _));
  password_autofill_manager_->OnShowPasswordSuggestions(
      dummy_key, base::i18n::RIGHT_TO_LEFT, base::string16(), false,
      element_bounds);

  // Now simulate displaying suggestions matching "John".
  EXPECT_CALL(
      *autofill_client,
      ShowAutofillPopup(element_bounds, _,
                        SuggestionVectorValuesAre(
                            testing::UnorderedElementsAre(additional_username)),
                        _));
  password_autofill_manager_->OnShowPasswordSuggestions(
      dummy_key, base::i18n::RIGHT_TO_LEFT, base::ASCIIToUTF16("John"), false,
      element_bounds);

  // Finally, simulate displaying all suggestions, without any prefix matching.
  EXPECT_CALL(
      *autofill_client,
      ShowAutofillPopup(element_bounds, _,
                        SuggestionVectorValuesAre(testing::UnorderedElementsAre(
                            test_username_, additional_username)),
                        _));
  password_autofill_manager_->OnShowPasswordSuggestions(
      dummy_key, base::i18n::RIGHT_TO_LEFT, base::ASCIIToUTF16("xyz"), true,
      element_bounds);
}

// Verify that, for Android application credentials, the prettified realms of
// applications are displayed as the labels of suggestions on the UI (for
// matches of all levels of preferredness).
TEST_F(PasswordAutofillManagerTest, PrettifiedAndroidRealmsAreShownAsLabels) {
  std::unique_ptr<TestPasswordManagerClient> client(
      new TestPasswordManagerClient);
  std::unique_ptr<MockAutofillClient> autofill_client(new MockAutofillClient);
  InitializePasswordAutofillManager(client.get(), autofill_client.get());

  autofill::PasswordFormFillData data;
  data.username_field.value = test_username_;
  data.preferred_realm = "android://hash@com.example1.android/";

  autofill::PasswordAndRealm additional;
  additional.realm = "android://hash@com.example2.android/";
  base::string16 additional_username(base::ASCIIToUTF16("John Foo"));
  data.additional_logins[additional_username] = additional;

  const int dummy_key = 0;
  password_autofill_manager_->OnAddPasswordFormMapping(dummy_key, data);

  EXPECT_CALL(*autofill_client,
              ShowAutofillPopup(
                  _, _,
                  SuggestionVectorLabelsAre(testing::UnorderedElementsAre(
                      base::ASCIIToUTF16("android://com.example1.android/"),
                      base::ASCIIToUTF16("android://com.example2.android/"))),
                  _));
  password_autofill_manager_->OnShowPasswordSuggestions(
      dummy_key, base::i18n::RIGHT_TO_LEFT, base::string16(), false,
      gfx::RectF());
}

TEST_F(PasswordAutofillManagerTest, FillSuggestionPasswordField) {
  std::unique_ptr<TestPasswordManagerClient> client(
      new TestPasswordManagerClient);
  std::unique_ptr<MockAutofillClient> autofill_client(new MockAutofillClient);
  InitializePasswordAutofillManager(client.get(), autofill_client.get());

  gfx::RectF element_bounds;
  autofill::PasswordFormFillData data;
  data.username_field.value = test_username_;
  data.password_field.value = test_password_;
  data.preferred_realm = "http://foo.com/";

  autofill::PasswordAndRealm additional;
  additional.realm = "https://foobarrealm.org";
  base::string16 additional_username(base::ASCIIToUTF16("John Foo"));
  data.additional_logins[additional_username] = additional;

  autofill::UsernamesCollectionKey usernames_key;
  usernames_key.realm = "http://yetanother.net";

  int dummy_key = 0;
  password_autofill_manager_->OnAddPasswordFormMapping(dummy_key, data);

  // Simulate displaying suggestions matching a username and specifying that the
  // field is a password field.
  base::string16 title = l10n_util::GetStringUTF16(
      IDS_AUTOFILL_PASSWORD_FIELD_SUGGESTIONS_TITLE);
  std::vector<base::string16> elements = {title, test_username_};
  if (IsManualFallbackForFillingEnabled()) {
    elements = {
      title,
      test_username_,
#if !defined(OS_ANDROID)
      base::string16(),
#endif
      l10n_util::GetStringUTF16(IDS_AUTOFILL_SHOW_ALL_SAVED_FALLBACK)
    };
  }

  EXPECT_CALL(
      *autofill_client,
      ShowAutofillPopup(
          element_bounds, _,
          SuggestionVectorValuesAre(testing::ElementsAreArray(elements)), _));
  password_autofill_manager_->OnShowPasswordSuggestions(
      dummy_key, base::i18n::RIGHT_TO_LEFT, test_username_,
      autofill::IS_PASSWORD_FIELD, element_bounds);
}

// Verify that typing "foo" into the username field will match usernames
// "foo.bar@example.com", "bar.foo@example.com" and "example@foo.com".
TEST_F(PasswordAutofillManagerTest, DisplaySuggestionsWithMatchingTokens) {
  // Token matching is currently behind a flag.
  base::CommandLine::ForCurrentProcess()->AppendSwitch(
      autofill::switches::kEnableSuggestionsWithSubstringMatch);

  std::unique_ptr<TestPasswordManagerClient> client(
      new TestPasswordManagerClient);
  std::unique_ptr<MockAutofillClient> autofill_client(new MockAutofillClient);
  InitializePasswordAutofillManager(client.get(), autofill_client.get());

  gfx::RectF element_bounds;
  autofill::PasswordFormFillData data;
  base::string16 username = base::ASCIIToUTF16("foo.bar@example.com");
  data.username_field.value = username;
  data.password_field.value = base::ASCIIToUTF16("foobar");
  data.preferred_realm = "http://foo.com/";

  autofill::PasswordAndRealm additional;
  additional.realm = "https://foobarrealm.org";
  base::string16 additional_username(base::ASCIIToUTF16("bar.foo@example.com"));
  data.additional_logins[additional_username] = additional;

  autofill::UsernamesCollectionKey usernames_key;
  usernames_key.realm = "http://yetanother.net";

  int dummy_key = 0;
  password_autofill_manager_->OnAddPasswordFormMapping(dummy_key, data);

  EXPECT_CALL(
      *autofill_client,
      ShowAutofillPopup(element_bounds, _,
                        SuggestionVectorValuesAre(testing::UnorderedElementsAre(
                            username, additional_username)),
                        _));
  password_autofill_manager_->OnShowPasswordSuggestions(
      dummy_key, base::i18n::RIGHT_TO_LEFT, base::ASCIIToUTF16("foo"), false,
      element_bounds);
}

// Verify that typing "oo" into the username field will not match any usernames
// "foo.bar@example.com", "bar.foo@example.com" or "example@foo.com".
TEST_F(PasswordAutofillManagerTest, NoSuggestionForNonPrefixTokenMatch) {
  // Token matching is currently behind a flag.
  base::CommandLine::ForCurrentProcess()->AppendSwitch(
      autofill::switches::kEnableSuggestionsWithSubstringMatch);

  std::unique_ptr<TestPasswordManagerClient> client(
      new TestPasswordManagerClient);
  std::unique_ptr<MockAutofillClient> autofill_client(new MockAutofillClient);
  InitializePasswordAutofillManager(client.get(), autofill_client.get());

  gfx::RectF element_bounds;
  autofill::PasswordFormFillData data;
  base::string16 username = base::ASCIIToUTF16("foo.bar@example.com");
  data.username_field.value = username;
  data.password_field.value = base::ASCIIToUTF16("foobar");
  data.preferred_realm = "http://foo.com/";

  autofill::PasswordAndRealm additional;
  additional.realm = "https://foobarrealm.org";
  base::string16 additional_username(base::ASCIIToUTF16("bar.foo@example.com"));
  data.additional_logins[additional_username] = additional;

  autofill::UsernamesCollectionKey usernames_key;
  usernames_key.realm = "http://yetanother.net";

  int dummy_key = 0;
  password_autofill_manager_->OnAddPasswordFormMapping(dummy_key, data);

  EXPECT_CALL(*autofill_client, ShowAutofillPopup(_, _, _, _)).Times(0);

  password_autofill_manager_->OnShowPasswordSuggestions(
      dummy_key, base::i18n::RIGHT_TO_LEFT, base::ASCIIToUTF16("oo"), false,
      element_bounds);
}

// Verify that typing "foo@exam" into the username field will match username
// "bar.foo@example.com" even if the field contents span accross multiple
// tokens.
TEST_F(PasswordAutofillManagerTest,
       MatchingContentsWithSuggestionTokenSeparator) {
  // Token matching is currently behind a flag.
  base::CommandLine::ForCurrentProcess()->AppendSwitch(
      autofill::switches::kEnableSuggestionsWithSubstringMatch);

  std::unique_ptr<TestPasswordManagerClient> client(
      new TestPasswordManagerClient);
  std::unique_ptr<MockAutofillClient> autofill_client(new MockAutofillClient);
  InitializePasswordAutofillManager(client.get(), autofill_client.get());

  gfx::RectF element_bounds;
  autofill::PasswordFormFillData data;
  base::string16 username = base::ASCIIToUTF16("foo.bar@example.com");
  data.username_field.value = username;
  data.password_field.value = base::ASCIIToUTF16("foobar");
  data.preferred_realm = "http://foo.com/";

  autofill::PasswordAndRealm additional;
  additional.realm = "https://foobarrealm.org";
  base::string16 additional_username(base::ASCIIToUTF16("bar.foo@example.com"));
  data.additional_logins[additional_username] = additional;

  autofill::UsernamesCollectionKey usernames_key;
  usernames_key.realm = "http://yetanother.net";

  int dummy_key = 0;
  password_autofill_manager_->OnAddPasswordFormMapping(dummy_key, data);

  EXPECT_CALL(
      *autofill_client,
      ShowAutofillPopup(element_bounds, _,
                        SuggestionVectorValuesAre(
                            testing::UnorderedElementsAre(additional_username)),
                        _));
  password_autofill_manager_->OnShowPasswordSuggestions(
      dummy_key, base::i18n::RIGHT_TO_LEFT, base::ASCIIToUTF16("foo@exam"),
      false, element_bounds);
}

// Verify that typing "example" into the username field will match and order
// usernames "example@foo.com", "foo.bar@example.com" and "bar.foo@example.com"
// i.e. prefix matched followed by substring matched.
TEST_F(PasswordAutofillManagerTest,
       DisplaySuggestionsWithPrefixesPrecedeSubstringMatched) {
  // Token matching is currently behind a flag.
  base::CommandLine::ForCurrentProcess()->AppendSwitch(
      autofill::switches::kEnableSuggestionsWithSubstringMatch);

  std::unique_ptr<TestPasswordManagerClient> client(
      new TestPasswordManagerClient);
  std::unique_ptr<MockAutofillClient> autofill_client(new MockAutofillClient);
  InitializePasswordAutofillManager(client.get(), autofill_client.get());

  gfx::RectF element_bounds;
  autofill::PasswordFormFillData data;
  base::string16 username = base::ASCIIToUTF16("foo.bar@example.com");
  data.username_field.value = username;
  data.password_field.value = base::ASCIIToUTF16("foobar");
  data.preferred_realm = "http://foo.com/";

  autofill::PasswordAndRealm additional;
  additional.realm = "https://foobarrealm.org";
  base::string16 additional_username(base::ASCIIToUTF16("bar.foo@example.com"));
  data.additional_logins[additional_username] = additional;

  autofill::UsernamesCollectionKey usernames_key;
  usernames_key.realm = "http://yetanother.net";

  int dummy_key = 0;
  password_autofill_manager_->OnAddPasswordFormMapping(dummy_key, data);

  EXPECT_CALL(
      *autofill_client,
      ShowAutofillPopup(element_bounds, _,
                        SuggestionVectorValuesAre(testing::UnorderedElementsAre(
                            username, additional_username)),
                        _));
  password_autofill_manager_->OnShowPasswordSuggestions(
      dummy_key, base::i18n::RIGHT_TO_LEFT, base::ASCIIToUTF16("foo"), false,
      element_bounds);
}

TEST_F(PasswordAutofillManagerTest, PreviewAndFillEmptyUsernameSuggestion) {
  // Initialize PasswordAutofillManager with credentials without username.
  std::unique_ptr<TestPasswordManagerClient> client(
      new TestPasswordManagerClient);
  std::unique_ptr<MockAutofillClient> autofill_client(new MockAutofillClient);
  fill_data().username_field.value.clear();
  InitializePasswordAutofillManager(client.get(), autofill_client.get());

  base::string16 no_username_string =
      l10n_util::GetStringUTF16(IDS_PASSWORD_MANAGER_EMPTY_LOGIN);

  // Simulate that the user clicks on a username field.
  EXPECT_CALL(*autofill_client, ShowAutofillPopup(_, _, _, _));
  gfx::RectF element_bounds;
  password_autofill_manager_->OnShowPasswordSuggestions(
      fill_data_id(), base::i18n::RIGHT_TO_LEFT, base::string16(), false,
      element_bounds);

  // Check that preview of the empty username works.
  EXPECT_CALL(*client->mock_driver(),
              PreviewSuggestion(base::string16(), test_password_));
  password_autofill_manager_->DidSelectSuggestion(no_username_string,
                                                  0 /*not used*/);
  testing::Mock::VerifyAndClearExpectations(client->mock_driver());

  // Check that fill of the empty username works.
  EXPECT_CALL(*client->mock_driver(),
              FillSuggestion(base::string16(), test_password_));
  EXPECT_CALL(*autofill_client, HideAutofillPopup());
  password_autofill_manager_->DidAcceptSuggestion(
      no_username_string, autofill::POPUP_ITEM_ID_PASSWORD_ENTRY, 1);
  testing::Mock::VerifyAndClearExpectations(client->mock_driver());
}

// Tests that the "Show all passwords" suggestion isn't shown along with
// "Use password for" in the popup when the feature which controls its
// appearance is disabled.
TEST_F(PasswordAutofillManagerTest,
       NotShowAllPasswordsOptionOnPasswordFieldWhenFeatureDisabled) {
  auto client = std::make_unique<TestPasswordManagerClient>();
  auto autofill_client = std::make_unique<MockAutofillClient>();
  InitializePasswordAutofillManager(client.get(), autofill_client.get());

  gfx::RectF element_bounds;
  autofill::PasswordFormFillData data;
  data.username_field.value = test_username_;
  data.password_field.value = test_password_;
  data.origin = GURL("https://foo.test");

  int dummy_key = 0;
  password_autofill_manager_->OnAddPasswordFormMapping(dummy_key, data);

  // String "Use password for:" shown when displaying suggestions matching a
  // username and specifying that the field is a password field.
  base::string16 title =
      l10n_util::GetStringUTF16(IDS_AUTOFILL_PASSWORD_FIELD_SUGGESTIONS_TITLE);

  SetManualFallbacksForFilling(false);

  // No "Show all passwords row" when feature is disabled.
  EXPECT_CALL(*autofill_client,
              ShowAutofillPopup(element_bounds, _,
                                SuggestionVectorValuesAre(testing::ElementsAre(
                                    title, test_username_)),
                                _));
  password_autofill_manager_->OnShowPasswordSuggestions(
      dummy_key, base::i18n::RIGHT_TO_LEFT, test_username_,
      autofill::IS_PASSWORD_FIELD, element_bounds);
}

// Tests that the "Generate Password Suggestion" suggestion isn't shown in the
// Suggestion popup when the user is sync user with custom passphrase and manual
// fallbacks experiment is enabled.
TEST_F(PasswordAutofillManagerTest,
       NotShowGeneratePasswordOptionOnPasswordFieldWhenCustomPassphraseUser) {
  TestGenerationFallback(true /* custom_passphrase_enabled */);
}

// Tests that the "Generate Password Suggestion" suggestion is shown along
// with "Use password for" and "Show all passwords" in the popup for the user
// with custom passphrase.
TEST_F(PasswordAutofillManagerTest, ShowGeneratePasswordOptionOnPasswordField) {
  TestGenerationFallback(false /* custom_passphrase_enabled */);
}

// Tests that the "Show all passwords" suggestion is shown along with
// "Use password for" in the popup when the feature which controls its
// appearance is enabled.
TEST_F(PasswordAutofillManagerTest, ShowAllPasswordsOptionOnPasswordField) {
  const char kShownContextHistogram[] =
      "PasswordManager.ShowAllSavedPasswordsShownContext";
  const char kAcceptedContextHistogram[] =
      "PasswordManager.ShowAllSavedPasswordsAcceptedContext";
  base::HistogramTester histograms;
  ukm::TestAutoSetUkmRecorder test_ukm_recorder;

  auto client = std::make_unique<TestPasswordManagerClient>();
  auto autofill_client = std::make_unique<MockAutofillClient>();
  auto manager =
      std::make_unique<password_manager::PasswordManager>(client.get());
  InitializePasswordAutofillManager(client.get(), autofill_client.get());

  ON_CALL(*(client->mock_driver()), GetPasswordManager())
      .WillByDefault(testing::Return(manager.get()));

  gfx::RectF element_bounds;
  autofill::PasswordFormFillData data;
  data.username_field.value = test_username_;
  data.password_field.value = test_password_;
  data.origin = GURL("https://foo.test");

  int dummy_key = 0;
  password_autofill_manager_->OnAddPasswordFormMapping(dummy_key, data);

  // String "Use password for:" shown when displaying suggestions matching a
  // username and specifying that the field is a password field.
  base::string16 title =
      l10n_util::GetStringUTF16(IDS_AUTOFILL_PASSWORD_FIELD_SUGGESTIONS_TITLE);

  SetManualFallbacksForFilling(true);

  std::vector<base::string16> elements = {title, test_username_};
  if (!IsPreLollipopAndroid()) {
    elements = {
      title,
      test_username_,
#if !defined(OS_ANDROID)
      base::string16(),
#endif
      l10n_util::GetStringUTF16(IDS_AUTOFILL_SHOW_ALL_SAVED_FALLBACK)
    };
  }

  EXPECT_CALL(
      *autofill_client,
      ShowAutofillPopup(
          element_bounds, _,
          SuggestionVectorValuesAre(testing::ElementsAreArray(elements)), _));

  password_autofill_manager_->OnShowPasswordSuggestions(
      dummy_key, base::i18n::RIGHT_TO_LEFT, test_username_,
      autofill::IS_PASSWORD_FIELD, element_bounds);

  if (!IsPreLollipopAndroid()) {
    // Expect a sample only in the shown histogram.
    histograms.ExpectUniqueSample(
        kShownContextHistogram,
        metrics_util::SHOW_ALL_SAVED_PASSWORDS_CONTEXT_PASSWORD, 1);
    // Clicking at the "Show all passwords row" should trigger a call to open
    // the Password Manager settings page and hide the popup.
    EXPECT_CALL(
        *autofill_client,
        ExecuteCommand(autofill::POPUP_ITEM_ID_ALL_SAVED_PASSWORDS_ENTRY));
    EXPECT_CALL(*autofill_client, HideAutofillPopup());
    password_autofill_manager_->DidAcceptSuggestion(
        base::string16(), autofill::POPUP_ITEM_ID_ALL_SAVED_PASSWORDS_ENTRY, 0);
    // Expect a sample in both the shown and accepted histogram.
    histograms.ExpectUniqueSample(
        kShownContextHistogram,
        metrics_util::SHOW_ALL_SAVED_PASSWORDS_CONTEXT_PASSWORD, 1);
    histograms.ExpectUniqueSample(
        kAcceptedContextHistogram,
        metrics_util::SHOW_ALL_SAVED_PASSWORDS_CONTEXT_PASSWORD, 1);
    // Trigger UKM reporting, which happens at destruction time.
    ukm::SourceId expected_source_id = client->GetUkmSourceId();
    manager.reset();
    autofill_client.reset();
    client.reset();

    const auto& entries =
        test_ukm_recorder.GetEntriesByName(UkmEntry::kEntryName);
    EXPECT_EQ(1u, entries.size());
    for (const auto* entry : entries) {
      EXPECT_EQ(expected_source_id, entry->source_id);
      test_ukm_recorder.ExpectEntryMetric(
          entry, UkmEntry::kPageLevelUserActionName,
          static_cast<int64_t>(
              password_manager::PasswordManagerMetricsRecorder::
                  PageLevelUserAction::kShowAllPasswordsWhileSomeAreSuggested));
    }
  } else {
    EXPECT_THAT(histograms.GetAllSamples(kShownContextHistogram),
                testing::IsEmpty());
    EXPECT_THAT(histograms.GetAllSamples(kAcceptedContextHistogram),
                testing::IsEmpty());
  }
}

TEST_F(PasswordAutofillManagerTest, ShowStandaloneShowAllPasswords) {
  const char kShownContextHistogram[] =
      "PasswordManager.ShowAllSavedPasswordsShownContext";
  const char kAcceptedContextHistogram[] =
      "PasswordManager.ShowAllSavedPasswordsAcceptedContext";
  base::HistogramTester histograms;
  ukm::TestAutoSetUkmRecorder test_ukm_recorder;

  auto client = std::make_unique<TestPasswordManagerClient>();
  auto autofill_client = std::make_unique<MockAutofillClient>();
  auto manager =
      std::make_unique<password_manager::PasswordManager>(client.get());
  InitializePasswordAutofillManager(client.get(), autofill_client.get());

  ON_CALL(*(client->mock_driver()), GetPasswordManager())
      .WillByDefault(testing::Return(manager.get()));

  gfx::RectF element_bounds;
  autofill::PasswordFormFillData data;
  data.username_field.value = test_username_;
  data.password_field.value = test_password_;
  data.origin = GURL("http://foo.test");

  // String for the "Show all passwords" fallback.
  base::string16 show_all_saved_row_text =
      l10n_util::GetStringUTF16(IDS_AUTOFILL_SHOW_ALL_SAVED_FALLBACK);

  SetManualFallbacksForFillingStandalone(true);

  EXPECT_CALL(
      *autofill_client,
      ShowAutofillPopup(element_bounds, _,
                        SuggestionVectorValuesAre(testing::ElementsAreArray(
                            {show_all_saved_row_text})),
                        _))
      .Times(IsPreLollipopAndroid() ? 0 : 1);
  password_autofill_manager_->OnShowManualFallbackSuggestion(
      base::i18n::RIGHT_TO_LEFT, element_bounds);

  if (IsPreLollipopAndroid()) {
    EXPECT_THAT(histograms.GetAllSamples(kShownContextHistogram),
                testing::IsEmpty());
  } else {
    // Expect a sample only in the shown histogram.
    histograms.ExpectUniqueSample(
        kShownContextHistogram,
        metrics_util::SHOW_ALL_SAVED_PASSWORDS_CONTEXT_MANUAL_FALLBACK, 1);
  }

  if (!IsPreLollipopAndroid()) {
    // Clicking at the "Show all passwords row" should trigger a call to open
    // the Password Manager settings page and hide the popup.
    EXPECT_CALL(
        *autofill_client,
        ExecuteCommand(autofill::POPUP_ITEM_ID_ALL_SAVED_PASSWORDS_ENTRY));
    EXPECT_CALL(*autofill_client, HideAutofillPopup());
    password_autofill_manager_->DidAcceptSuggestion(
        base::string16(), autofill::POPUP_ITEM_ID_ALL_SAVED_PASSWORDS_ENTRY, 0);
    // Expect a sample in both the shown and accepted histogram.
    histograms.ExpectUniqueSample(
        kShownContextHistogram,
        metrics_util::SHOW_ALL_SAVED_PASSWORDS_CONTEXT_MANUAL_FALLBACK, 1);
    histograms.ExpectUniqueSample(
        kAcceptedContextHistogram,
        metrics_util::SHOW_ALL_SAVED_PASSWORDS_CONTEXT_MANUAL_FALLBACK, 1);
    // Trigger UKM reporting, which happens at destruction time.
    ukm::SourceId expected_source_id = client->GetUkmSourceId();
    manager.reset();
    autofill_client.reset();
    client.reset();

    const auto& entries =
        test_ukm_recorder.GetEntriesByName("PageWithPassword");
    EXPECT_EQ(1u, entries.size());
    for (const auto* entry : entries) {
      EXPECT_EQ(expected_source_id, entry->source_id);
      test_ukm_recorder.ExpectEntryMetric(
          entry, UkmEntry::kPageLevelUserActionName,
          static_cast<int64_t>(
              password_manager::PasswordManagerMetricsRecorder::
                  PageLevelUserAction::kShowAllPasswordsWhileNoneAreSuggested));
    }
  } else {
    EXPECT_THAT(histograms.GetAllSamples(kShownContextHistogram),
                testing::IsEmpty());
    EXPECT_THAT(histograms.GetAllSamples(kAcceptedContextHistogram),
                testing::IsEmpty());
  }
}

// Tests that the "Show all passwords" fallback doesn't shows up in non-password
// fields of login forms.
TEST_F(PasswordAutofillManagerTest,
       NotShowAllPasswordsOptionOnNonPasswordField) {
  auto client = std::make_unique<TestPasswordManagerClient>();
  auto autofill_client = std::make_unique<MockAutofillClient>();
  InitializePasswordAutofillManager(client.get(), autofill_client.get());

  gfx::RectF element_bounds;
  autofill::PasswordFormFillData data;
  data.username_field.value = test_username_;
  data.password_field.value = test_password_;
  data.origin = GURL("https://foo.test");

  int dummy_key = 0;
  password_autofill_manager_->OnAddPasswordFormMapping(dummy_key, data);

  SetManualFallbacksForFilling(true);

  EXPECT_CALL(
      *autofill_client,
      ShowAutofillPopup(
          element_bounds, _,
          SuggestionVectorValuesAre(testing::ElementsAre(test_username_)), _));
  password_autofill_manager_->OnShowPasswordSuggestions(
      dummy_key, base::i18n::RIGHT_TO_LEFT, test_username_, 0, element_bounds);
}

// SimpleWebviewDialog doesn't have an autofill client. Nothing should crash if
// the filling fallback is invoked.
TEST_F(PasswordAutofillManagerTest, ShowAllPasswordsWithoutAutofillClient) {
  auto client = std::make_unique<TestPasswordManagerClient>();
  InitializePasswordAutofillManager(client.get(), nullptr);

  SetManualFallbacksForFillingStandalone(true);

  password_autofill_manager_->OnShowManualFallbackSuggestion(
      base::i18n::RIGHT_TO_LEFT, gfx::RectF());
}

}  // namespace password_manager
