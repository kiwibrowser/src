// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/safe_browsing/protocol_manager.h"

#include <memory>
#include <vector>

#include "base/single_thread_task_runner.h"
#include "base/strings/stringprintf.h"
#include "base/test/scoped_mock_time_message_loop_task_runner.h"
#include "base/threading/thread_task_runner_handle.h"
#include "base/time/time.h"
#include "chrome/browser/safe_browsing/chunk.pb.h"
#include "components/safe_browsing/common/safe_browsing_prefs.h"
#include "components/safe_browsing/db/safebrowsing.pb.h"
#include "components/safe_browsing/db/util.h"
#include "content/public/test/test_browser_thread_bundle.h"
#include "google_apis/google_api_keys.h"
#include "net/base/escape.h"
#include "net/base/load_flags.h"
#include "net/base/net_errors.h"
#include "services/network/public/cpp/weak_wrapper_shared_url_loader_factory.h"
#include "services/network/test/test_url_loader_factory.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gmock_mutant.h"
#include "testing/gtest/include/gtest/gtest.h"

using base::Time;
using base::TimeDelta;
using testing::_;
using testing::Invoke;

namespace {

const char kUrlPrefix[] = "https://prefix.com/foo";
const char kBackupConnectUrlPrefix[] = "https://alt1-prefix.com/foo";
const char kBackupHttpUrlPrefix[] = "https://alt2-prefix.com/foo";
const char kBackupNetworkUrlPrefix[] = "https://alt3-prefix.com/foo";
const char kClient[] = "unittest";
const char kAppVer[] = "1.0";
const char kAdditionalQuery[] = "additional_query";
const char kUrlSuffix[] = "&ext=0";

const char kDefaultPhishList[] = "goog-phish-shavar";
const char kDefaultMalwareList[] = "goog-malware-shavar";

// Add-prefix chunk with single prefix.
const char kRawChunkPayload1[] = {
  '\0', '\0', '\0', '\x08',  // 32-bit payload length in network byte order.
  '\x08',                    // field 1, wire format varint
  '\x03',                    // chunk_number varint 3
  '\x22',                    // field 4, wire format length-delimited
  '\x04',                    // varint 4 length
  'a', 'b', 'c', 'd'         // 4-byte prefix
};
const std::string kChunkPayload1(kRawChunkPayload1, sizeof(kRawChunkPayload1));

// Add-prefix chunk_number 5 with single prefix.
const char kRawChunkPayload2[] = {
  '\0', '\0', '\0', '\x08',  // 32-bit payload length in network byte order.
  '\x08',                    // field 1, wire format varint
  '\x05',                    // chunk_number varint 5
  '\x22',                    // field 4, wire format length-delimited
  '\x04',                    // varint length 4
  'e', 'f', 'g', 'h'         // 4-byte prefix
};
const std::string kChunkPayload2(kRawChunkPayload2, sizeof(kRawChunkPayload2));

}  // namespace

namespace safe_browsing {

class SafeBrowsingProtocolManagerTest : public testing::Test {
 protected:
  SafeBrowsingProtocolManagerTest()
      : thread_bundle_(content::TestBrowserThreadBundle::Options::IO_MAINLOOP) {
  }

  ~SafeBrowsingProtocolManagerTest() override {}

  void SetUp() override {
    std::string key = google_apis::GetAPIKey();
    if (!key.empty()) {
      key_param_ = base::StringPrintf(
          "&key=%s",
          net::EscapeQueryParamValue(key, true).c_str());
    }

    test_shared_loader_factory_ =
        base::MakeRefCounted<network::WeakWrapperSharedURLLoaderFactory>(
            &test_url_loader_factory_);
    test_url_loader_factory_.SetInterceptor(base::Bind(
        &SafeBrowsingProtocolManagerTest::OnRequest, base::Unretained(this)));
  }

  std::unique_ptr<SafeBrowsingProtocolManager> CreateProtocolManager(
      SafeBrowsingProtocolManagerDelegate* delegate) {
    SafeBrowsingProtocolConfig config;
    config.client_name = kClient;
    config.url_prefix = kUrlPrefix;
    config.backup_connect_error_url_prefix = kBackupConnectUrlPrefix;
    config.backup_http_error_url_prefix = kBackupHttpUrlPrefix;
    config.backup_network_error_url_prefix = kBackupNetworkUrlPrefix;
    config.version = kAppVer;
    return std::unique_ptr<SafeBrowsingProtocolManager>(
        SafeBrowsingProtocolManager::Create(
            delegate, test_shared_loader_factory_, config));
  }

  void OnRequest(const network::ResourceRequest& request) {
    last_request_ = request;
  }

  std::string GetBodyFromRequest(const network::ResourceRequest& request) {
    auto body = request.request_body;
    if (!body)
      return std::string();

    CHECK_EQ(1u, body->elements()->size());
    auto& element = body->elements()->at(0);
    CHECK_EQ(network::DataElement::TYPE_BYTES, element.type());
    return std::string(element.bytes(), element.length());
  }

  void ValidateUpdateFetcherRequest(const std::string& expected_prefix,
                                    const std::string& expected_suffix) {
    EXPECT_EQ(net::LOAD_DISABLE_CACHE, last_request_.load_flags);

    std::string expected_lists(base::StringPrintf("%s;\n%s;\n",
                                                  kDefaultPhishList,
                                                  kDefaultMalwareList));
    EXPECT_EQ(expected_lists, GetBodyFromRequest(last_request_));
    EXPECT_EQ(GURL(expected_prefix +
                   "/downloads?client=unittest&appver=1.0"
                   "&pver=3.0" +
                   key_param_ + expected_suffix),
              last_request_.url);
  }

