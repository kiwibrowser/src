// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/password_manager/core/browser/suppressed_form_fetcher.h"

#include "base/macros.h"
#include "base/message_loop/message_loop.h"
#include "components/password_manager/core/browser/mock_password_store.h"
#include "components/password_manager/core/browser/password_manager_test_utils.h"
#include "components/password_manager/core/browser/stub_password_manager_client.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace password_manager {
namespace {

using autofill::PasswordForm;
using testing::_;

constexpr const char kTestHttpURL[] = "http://one.example.com/";
constexpr const char kTestHttpsURL[] = "https://one.example.com/";
constexpr const char kTestPSLMatchingHttpURL[] = "http://psl.example.com/";
constexpr const char kTestPSLMatchingHttpsURL[] = "https://psl.example.com/";
constexpr const char kTestHttpSameOrgNameURL[] = "http://login.example.co.uk/";
constexpr const char kTestHttpsSameOrgNameURL[] =
    "https://login.example.co.uk/";

class MockConsumer : public SuppressedFormFetcher::Consumer {
 public:
  MockConsumer() = default;
  ~MockConsumer() = default;

  // GMock still cannot mock methods with move-only args.
  MOCK_METHOD1(ProcessSuppressedHTTPSFormsConstRef,
               void(const std::vector<std::unique_ptr<PasswordForm>>&));

 protected:
  // SuppressedFormFetcher::Consumer:
  void ProcessSuppressedForms(
      std::vector<std::unique_ptr<PasswordForm>> forms) override {
    ProcessSuppressedHTTPSFormsConstRef(forms);
  }

 private:
  DISALLOW_COPY_AND_ASSIGN(MockConsumer);
};

class PasswordManagerClientWithMockStore : public StubPasswordManagerClient {
 public:
  PasswordManagerClientWithMockStore()
      : mock_store_(new ::testing::StrictMock<MockPasswordStore>()) {}
  ~PasswordManagerClientWithMockStore() override {
    mock_store_->ShutdownOnUIThread();
  }

  MockPasswordStore& mock_password_store() const { return *mock_store_; }

 protected:
  // StubPasswordManagerClient:
  PasswordStore* GetPasswordStore() const override { return mock_store_.get(); }

 private:
  scoped_refptr<MockPasswordStore> mock_store_;

  DISALLOW_COPY_AND_ASSIGN(PasswordManagerClientWithMockStore);
};

}  // namespace

class SuppressedFormFetcherTest : public testing::Test {
 public:
  SuppressedFormFetcherTest() = default;
  ~SuppressedFormFetcherTest() override = default;

  MockConsumer* mock_consumer() { return &consumer_; }
  MockPasswordStore* mock_store() { return &client_.mock_password_store(); }
  PasswordManagerClientWithMockStore* mock_client() { return &client_; }

 private:
  base::MessageLoop message_loop_;  // Needed by the MockPasswordStore.

  MockConsumer consumer_;
  PasswordManagerClientWithMockStore client_;

  DISALLOW_COPY_AND_ASSIGN(SuppressedFormFetcherTest);
};

TEST_F(SuppressedFormFetcherTest, EmptyStore) {
  EXPECT_CALL(*mock_store(), GetLoginsForSameOrganizationName(kTestHttpURL, _));
  SuppressedFormFetcher suppressed_form_fetcher(kTestHttpURL, mock_client(),
                                                mock_consumer());
  EXPECT_CALL(*mock_consumer(),
              ProcessSuppressedHTTPSFormsConstRef(::testing::IsEmpty()));
  suppressed_form_fetcher.OnGetPasswordStoreResults(
      std::vector<std::unique_ptr<PasswordForm>>());
}

TEST_F(SuppressedFormFetcherTest, FullStore) {
  static constexpr const PasswordFormData kSuppressedCredentials[] = {
      // Credential that is for the HTTPS counterpart of the observed form.
      {PasswordForm::SCHEME_HTML, kTestHttpsURL, kTestHttpsURL, "", L"", L"",
       L"", L"username_value_1", L"password_value_1", true, 1},
      // Once again, but with a different username/password.
      {PasswordForm::SCHEME_HTML, kTestHttpsURL, kTestHttpsURL, "", L"", L"",
       L"", L"username_value_2", L"password_value_2", true, 1},
      // A PSL match to the observed form.
      {PasswordForm::SCHEME_HTML, kTestPSLMatchingHttpURL,
       kTestPSLMatchingHttpURL, "", L"", L"", L"", L"username_value_2",
       L"password_value_2", true, 1},
      // A PSL match to the HTTPS counterpart of the observed form. Note that
      // this is *not* a PSL match to the observed form, but a same organization
      // name match.
      {PasswordForm::SCHEME_HTML, kTestPSLMatchingHttpsURL,
       kTestPSLMatchingHttpsURL, "", L"", L"", L"", L"username_value_3",
       L"password_value_3", true, 1},
      // Credentials for a HTTP origin with the same organization
      // identifying name.
      {PasswordForm::SCHEME_HTML, kTestHttpSameOrgNameURL,
       kTestHttpSameOrgNameURL, "", L"", L"", L"", L"username_value_4",
       L"password_value_4", true, 1},
      // Credentials for a HTTPS origin with the same organization
      // identifying name.
      {PasswordForm::SCHEME_HTML, kTestHttpsSameOrgNameURL,
       kTestHttpsSameOrgNameURL, "", L"", L"", L"", L"username_value_5",
       L"password_value_5", true, 1}};

  static const PasswordFormData kNotSuppressedCredentials[] = {
      // Credential exactly matching the observed form.
      {PasswordForm::SCHEME_HTML, kTestHttpURL, kTestHttpURL, "", L"", L"", L"",
       L"username_value_1", L"password_value_1", true, 1},
  };

  std::vector<std::unique_ptr<PasswordForm>> simulated_store_results;
  std::vector<std::unique_ptr<PasswordForm>> expected_results;
  for (const auto& form_data : kSuppressedCredentials) {
    expected_results.push_back(FillPasswordFormWithData(form_data));
    simulated_store_results.push_back(FillPasswordFormWithData(form_data));
  }
  for (const auto& form_data : kNotSuppressedCredentials) {
    simulated_store_results.push_back(FillPasswordFormWithData(form_data));
  }

  EXPECT_CALL(*mock_store(), GetLoginsForSameOrganizationName(kTestHttpURL, _));
  SuppressedFormFetcher suppressed_form_fetcher(kTestHttpURL, mock_client(),
                                                mock_consumer());
  EXPECT_CALL(*mock_consumer(),
              ProcessSuppressedHTTPSFormsConstRef(
                  UnorderedPasswordFormElementsAre(&expected_results)));
  suppressed_form_fetcher.OnGetPasswordStoreResults(
      std::move(simulated_store_results));
}

}  // namespace password_manager
