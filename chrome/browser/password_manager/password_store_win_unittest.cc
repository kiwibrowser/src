// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/password_manager/password_store_win.h"

#include <memory>
#include <string>
#include <vector>

#include "base/bind.h"
#include "base/bind_helpers.h"
#include "base/files/scoped_temp_dir.h"
#include "base/location.h"
#include "base/macros.h"
#include "base/task_scheduler/post_task.h"
#include "base/test/histogram_tester.h"
#include "base/threading/sequenced_task_runner_handle.h"
#include "base/threading/thread_task_runner_handle.h"
#include "base/time/time.h"
#include "chrome/test/base/testing_profile.h"
#include "components/os_crypt/ie7_password_win.h"
#include "components/password_manager/core/browser/password_manager_metrics_util.h"
#include "components/password_manager/core/browser/password_manager_test_utils.h"
#include "components/password_manager/core/browser/password_store.h"
#include "components/password_manager/core/browser/password_store_consumer.h"
#include "components/password_manager/core/browser/webdata/logins_table.h"
#include "components/password_manager/core/browser/webdata/password_web_data_service_win.h"
#include "components/webdata/common/web_database_service.h"
#include "content/public/test/test_browser_thread_bundle.h"
#include "content/public/test/test_utils.h"
#include "crypto/wincrypt_shim.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

using autofill::PasswordForm;
using password_manager::LoginDatabase;
using password_manager::PasswordFormData;
using password_manager::PasswordStore;
using password_manager::PasswordStoreConsumer;
using password_manager::UnorderedPasswordFormElementsAre;
using testing::_;
using testing::DoAll;
using testing::IsEmpty;
using testing::WithArg;

namespace {

class MockPasswordStoreConsumer : public PasswordStoreConsumer {
 public:
  MOCK_METHOD1(OnGetPasswordStoreResultsConstRef,
               void(const std::vector<std::unique_ptr<PasswordForm>>&));

  // GMock cannot mock methods with move-only args.
  void OnGetPasswordStoreResults(
      std::vector<std::unique_ptr<PasswordForm>> results) override {
    OnGetPasswordStoreResultsConstRef(results);
  }
};

class MockWebDataServiceConsumer : public WebDataServiceConsumer {
 public:
  MOCK_METHOD0(OnWebDataServiceRequestDoneStub, void());

  // GMock cannot mock methods with move-only args.
  void OnWebDataServiceRequestDone(WebDataServiceBase::Handle h,
                                   std::unique_ptr<WDTypedResult> result) {
    OnWebDataServiceRequestDoneStub();
  }
};

}  // anonymous namespace

class PasswordStoreWinTest : public testing::Test {
 protected:
  PasswordStoreWinTest() {}

  bool CreateIE7PasswordInfo(const std::wstring& url,
                             const base::Time& created,
                             IE7PasswordInfo* info) {
    // Copied from chrome/browser/importer/importer_unittest.cc
    // The username is "abcdefgh" and the password "abcdefghijkl".
    unsigned char data[] =
        "\x0c\x00\x00\x00\x38\x00\x00\x00\x2c\x00\x00\x00"
        "\x57\x49\x43\x4b\x18\x00\x00\x00\x02\x00\x00\x00"
        "\x67\x00\x72\x00\x01\x00\x00\x00\x00\x00\x00\x00"
        "\x00\x00\x00\x00\x4e\xfa\x67\x76\x22\x94\xc8\x01"
        "\x08\x00\x00\x00\x12\x00\x00\x00\x4e\xfa\x67\x76"
        "\x22\x94\xc8\x01\x0c\x00\x00\x00\x61\x00\x62\x00"
        "\x63\x00\x64\x00\x65\x00\x66\x00\x67\x00\x68\x00"
        "\x00\x00\x61\x00\x62\x00\x63\x00\x64\x00\x65\x00"
        "\x66\x00\x67\x00\x68\x00\x69\x00\x6a\x00\x6b\x00"
        "\x6c\x00\x00\x00";
    DATA_BLOB input = {0};
    DATA_BLOB url_key = {0};
    DATA_BLOB output = {0};

    input.pbData = data;
    input.cbData = sizeof(data);

    url_key.pbData =
        reinterpret_cast<unsigned char*>(const_cast<wchar_t*>(url.data()));
    url_key.cbData =
        static_cast<DWORD>((url.size() + 1) * sizeof(std::wstring::value_type));

    if (!CryptProtectData(&input, nullptr, &url_key, nullptr, nullptr,
                          CRYPTPROTECT_UI_FORBIDDEN, &output))
      return false;

    std::vector<unsigned char> encrypted_data;
    encrypted_data.resize(output.cbData);
    memcpy(&encrypted_data.front(), output.pbData, output.cbData);

    LocalFree(output.pbData);

    info->url_hash = ie7_password::GetUrlHash(url);
    info->encrypted_data = encrypted_data;
    info->date_created = created;

    return true;
  }