  void ValidateUpdateFetcherRequest() {
    ValidateUpdateFetcherRequest(kUrlPrefix, kUrlSuffix);
  }

  void ValidateRedirectFetcherRequest(const std::string& expected_url) {
    EXPECT_EQ(net::LOAD_DISABLE_CACHE, last_request_.load_flags);
    EXPECT_EQ("", GetBodyFromRequest(last_request_));
    EXPECT_EQ(GURL(expected_url), last_request_.url);
  }

  // Fakes BrowserThreads and the main MessageLoop.
  content::TestBrowserThreadBundle thread_bundle_;

  network::TestURLLoaderFactory test_url_loader_factory_;
  scoped_refptr<network::SharedURLLoaderFactory> test_shared_loader_factory_;
  network::ResourceRequest last_request_;

  // Replaces the main MessageLoop's TaskRunner with a TaskRunner on which time
  // is mocked to allow testing of things bound to timers below.
  base::ScopedMockTimeMessageLoopTaskRunner mock_time_task_runner_;

  std::string key_param_;
};

// Ensure that we respect section 5 of the SafeBrowsing protocol specification.
TEST_F(SafeBrowsingProtocolManagerTest, TestBackOffTimes) {
  std::unique_ptr<SafeBrowsingProtocolManager> pm(
      CreateProtocolManager(nullptr));

  pm->next_update_interval_ = TimeDelta::FromSeconds(1800);
  ASSERT_TRUE(pm->back_off_fuzz_ >= 0.0 && pm->back_off_fuzz_ <= 1.0);

  TimeDelta next;

  // No errors received so far.
  next = pm->GetNextUpdateInterval(false);
  EXPECT_EQ(next, TimeDelta::FromSeconds(1800));

  // 1 error.
  next = pm->GetNextUpdateInterval(true);
  EXPECT_EQ(next, TimeDelta::FromSeconds(60));

  // 2 errors.
  next = pm->GetNextUpdateInterval(true);
  EXPECT_TRUE(next >= TimeDelta::FromMinutes(30) &&
              next <= TimeDelta::FromMinutes(60));

  // 3 errors.
  next = pm->GetNextUpdateInterval(true);
  EXPECT_TRUE(next >= TimeDelta::FromMinutes(60) &&
              next <= TimeDelta::FromMinutes(120));

  // 4 errors.
  next = pm->GetNextUpdateInterval(true);
  EXPECT_TRUE(next >= TimeDelta::FromMinutes(120) &&
              next <= TimeDelta::FromMinutes(240));

  // 5 errors.
  next = pm->GetNextUpdateInterval(true);
  EXPECT_TRUE(next >= TimeDelta::FromMinutes(240) &&
              next <= TimeDelta::FromMinutes(480));

  // 6 errors, reached max backoff.
  next = pm->GetNextUpdateInterval(true);
  EXPECT_EQ(next, TimeDelta::FromMinutes(480));

  // 7 errors.
  next = pm->GetNextUpdateInterval(true);
  EXPECT_EQ(next, TimeDelta::FromMinutes(480));

  // Received a successful response.
  next = pm->GetNextUpdateInterval(false);
  EXPECT_EQ(next, TimeDelta::FromSeconds(1800));
}

TEST_F(SafeBrowsingProtocolManagerTest, TestChunkStrings) {
  std::unique_ptr<SafeBrowsingProtocolManager> pm(
      CreateProtocolManager(nullptr));

  // Add and Sub chunks.
  SBListChunkRanges phish(kDefaultPhishList);
  phish.adds = "1,4,6,8-20,99";
  phish.subs = "16,32,64-96";
  EXPECT_EQ(base::StringPrintf("%s;a:1,4,6,8-20,99:s:16,32,64-96\n",
                               kDefaultPhishList),
            FormatList(phish));

  // Add chunks only.
  phish.subs = "";
  EXPECT_EQ(base::StringPrintf("%s;a:1,4,6,8-20,99\n", kDefaultPhishList),
            FormatList(phish));

  // Sub chunks only.
  phish.adds = "";
  phish.subs = "16,32,64-96";
  EXPECT_EQ(base::StringPrintf("%s;s:16,32,64-96\n", kDefaultPhishList),
            FormatList(phish));

  // No chunks of either type.
  phish.adds = "";
  phish.subs = "";
  EXPECT_EQ(base::StringPrintf("%s;\n", kDefaultPhishList), FormatList(phish));
}

TEST_F(SafeBrowsingProtocolManagerTest, TestGetHashBackOffTimes) {
  std::unique_ptr<SafeBrowsingProtocolManager> pm(
      CreateProtocolManager(nullptr));

  // No errors or back off time yet.
  EXPECT_EQ(0U, pm->gethash_error_count_);
  EXPECT_TRUE(pm->next_gethash_time_.is_null());

  Time now = Time::Now();

  // 1 error.
  pm->HandleGetHashError(now);
  EXPECT_EQ(1U, pm->gethash_error_count_);
  TimeDelta margin = TimeDelta::FromSeconds(5);  // Fudge factor.
  Time future = now + TimeDelta::FromMinutes(1);
  EXPECT_TRUE(pm->next_gethash_time_ >= future - margin &&
              pm->next_gethash_time_ <= future + margin);

  // 2 errors.
  pm->HandleGetHashError(now);
  EXPECT_EQ(2U, pm->gethash_error_count_);
  EXPECT_TRUE(pm->next_gethash_time_ >= now + TimeDelta::FromMinutes(30));
  EXPECT_TRUE(pm->next_gethash_time_ <= now + TimeDelta::FromMinutes(60));

  // 3 errors.
  pm->HandleGetHashError(now);
  EXPECT_EQ(3U, pm->gethash_error_count_);
  EXPECT_TRUE(pm->next_gethash_time_ >= now + TimeDelta::FromMinutes(60));
  EXPECT_TRUE(pm->next_gethash_time_ <= now + TimeDelta::FromMinutes(120));

  // 4 errors.
  pm->HandleGetHashError(now);
  EXPECT_EQ(4U, pm->gethash_error_count_);
  EXPECT_TRUE(pm->next_gethash_time_ >= now + TimeDelta::FromMinutes(120));
  EXPECT_TRUE(pm->next_gethash_time_ <= now + TimeDelta::FromMinutes(240));

  // 5 errors.
  pm->HandleGetHashError(now);
  EXPECT_EQ(5U, pm->gethash_error_count_);
  EXPECT_TRUE(pm->next_gethash_time_ >= now + TimeDelta::FromMinutes(240));
  EXPECT_TRUE(pm->next_gethash_time_ <= now + TimeDelta::FromMinutes(480));

  // 6 errors, reached max backoff.
  pm->HandleGetHashError(now);
  EXPECT_EQ(6U, pm->gethash_error_count_);
  EXPECT_TRUE(pm->next_gethash_time_ == now + TimeDelta::FromMinutes(480));

  // 7 errors.
  pm->HandleGetHashError(now);
  EXPECT_EQ(7U, pm->gethash_error_count_);
  EXPECT_TRUE(pm->next_gethash_time_== now + TimeDelta::FromMinutes(480));
}

TEST_F(SafeBrowsingProtocolManagerTest, TestGetHashUrl) {
  std::unique_ptr<SafeBrowsingProtocolManager> pm(
      CreateProtocolManager(nullptr));

  EXPECT_EQ(
      "https://prefix.com/foo/gethash?client=unittest&appver=1.0&"
      "pver=3.0" +
          key_param_ + "&ext=0",
      pm->GetHashUrl(SBER_LEVEL_OFF).spec());

  pm->set_additional_query(kAdditionalQuery);
  EXPECT_EQ(
      "https://prefix.com/foo/gethash?client=unittest&appver=1.0&"
      "pver=3.0" +
          key_param_ + "&additional_query&ext=1",
      pm->GetHashUrl(SBER_LEVEL_LEGACY).spec());

  EXPECT_EQ(
      "https://prefix.com/foo/gethash?client=unittest&appver=1.0&"
      "pver=3.0" +
          key_param_ + "&additional_query&ext=2",
      pm->GetHashUrl(SBER_LEVEL_SCOUT).spec());
}

TEST_F(SafeBrowsingProtocolManagerTest, TestUpdateUrl) {
  std::unique_ptr<SafeBrowsingProtocolManager> pm(
      CreateProtocolManager(nullptr));

  EXPECT_EQ(
      "https://prefix.com/foo/downloads?client=unittest&appver=1.0&"
      "pver=3.0" +
          key_param_ + "&ext=1",
      pm->UpdateUrl(SBER_LEVEL_LEGACY).spec());

  EXPECT_EQ(
      "https://prefix.com/foo/downloads?client=unittest&appver=1.0&"
      "pver=3.0" +
          key_param_ + "&ext=2",
      pm->UpdateUrl(SBER_LEVEL_SCOUT).spec());

  pm->set_additional_query(kAdditionalQuery);
  EXPECT_EQ(
      "https://prefix.com/foo/downloads?client=unittest&appver=1.0&"
      "pver=3.0" +
          key_param_ + "&additional_query&ext=0",
      pm->UpdateUrl(SBER_LEVEL_OFF).spec());
}

TEST_F(SafeBrowsingProtocolManagerTest, TestNextChunkUrl) {
  std::unique_ptr<SafeBrowsingProtocolManager> pm(
      CreateProtocolManager(nullptr));

  std::string url_partial = "localhost:1234/foo/bar?foo";
  std::string url_http_full = "http://localhost:1234/foo/bar?foo";
  std::string url_https_full = "https://localhost:1234/foo/bar?foo";
  std::string url_https_no_query = "https://localhost:1234/foo/bar";

  EXPECT_EQ("https://localhost:1234/foo/bar?foo",
            pm->NextChunkUrl(url_partial).spec());
  EXPECT_EQ("http://localhost:1234/foo/bar?foo",
            pm->NextChunkUrl(url_http_full).spec());
  EXPECT_EQ("https://localhost:1234/foo/bar?foo",
            pm->NextChunkUrl(url_https_full).spec());
  EXPECT_EQ("https://localhost:1234/foo/bar",
            pm->NextChunkUrl(url_https_no_query).spec());

  pm->set_additional_query(kAdditionalQuery);
  EXPECT_EQ("https://localhost:1234/foo/bar?foo&additional_query",
            pm->NextChunkUrl(url_partial).spec());
  EXPECT_EQ("http://localhost:1234/foo/bar?foo&additional_query",
            pm->NextChunkUrl(url_http_full).spec());
  EXPECT_EQ("https://localhost:1234/foo/bar?foo&additional_query",
            pm->NextChunkUrl(url_https_full).spec());
  EXPECT_EQ("https://localhost:1234/foo/bar?additional_query",
            pm->NextChunkUrl(url_https_no_query).spec());
}

namespace {

class MockProtocolDelegate : public SafeBrowsingProtocolManagerDelegate {
 public:
  MockProtocolDelegate() {}
  ~MockProtocolDelegate() override {}

  MOCK_METHOD0(UpdateStarted, void());
  MOCK_METHOD1(UpdateFinished, void(bool));
  MOCK_METHOD0(ResetDatabase, void());
  MOCK_METHOD1(GetChunks, void(GetChunksCallback));

  // gmock does not work with std::unique_ptr<> at this time.  Add a local
  // method to
  // mock, then call that from an override.  Beware of object ownership when
  // making changes here.
  MOCK_METHOD3(AddChunksRaw,
               void(const std::string& lists,
                    const std::vector<std::unique_ptr<SBChunkData>>& chunks,
                    AddChunksCallback));
  void AddChunks(
      const std::string& list,
      std::unique_ptr<std::vector<std::unique_ptr<SBChunkData>>> chunks,
      AddChunksCallback callback) override {
    AddChunksRaw(list, *chunks, callback);
  }

  // TODO(shess): Actually test this case somewhere.
  MOCK_METHOD1(DeleteChunksRaw,
               void(const std::vector<SBChunkDelete>& chunk_deletes));
  void DeleteChunks(
      std::unique_ptr<std::vector<SBChunkDelete>> chunk_deletes) override {
    DeleteChunksRaw(*chunk_deletes);
  }
};

// |InvokeGetChunksCallback| is required because GMock's InvokeArgument action
// expects to use operator(), and a Callback only provides Run().
// TODO(cbentzel): Use ACTION or ACTION_TEMPLATE instead?
void InvokeGetChunksCallback(
    const std::vector<SBListChunkRanges>& ranges,
    bool database_error,
    SafeBrowsingProtocolManagerDelegate::GetChunksCallback callback) {
  callback.Run(ranges, database_error, SBER_LEVEL_OFF);
}

// |HandleAddChunks| deletes the chunks and asynchronously invokes
// |callback| since SafeBrowsingProtocolManager is not re-entrant at the time
// this is called. This guarantee is part of the
// SafeBrowsingProtocolManagerDelegate contract.
void HandleAddChunks(
    const std::string& unused_list,
    const std::vector<std::unique_ptr<SBChunkData>>& chunks,
    SafeBrowsingProtocolManagerDelegate::AddChunksCallback callback) {
  scoped_refptr<base::SingleThreadTaskRunner> task_runner(
      base::ThreadTaskRunnerHandle::Get());
  if (!task_runner.get())
    return;
  task_runner->PostTask(FROM_HERE, callback);
}

}  // namespace

// Tests that the Update protocol will be skipped if there are problems
// accessing the database.
TEST_F(SafeBrowsingProtocolManagerTest, ProblemAccessingDatabase) {
  testing::StrictMock<MockProtocolDelegate> test_delegate;
  EXPECT_CALL(test_delegate, UpdateStarted()).Times(1);
  EXPECT_CALL(test_delegate, GetChunks(_)).WillOnce(
      Invoke(testing::CreateFunctor(InvokeGetChunksCallback,
                                    std::vector<SBListChunkRanges>(),
                                    true)));
  EXPECT_CALL(test_delegate, UpdateFinished(false)).Times(1);

  std::unique_ptr<SafeBrowsingProtocolManager> pm(
      CreateProtocolManager(&test_delegate));

  pm->ForceScheduleNextUpdate(TimeDelta());
  mock_time_task_runner_->RunUntilIdle();

  EXPECT_TRUE(pm->IsUpdateScheduled());
}

// Tests the contents of the POST body when there are contents in the
// local database. This is not exhaustive, as the actual list formatting
// is covered by SafeBrowsingProtocolManagerTest.TestChunkStrings.
TEST_F(SafeBrowsingProtocolManagerTest, ExistingDatabase) {
  std::vector<SBListChunkRanges> ranges;
  SBListChunkRanges range_phish(kPhishingList);
  range_phish.adds = "adds_phish";
  range_phish.subs = "subs_phish";
  ranges.push_back(range_phish);

  SBListChunkRanges range_unknown("unknown_list");
  range_unknown.adds = "adds_unknown";
  range_unknown.subs = "subs_unknown";
  ranges.push_back(range_unknown);

  testing::StrictMock<MockProtocolDelegate> test_delegate;
  EXPECT_CALL(test_delegate, UpdateStarted()).Times(1);
  EXPECT_CALL(test_delegate, GetChunks(_)).WillOnce(
      Invoke(testing::CreateFunctor(InvokeGetChunksCallback,
                                    ranges,
                                    false)));
  EXPECT_CALL(test_delegate, UpdateFinished(true)).Times(1);

  std::unique_ptr<SafeBrowsingProtocolManager> pm(
      CreateProtocolManager(&test_delegate));

  // Kick off initialization. This returns chunks from the DB synchronously.
  pm->ForceScheduleNextUpdate(TimeDelta());
  mock_time_task_runner_->RunUntilIdle();

  EXPECT_EQ(net::LOAD_DISABLE_CACHE, last_request_.load_flags);
  EXPECT_EQ(base::StringPrintf("%s;a:adds_phish:s:subs_phish\n"
                               "unknown_list;a:adds_unknown:s:subs_unknown\n"
                               "%s;\n",
                               kDefaultPhishList, kDefaultMalwareList),
            GetBodyFromRequest(last_request_));
  EXPECT_EQ(GURL("https://prefix.com/foo/downloads?client=unittest&appver=1.0"
                 "&pver=3.0" +
                 key_param_ + "&ext=0"),
            last_request_.url);

  pm->OnURLLoaderCompleteInternal(nullptr, net::OK, 200, std::string());

  EXPECT_TRUE(pm->IsUpdateScheduled());
}

TEST_F(SafeBrowsingProtocolManagerTest, UpdateResponseBadBodyBackupSuccess) {
  testing::StrictMock<MockProtocolDelegate> test_delegate;
  EXPECT_CALL(test_delegate, UpdateStarted()).Times(1);
  EXPECT_CALL(test_delegate, GetChunks(_)).WillOnce(
      Invoke(testing::CreateFunctor(InvokeGetChunksCallback,
                                    std::vector<SBListChunkRanges>(),
                                    false)));
  EXPECT_CALL(test_delegate, UpdateFinished(true)).Times(1);

  std::unique_ptr<SafeBrowsingProtocolManager> pm(
      CreateProtocolManager(&test_delegate));

  // Kick off initialization. This returns chunks from the DB synchronously.
  pm->ForceScheduleNextUpdate(TimeDelta());
  mock_time_task_runner_->RunUntilIdle();

  // We should have an URLFetcher at this point in time.
  ValidateUpdateFetcherRequest();

  // The update response is successful, but an invalid body.
  pm->OnURLLoaderCompleteInternal(nullptr, net::OK, 200,
                                  "THIS_IS_A_BAD_RESPONSE");

  // There should now be a backup request.
  ValidateUpdateFetcherRequest(kBackupHttpUrlPrefix, "");

  // Respond to the backup successfully.
  pm->OnURLLoaderCompleteInternal(nullptr, net::OK, 200, std::string());

  EXPECT_TRUE(pm->IsUpdateScheduled());
}

// Tests what happens when there is an HTTP error response to the update
// request, as well as an error response to the backup update request.
TEST_F(SafeBrowsingProtocolManagerTest, UpdateResponseHttpErrorBackupError) {
  testing::StrictMock<MockProtocolDelegate> test_delegate;
  EXPECT_CALL(test_delegate, UpdateStarted()).Times(1);
  EXPECT_CALL(test_delegate, GetChunks(_)).WillOnce(
      Invoke(testing::CreateFunctor(InvokeGetChunksCallback,
                                    std::vector<SBListChunkRanges>(),
                                    false)));
  EXPECT_CALL(test_delegate, UpdateFinished(false)).Times(1);

  std::unique_ptr<SafeBrowsingProtocolManager> pm(
      CreateProtocolManager(&test_delegate));

  // Kick off initialization. This returns chunks from the DB synchronously.
  pm->ForceScheduleNextUpdate(TimeDelta());
  mock_time_task_runner_->RunUntilIdle();

  // We should have an URLFetcher at this point in time.
  ValidateUpdateFetcherRequest();

  // Go ahead and respond to it.
  pm->OnURLLoaderCompleteInternal(nullptr, net::OK, 404, std::string());

  // There should now be a backup request.
  ValidateUpdateFetcherRequest(kBackupHttpUrlPrefix, "");

  // Respond to the backup unsuccessfully.
  pm->OnURLLoaderCompleteInternal(nullptr, net::OK, 404, std::string());

  EXPECT_TRUE(pm->IsUpdateScheduled());
}

// Tests what happens when there is an HTTP error response to the update
// request, followed by a successful response to the backup update request.
TEST_F(SafeBrowsingProtocolManagerTest, UpdateResponseHttpErrorBackupSuccess) {
  testing::StrictMock<MockProtocolDelegate> test_delegate;
  EXPECT_CALL(test_delegate, UpdateStarted()).Times(1);
  EXPECT_CALL(test_delegate, GetChunks(_)).WillOnce(
      Invoke(testing::CreateFunctor(InvokeGetChunksCallback,
                                    std::vector<SBListChunkRanges>(),
                                    false)));
  EXPECT_CALL(test_delegate, UpdateFinished(true)).Times(1);

  std::unique_ptr<SafeBrowsingProtocolManager> pm(
      CreateProtocolManager(&test_delegate));

  // Kick off initialization. This returns chunks from the DB synchronously.
  pm->ForceScheduleNextUpdate(TimeDelta());
  mock_time_task_runner_->RunUntilIdle();

  // We should have an URLFetcher at this point in time.
  ValidateUpdateFetcherRequest();

  // Go ahead and respond to it.
  pm->OnURLLoaderCompleteInternal(nullptr, net::OK, 404, std::string());

  // There should now be a backup request.
  ValidateUpdateFetcherRequest(kBackupHttpUrlPrefix, "");

  // Respond to the backup successfully.
  pm->OnURLLoaderCompleteInternal(nullptr, net::OK, 200, std::string());

  EXPECT_TRUE(pm->IsUpdateScheduled());
}

// Tests what happens when there is an HTTP error response to the update
// request, and a timeout on the backup update request.
TEST_F(SafeBrowsingProtocolManagerTest, UpdateResponseHttpErrorBackupTimeout) {
  testing::StrictMock<MockProtocolDelegate> test_delegate;
  EXPECT_CALL(test_delegate, UpdateStarted()).Times(1);
  EXPECT_CALL(test_delegate, GetChunks(_)).WillOnce(
      Invoke(testing::CreateFunctor(InvokeGetChunksCallback,
                                    std::vector<SBListChunkRanges>(),
                                    false)));
  EXPECT_CALL(test_delegate, UpdateFinished(false)).Times(1);

  std::unique_ptr<SafeBrowsingProtocolManager> pm(
      CreateProtocolManager(&test_delegate));

  // Kick off initialization. This returns chunks from the DB synchronously.
  pm->ForceScheduleNextUpdate(TimeDelta());
  mock_time_task_runner_->RunUntilIdle();

  // We should have an URLFetcher at this point in time.
  ValidateUpdateFetcherRequest();

  // Go ahead and respond to it.
  pm->OnURLLoaderCompleteInternal(nullptr, net::OK, 404, std::string());

  // There should now be a backup request.
  ValidateUpdateFetcherRequest(kBackupHttpUrlPrefix, "");

  // Confirm that no update is scheduled (still waiting on a response to the
  // backup request).
  EXPECT_FALSE(pm->IsUpdateScheduled());

  // Force the timeout to fire. Need to fast forward by twice the timeout amount
  // as issuing the backup request above restarted the timeout timer but that
  // Timer's clock isn't mocked and its impl is such that it will re-use its
  // initial delayed task and re-post by the remainder of the timeout when it
  // fires (which is pretty much the full timeout in real time since we mock the
  // wait). A cleaner solution would be to pass
  // |mock_time_task_runner_->GetMockTickClock()| to the
  // SafeBrowsingProtocolManager's Timers but such hooks were deemed overkill
  // per this being the only use case at this point.
  mock_time_task_runner_->FastForwardBy(
      SafeBrowsingProtocolManager::GetUpdateTimeoutForTesting() * 2);
  EXPECT_TRUE(pm->IsUpdateScheduled());
}

// Tests what happens when there is a connection error when issuing the update
// request, and an error with the backup update request.
TEST_F(SafeBrowsingProtocolManagerTest,
       UpdateResponseConnectionErrorBackupError) {
  testing::StrictMock<MockProtocolDelegate> test_delegate;
  EXPECT_CALL(test_delegate, UpdateStarted()).Times(1);
  EXPECT_CALL(test_delegate, GetChunks(_)).WillOnce(
      Invoke(testing::CreateFunctor(InvokeGetChunksCallback,
                                    std::vector<SBListChunkRanges>(),
                                    false)));
  EXPECT_CALL(test_delegate, UpdateFinished(false)).Times(1);

  std::unique_ptr<SafeBrowsingProtocolManager> pm(
      CreateProtocolManager(&test_delegate));

  // Kick off initialization. This returns chunks from the DB synchronously.
  pm->ForceScheduleNextUpdate(TimeDelta());
  mock_time_task_runner_->RunUntilIdle();

  // We should have an URLFetcher at this point in time.
  ValidateUpdateFetcherRequest();

  // Go ahead and respond to it.
  pm->OnURLLoaderCompleteInternal(nullptr, net::ERR_CONNECTION_RESET, 0,
                                  std::string());

  // There should be a backup URLFetcher now.
  ValidateUpdateFetcherRequest(kBackupConnectUrlPrefix, "");

  // Respond to the backup unsuccessfully.
  pm->OnURLLoaderCompleteInternal(nullptr, net::OK, 404, std::string());

  EXPECT_TRUE(pm->IsUpdateScheduled());
}

// Tests what happens when there is a connection error when issuing the update
// request, and a successful response to the backup update request.
TEST_F(SafeBrowsingProtocolManagerTest,
       UpdateResponseConnectionErrorBackupSuccess) {
  testing::StrictMock<MockProtocolDelegate> test_delegate;
  EXPECT_CALL(test_delegate, UpdateStarted()).Times(1);
  EXPECT_CALL(test_delegate, GetChunks(_)).WillOnce(
      Invoke(testing::CreateFunctor(InvokeGetChunksCallback,
                                    std::vector<SBListChunkRanges>(),
                                    false)));
  EXPECT_CALL(test_delegate, UpdateFinished(true)).Times(1);

  std::unique_ptr<SafeBrowsingProtocolManager> pm(
      CreateProtocolManager(&test_delegate));

  // Kick off initialization. This returns chunks from the DB synchronously.
  pm->ForceScheduleNextUpdate(TimeDelta());
  mock_time_task_runner_->RunUntilIdle();

  // We should have an URLFetcher at this point in time.
  ValidateUpdateFetcherRequest();

  // Go ahead and respond to it.
  pm->OnURLLoaderCompleteInternal(nullptr, net::ERR_CONNECTION_RESET, 0,
                                  std::string());

  // There should be a backup URLFetcher now.
  ValidateUpdateFetcherRequest(kBackupConnectUrlPrefix, "");

  // Respond to the backup unsuccessfully.
  pm->OnURLLoaderCompleteInternal(nullptr, net::OK, 200, std::string());

  EXPECT_TRUE(pm->IsUpdateScheduled());
}
// Tests what happens when there is a network state error when issuing the
// update request, and an error with the backup update request.
TEST_F(SafeBrowsingProtocolManagerTest,
       UpdateResponseNetworkErrorBackupError) {
  testing::StrictMock<MockProtocolDelegate> test_delegate;
  EXPECT_CALL(test_delegate, UpdateStarted()).Times(1);
  EXPECT_CALL(test_delegate, GetChunks(_)).WillOnce(
      Invoke(testing::CreateFunctor(InvokeGetChunksCallback,
                                    std::vector<SBListChunkRanges>(),
                                    false)));
  EXPECT_CALL(test_delegate, UpdateFinished(false)).Times(1);

  std::unique_ptr<SafeBrowsingProtocolManager> pm(
      CreateProtocolManager(&test_delegate));

  // Kick off initialization. This returns chunks from the DB synchronously.
  pm->ForceScheduleNextUpdate(TimeDelta());
  mock_time_task_runner_->RunUntilIdle();

  // We should have an URLFetcher at this point in time.
  ValidateUpdateFetcherRequest();

  // Go ahead and respond to it.
  pm->OnURLLoaderCompleteInternal(nullptr, net::ERR_INTERNET_DISCONNECTED, 0,
                                  std::string());

  // There should be a backup URLFetcher now.
  ValidateUpdateFetcherRequest(kBackupNetworkUrlPrefix, "");

  // Respond to the backup unsuccessfully.
  pm->OnURLLoaderCompleteInternal(nullptr, net::OK, 404, std::string());

  EXPECT_TRUE(pm->IsUpdateScheduled());
}

// Tests what happens when there is a network state error when issuing the
// update request, and a successful response to the backup update request.
TEST_F(SafeBrowsingProtocolManagerTest,
       UpdateResponseNetworkErrorBackupSuccess) {
  testing::StrictMock<MockProtocolDelegate> test_delegate;
  EXPECT_CALL(test_delegate, UpdateStarted()).Times(1);
  EXPECT_CALL(test_delegate, GetChunks(_)).WillOnce(
      Invoke(testing::CreateFunctor(InvokeGetChunksCallback,
                                    std::vector<SBListChunkRanges>(),
                                    false)));
  EXPECT_CALL(test_delegate, UpdateFinished(true)).Times(1);

  std::unique_ptr<SafeBrowsingProtocolManager> pm(
      CreateProtocolManager(&test_delegate));

  // Kick off initialization. This returns chunks from the DB synchronously.
  pm->ForceScheduleNextUpdate(TimeDelta());
  mock_time_task_runner_->RunUntilIdle();

  // We should have an URLFetcher at this point in time.
  ValidateUpdateFetcherRequest();

  // Go ahead and respond to it.
  pm->OnURLLoaderCompleteInternal(nullptr, net::ERR_INTERNET_DISCONNECTED, 0,
                                  std::string());

  // There should be a backup URLFetcher now.
  ValidateUpdateFetcherRequest(kBackupNetworkUrlPrefix, "");

  // Respond to the backup unsuccessfully.
  pm->OnURLLoaderCompleteInternal(nullptr, net::OK, 200, std::string());

  EXPECT_TRUE(pm->IsUpdateScheduled());
}

// Tests what happens when there is a timeout before an update response.
TEST_F(SafeBrowsingProtocolManagerTest, UpdateResponseTimeoutBackupSuccess) {
  testing::StrictMock<MockProtocolDelegate> test_delegate;
  EXPECT_CALL(test_delegate, UpdateStarted()).Times(1);
  EXPECT_CALL(test_delegate, GetChunks(_)).WillOnce(
      Invoke(testing::CreateFunctor(InvokeGetChunksCallback,
                                    std::vector<SBListChunkRanges>(),
                                    false)));
  EXPECT_CALL(test_delegate, UpdateFinished(true)).Times(1);

  std::unique_ptr<SafeBrowsingProtocolManager> pm(
      CreateProtocolManager(&test_delegate));

  // Kick off initialization. This returns chunks from the DB synchronously.
  pm->ForceScheduleNextUpdate(TimeDelta());
  mock_time_task_runner_->RunUntilIdle();

  // We should have an URLFetcher at this point in time.
  ValidateUpdateFetcherRequest();

  // Force the timeout to fire.
  mock_time_task_runner_->FastForwardBy(
      SafeBrowsingProtocolManager::GetUpdateTimeoutForTesting());

  // There should be a backup URLFetcher now.
  ValidateUpdateFetcherRequest(kBackupConnectUrlPrefix, "");

  // Respond to the backup unsuccessfully.
  pm->OnURLLoaderCompleteInternal(nullptr, net::OK, 200, std::string());

  EXPECT_TRUE(pm->IsUpdateScheduled());
}

// Tests what happens when there is a reset command in the response.
TEST_F(SafeBrowsingProtocolManagerTest, UpdateResponseReset) {
  testing::StrictMock<MockProtocolDelegate> test_delegate;
  EXPECT_CALL(test_delegate, UpdateStarted()).Times(1);
  EXPECT_CALL(test_delegate, GetChunks(_)).WillOnce(
      Invoke(testing::CreateFunctor(InvokeGetChunksCallback,
                                    std::vector<SBListChunkRanges>(),
                                    false)));
  EXPECT_CALL(test_delegate, ResetDatabase()).Times(1);
  EXPECT_CALL(test_delegate, UpdateFinished(true)).Times(1);

  std::unique_ptr<SafeBrowsingProtocolManager> pm(
      CreateProtocolManager(&test_delegate));

  // Kick off initialization. This returns chunks from the DB synchronously.
  pm->ForceScheduleNextUpdate(TimeDelta());
  mock_time_task_runner_->RunUntilIdle();

  ValidateUpdateFetcherRequest();

  // The update response is successful, and has a reset command.
  pm->OnURLLoaderCompleteInternal(nullptr, net::OK, 200, "r:pleasereset\n");

  EXPECT_TRUE(pm->IsUpdateScheduled());
}

// Tests a single valid update response, followed by a single redirect response
// that has an valid, but empty body.
TEST_F(SafeBrowsingProtocolManagerTest, EmptyRedirectResponse) {
  testing::StrictMock<MockProtocolDelegate> test_delegate;
  EXPECT_CALL(test_delegate, UpdateStarted()).Times(1);
  EXPECT_CALL(test_delegate, GetChunks(_)).WillOnce(
      Invoke(testing::CreateFunctor(InvokeGetChunksCallback,
                                    std::vector<SBListChunkRanges>(),
                                    false)));
  EXPECT_CALL(test_delegate, UpdateFinished(true)).Times(1);

  std::unique_ptr<SafeBrowsingProtocolManager> pm(
      CreateProtocolManager(&test_delegate));

  // Kick off initialization. This returns chunks from the DB synchronously.
  pm->ForceScheduleNextUpdate(TimeDelta());
  mock_time_task_runner_->RunUntilIdle();

  // The update response contains a single redirect command.
  ValidateUpdateFetcherRequest();
  pm->OnURLLoaderCompleteInternal(
      nullptr, net::OK, 200,
      base::StringPrintf("i:%s\n"
                         "u:redirect-server.example.com/path\n",
                         kDefaultPhishList));

  // The redirect response contains an empty body.
  ValidateRedirectFetcherRequest("https://redirect-server.example.com/path");
  pm->OnURLLoaderCompleteInternal(nullptr, net::OK, 200, std::string());

  EXPECT_TRUE(pm->IsUpdateScheduled());
}

// Tests a single valid update response, followed by a single redirect response
// that has an invalid body.
TEST_F(SafeBrowsingProtocolManagerTest, InvalidRedirectResponse) {
  testing::StrictMock<MockProtocolDelegate> test_delegate;
  EXPECT_CALL(test_delegate, UpdateStarted()).Times(1);
  EXPECT_CALL(test_delegate, GetChunks(_)).WillOnce(
      Invoke(testing::CreateFunctor(InvokeGetChunksCallback,
                                    std::vector<SBListChunkRanges>(),
                                    false)));
  EXPECT_CALL(test_delegate, UpdateFinished(false)).Times(1);

  std::unique_ptr<SafeBrowsingProtocolManager> pm(
      CreateProtocolManager(&test_delegate));

  // Kick off initialization. This returns chunks from the DB synchronously.
  pm->ForceScheduleNextUpdate(TimeDelta());
  mock_time_task_runner_->RunUntilIdle();

  // The update response contains a single redirect command.
  ValidateUpdateFetcherRequest();
  pm->OnURLLoaderCompleteInternal(
      nullptr, net::OK, 200,
      base::StringPrintf("i:%s\n"
                         "u:redirect-server.example.com/path\n",
                         kDefaultPhishList));

  // The redirect response contains an invalid body.
  ValidateRedirectFetcherRequest("https://redirect-server.example.com/path");
  pm->OnURLLoaderCompleteInternal(nullptr, net::OK, 200,
                                  "THIS IS AN INVALID RESPONSE");

  EXPECT_TRUE(pm->IsUpdateScheduled());
}

// Tests a single valid update response, followed by a single redirect response
// containing chunks.
TEST_F(SafeBrowsingProtocolManagerTest, SingleRedirectResponseWithChunks) {
  testing::StrictMock<MockProtocolDelegate> test_delegate;
  EXPECT_CALL(test_delegate, UpdateStarted()).Times(1);
  EXPECT_CALL(test_delegate, GetChunks(_)).WillOnce(
      Invoke(testing::CreateFunctor(InvokeGetChunksCallback,
                                    std::vector<SBListChunkRanges>(),
                                    false)));
  EXPECT_CALL(test_delegate, AddChunksRaw(kDefaultPhishList, _, _)).WillOnce(
      Invoke(HandleAddChunks));
  EXPECT_CALL(test_delegate, UpdateFinished(true)).Times(1);

  std::unique_ptr<SafeBrowsingProtocolManager> pm(
      CreateProtocolManager(&test_delegate));

  // Kick off initialization. This returns chunks from the DB synchronously.
  pm->ForceScheduleNextUpdate(TimeDelta());
  mock_time_task_runner_->RunUntilIdle();

  // The update response contains a single redirect command.
  ValidateUpdateFetcherRequest();
  pm->OnURLLoaderCompleteInternal(
      nullptr, net::OK, 200,
      base::StringPrintf("i:%s\n"
                         "u:redirect-server.example.com/path\n",
                         kDefaultPhishList));

  // The redirect response contains a single chunk.
  ValidateRedirectFetcherRequest("https://redirect-server.example.com/path");
  pm->OnURLLoaderCompleteInternal(nullptr, net::OK, 200, kChunkPayload1);

  EXPECT_FALSE(pm->IsUpdateScheduled());

  // The AddChunksCallback needs to be invoked.
  mock_time_task_runner_->RunUntilIdle();

  EXPECT_TRUE(pm->IsUpdateScheduled());
}

// Tests a single valid update response, followed by multiple redirect responses
// containing chunks.
TEST_F(SafeBrowsingProtocolManagerTest, MultipleRedirectResponsesWithChunks) {
  testing::StrictMock<MockProtocolDelegate> test_delegate;
  EXPECT_CALL(test_delegate, UpdateStarted()).Times(1);
  EXPECT_CALL(test_delegate, GetChunks(_)).WillOnce(
      Invoke(testing::CreateFunctor(InvokeGetChunksCallback,
                                    std::vector<SBListChunkRanges>(),
                                    false)));
  EXPECT_CALL(test_delegate, AddChunksRaw(kDefaultPhishList, _, _)).
      WillRepeatedly(Invoke(HandleAddChunks));
  EXPECT_CALL(test_delegate, UpdateFinished(true)).Times(1);

  std::unique_ptr<SafeBrowsingProtocolManager> pm(
      CreateProtocolManager(&test_delegate));

  // Kick off initialization. This returns chunks from the DB synchronously.
  pm->ForceScheduleNextUpdate(TimeDelta());
  mock_time_task_runner_->RunUntilIdle();

  // The update response contains multiple redirect commands.
  ValidateUpdateFetcherRequest();
  pm->OnURLLoaderCompleteInternal(
      nullptr, net::OK, 200,
      base::StringPrintf("i:%s\n"
                         "u:redirect-server.example.com/one\n"
                         "u:redirect-server.example.com/two\n",
                         kDefaultPhishList));

  // The first redirect response contains a single chunk.
  ValidateRedirectFetcherRequest("https://redirect-server.example.com/one");
  pm->OnURLLoaderCompleteInternal(nullptr, net::OK, 200, kChunkPayload1);

  // Invoke the AddChunksCallback to trigger the second request.
  mock_time_task_runner_->RunUntilIdle();

  EXPECT_FALSE(pm->IsUpdateScheduled());

  // The second redirect response contains a single chunk.
  ValidateRedirectFetcherRequest("https://redirect-server.example.com/two");
  pm->OnURLLoaderCompleteInternal(nullptr, net::OK, 200, kChunkPayload2);

  EXPECT_FALSE(pm->IsUpdateScheduled());

  // Invoke the AddChunksCallback to finish the update.
  mock_time_task_runner_->RunUntilIdle();

  EXPECT_TRUE(pm->IsUpdateScheduled());
}

}  // namespace safe_browsing