  void SetUp() override {
    ASSERT_TRUE(temp_dir_.CreateUniqueTempDir());

    profile_.reset(new TestingProfile());

    base::FilePath path = temp_dir_.GetPath().AppendASCII("web_data_test");
    // TODO(pkasting): http://crbug.com/740773 This should likely be sequenced,
    // not single-threaded.
    auto db_task_runner =
        base::CreateSingleThreadTaskRunnerWithTraits({base::MayBlock()});
    wdbs_ = new WebDatabaseService(path, base::ThreadTaskRunnerHandle::Get(),
                                   db_task_runner);
    // Need to add at least one table so the database gets created.
    wdbs_->AddTable(std::unique_ptr<WebDatabaseTable>(new LoginsTable()));
    wdbs_->LoadDatabase();
    wds_ =
        new PasswordWebDataService(wdbs_, base::ThreadTaskRunnerHandle::Get(),
                                   WebDataServiceBase::ProfileErrorCallback());
    wds_->Init();
  }

  void TearDown() override {
    if (store_.get())
      store_->ShutdownOnUIThread();
    if (wds_) {
      wds_->ShutdownOnUISequence();
      wds_ = nullptr;
    }
    if (wdbs_) {
      wdbs_->ShutdownDatabase();
      wdbs_ = nullptr;
    }
    content::RunAllTasksUntilIdle();
  }

  base::FilePath test_login_db_file_path() const {
    return temp_dir_.GetPath().Append(FILE_PATH_LITERAL("login_test"));
  }

  PasswordStoreWin* CreatePasswordStore() {
    return new PasswordStoreWin(
        std::make_unique<LoginDatabase>(test_login_db_file_path()), wds_.get());
  }

  content::TestBrowserThreadBundle test_browser_thread_bundle_;

  base::ScopedTempDir temp_dir_;
  std::unique_ptr<TestingProfile> profile_;
  scoped_refptr<PasswordWebDataService> wds_;
  scoped_refptr<WebDatabaseService> wdbs_;
  scoped_refptr<PasswordStore> store_;
};

MATCHER(EmptyWDResult, "") {
  return static_cast<
             const WDResult<std::vector<std::unique_ptr<PasswordForm>>>*>(arg)
      ->GetValue()
      .empty();
}

TEST_F(PasswordStoreWinTest, ReportIE7NoImport) {
  base::HistogramTester histogram_tester;

  store_ = CreatePasswordStore();
  EXPECT_TRUE(store_->Init(syncer::SyncableService::StartSyncFlare(), nullptr));

  MockPasswordStoreConsumer consumer;

  PasswordStore::FormDigest observed_form(PasswordForm::SCHEME_HTML,
                                          "http://example.com/origin",
                                          GURL("http://example.com/origin"));

  EXPECT_CALL(consumer, OnGetPasswordStoreResultsConstRef(_));
  store_->GetLogins(observed_form, &consumer);
  content::RunAllTasksUntilIdle();
  histogram_tester.ExpectUniqueSample(
      "PasswordManager.IE7LookupResult",
      password_manager::metrics_util::IE7_RESULTS_ABSENT, 1);
}

TEST_F(PasswordStoreWinTest, ReportIE7Import) {
  base::HistogramTester histogram_tester;

  IE7PasswordInfo password_info;
  ASSERT_TRUE(CreateIE7PasswordInfo(L"http://example.com/origin",
                                    base::Time::FromDoubleT(1),
                                    &password_info));
  // This IE7 password will be retrieved by the GetLogins call.
  wds_->AddIE7Login(password_info);

  store_ = CreatePasswordStore();
  EXPECT_TRUE(store_->Init(syncer::SyncableService::StartSyncFlare(), nullptr));

  MockPasswordStoreConsumer consumer;

  PasswordStore::FormDigest observed_form(PasswordForm::SCHEME_HTML,
                                          "http://example.com/origin",
                                          GURL("http://example.com/origin"));

  EXPECT_CALL(consumer, OnGetPasswordStoreResultsConstRef(_));
  store_->GetLogins(observed_form, &consumer);
  content::RunAllTasksUntilIdle();
  histogram_tester.ExpectUniqueSample(
      "PasswordManager.IE7LookupResult",
      password_manager::metrics_util::IE7_RESULTS_PRESENT, 1);
}

TEST_F(PasswordStoreWinTest, ConvertIE7Login) {
  IE7PasswordInfo password_info;
  ASSERT_TRUE(CreateIE7PasswordInfo(L"http://example.com/origin",
                                    base::Time::FromDoubleT(1),
                                    &password_info));
  // Verify the URL hash
  ASSERT_EQ(L"39471418FF5453FEEB3731E382DEB5D53E14FAF9B5",
            password_info.url_hash);

  // This IE7 password will be retrieved by the GetLogins call.
  wds_->AddIE7Login(password_info);

  store_ = CreatePasswordStore();
  EXPECT_TRUE(store_->Init(syncer::SyncableService::StartSyncFlare(), nullptr));

  MockPasswordStoreConsumer consumer;

  PasswordFormData form_data = {
      PasswordForm::SCHEME_HTML,
      "http://example.com/",
      "http://example.com/origin",
      "http://example.com/action",
      L"submit_element",
      L"username_element",
      L"password_element",
      nullptr,
      nullptr,
      true,
      1,
  };
  PasswordStore::FormDigest form(*FillPasswordFormWithData(form_data));

  // The returned form will not have 'action' or '*_element' fields set. This
  // is because credentials imported from IE don't have this information.
  PasswordFormData expected_form_data = {
      PasswordForm::SCHEME_HTML,
      "http://example.com/",
      "http://example.com/origin",
      nullptr,
      nullptr,
      nullptr,
      nullptr,
      L"abcdefgh",
      L"abcdefghijkl",
      true,
      1,
  };
  std::vector<std::unique_ptr<PasswordForm>> expected_forms;
  expected_forms.push_back(PasswordFormFromData(expected_form_data));

  // The IE7 password should be returned.
  EXPECT_CALL(consumer, OnGetPasswordStoreResultsConstRef(
                            UnorderedPasswordFormElementsAre(&expected_forms)));

  store_->GetLogins(form, &consumer);
  content::RunAllTasksUntilIdle();
}

TEST_F(PasswordStoreWinTest, OutstandingWDSQueries) {
  store_ = CreatePasswordStore();
  EXPECT_TRUE(store_->Init(syncer::SyncableService::StartSyncFlare(), nullptr));

  PasswordFormData form_data = {
      PasswordForm::SCHEME_HTML,
      "http://example.com/",
      "http://example.com/origin",
      "http://example.com/action",
      L"submit_element",
      L"username_element",
      L"password_element",
      nullptr,
      nullptr,
      true,
      1,
  };
  PasswordStore::FormDigest form(*FillPasswordFormWithData(form_data));

  MockPasswordStoreConsumer consumer;
  store_->GetLogins(form, &consumer);

  // Release the PSW and the WDS before the query can return.
  store_->ShutdownOnUIThread();
  store_ = nullptr;
  wds_->ShutdownOnUISequence();
  wds_ = nullptr;
  wdbs_->ShutdownDatabase();
  wdbs_ = nullptr;

  content::RunAllTasksUntilIdle();
}

TEST_F(PasswordStoreWinTest, MultipleWDSQueriesOnDifferentSequences) {
  IE7PasswordInfo password_info;
  ASSERT_TRUE(CreateIE7PasswordInfo(L"http://example.com/origin",
                                    base::Time::FromDoubleT(1),
                                    &password_info));
  wds_->AddIE7Login(password_info);

  store_ = CreatePasswordStore();
  EXPECT_TRUE(store_->Init(syncer::SyncableService::StartSyncFlare(), nullptr));

  MockPasswordStoreConsumer password_consumer;

  PasswordFormData form_data = {
      PasswordForm::SCHEME_HTML,
      "http://example.com/",
      "http://example.com/origin",
      "http://example.com/action",
      L"submit_element",
      L"username_element",
      L"password_element",
      nullptr,
      nullptr,
      true,
      1,
  };
  PasswordStore::FormDigest form(*FillPasswordFormWithData(form_data));

  // The returned form will not have 'action' or '*_element' fields set. This
  // is because credentials imported from IE don't have this information.
  PasswordFormData expected_form_data = {
      PasswordForm::SCHEME_HTML,
      "http://example.com/",
      "http://example.com/origin",
      nullptr,
      nullptr,
      nullptr,
      nullptr,
      L"abcdefgh",
      L"abcdefghijkl",
      true,
      1,
  };
  std::vector<std::unique_ptr<PasswordForm>> expected_forms;
  expected_forms.push_back(PasswordFormFromData(expected_form_data));

  // The IE7 password should be returned.
  EXPECT_CALL(password_consumer,
              OnGetPasswordStoreResultsConstRef(
                  UnorderedPasswordFormElementsAre(&expected_forms)));

  store_->GetLogins(form, &password_consumer);

  MockWebDataServiceConsumer wds_consumer;
  EXPECT_CALL(wds_consumer, OnWebDataServiceRequestDoneStub());
  wds_->GetIE7Login(password_info, &wds_consumer);

  content::RunAllTasksUntilIdle();
}

TEST_F(PasswordStoreWinTest, EmptyLogins) {
  store_ = CreatePasswordStore();
  store_->Init(syncer::SyncableService::StartSyncFlare(), nullptr);

  PasswordFormData form_data = {
      PasswordForm::SCHEME_HTML,
      "http://example.com/",
      "http://example.com/origin",
      "http://example.com/action",
      L"submit_element",
      L"username_element",
      L"password_element",
      nullptr,
      nullptr,
      true,
      1,
  };
  PasswordStore::FormDigest form(*FillPasswordFormWithData(form_data));

  MockPasswordStoreConsumer consumer;
  EXPECT_CALL(consumer, OnGetPasswordStoreResultsConstRef(IsEmpty()));
  store_->GetLogins(form, &consumer);

  content::RunAllTasksUntilIdle();
}

TEST_F(PasswordStoreWinTest, EmptyBlacklistLogins) {
  store_ = CreatePasswordStore();
  store_->Init(syncer::SyncableService::StartSyncFlare(), nullptr);

  MockPasswordStoreConsumer consumer;
  EXPECT_CALL(consumer, OnGetPasswordStoreResultsConstRef(IsEmpty()));
  store_->GetBlacklistLogins(&consumer);

  content::RunAllTasksUntilIdle();
}

TEST_F(PasswordStoreWinTest, EmptyAutofillableLogins) {
  store_ = CreatePasswordStore();
  store_->Init(syncer::SyncableService::StartSyncFlare(), nullptr);

  MockPasswordStoreConsumer consumer;
  EXPECT_CALL(consumer, OnGetPasswordStoreResultsConstRef(IsEmpty()));
  store_->GetAutofillableLogins(&consumer);

  content::RunAllTasksUntilIdle();
}
