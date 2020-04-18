// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/safe_browsing/local_database_manager.h"

#include <stddef.h>

#include <memory>
#include <string>
#include <vector>

#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "base/run_loop.h"
#include "chrome/browser/safe_browsing/safe_browsing_service.h"
#include "components/safe_browsing/db/v4_get_hash_protocol_manager.h"
#include "components/safe_browsing/db/v4_protocol_manager_util.h"
#include "components/safe_browsing/db/v4_test_util.h"
#include "content/public/test/test_browser_thread_bundle.h"
#include "content/public/test/test_utils.h"
#include "net/url_request/url_request_context_getter.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "testing/platform_test.h"
#include "url/gurl.h"

using content::TestBrowserThreadBundle;

namespace safe_browsing {

class LocalDatabaseManagerTest : public PlatformTest {
 public:
  struct HostListPair {
    std::string host;
    std::string list_type;
  };

  bool RunSBHashTest(const ListType list_type,
                     const SBThreatTypeSet& expected_threats,
                     const std::vector<std::string>& result_lists);
  bool RunUrlTest(const GURL& url,
                  ListType list_type,
                  const SBThreatTypeSet& expected_threats,
                  const std::vector<HostListPair>& host_list_results);

  // Constant values used in tests.
  const SBThreatTypeSet malware_threat_ =
      CreateSBThreatTypeSet({SB_THREAT_TYPE_URL_BINARY_MALWARE});
  const SBThreatTypeSet multiple_threats_ = CreateSBThreatTypeSet(
      {SB_THREAT_TYPE_URL_MALWARE, SB_THREAT_TYPE_URL_PHISHING});
  const SBThreatTypeSet unwanted_threat_ =
      CreateSBThreatTypeSet({SB_THREAT_TYPE_URL_UNWANTED});

 private:
  bool RunTest(std::unique_ptr<
                   LocalSafeBrowsingDatabaseManager::SafeBrowsingCheck> check,
               const std::vector<SBFullHashResult>& hash_results);

  TestBrowserThreadBundle thread_bundle_;
};

bool LocalDatabaseManagerTest::RunSBHashTest(
    const ListType list_type,
    const SBThreatTypeSet& expected_threats,
    const std::vector<std::string>& result_lists) {
  const SBFullHash same_full_hash = {};
  std::unique_ptr<LocalSafeBrowsingDatabaseManager::SafeBrowsingCheck> check(
      new LocalSafeBrowsingDatabaseManager::SafeBrowsingCheck(
          std::vector<GURL>(), std::vector<SBFullHash>(1, same_full_hash), NULL,
          list_type, expected_threats));

  std::vector<SBFullHashResult> fake_results;
  for (const auto& result_list : result_lists) {
    const SBFullHashResult full_hash_result = {same_full_hash,
                                               GetListId(result_list)};
    fake_results.push_back(full_hash_result);
  }
  return RunTest(std::move(check), fake_results);
}

bool LocalDatabaseManagerTest::RunUrlTest(
    const GURL& url,
    ListType list_type,
    const SBThreatTypeSet& expected_threats,
    const std::vector<HostListPair>& host_list_results) {
  std::unique_ptr<LocalSafeBrowsingDatabaseManager::SafeBrowsingCheck> check(
      new LocalSafeBrowsingDatabaseManager::SafeBrowsingCheck(
          std::vector<GURL>(1, url), std::vector<SBFullHash>(), NULL, list_type,
          expected_threats));
  std::vector<SBFullHashResult> full_hash_results;
  for (const auto& host_list : host_list_results) {
    SBFullHashResult hash_result =
        {SBFullHashForString(host_list.host), GetListId(host_list.list_type)};
    full_hash_results.push_back(hash_result);
  }
  return RunTest(std::move(check), full_hash_results);
}

bool LocalDatabaseManagerTest::RunTest(
    std::unique_ptr<LocalSafeBrowsingDatabaseManager::SafeBrowsingCheck> check,
    const std::vector<SBFullHashResult>& hash_results) {
  scoped_refptr<SafeBrowsingService> sb_service_(
      SafeBrowsingService::CreateSafeBrowsingService());
  scoped_refptr<LocalSafeBrowsingDatabaseManager> db_manager_(
      new LocalSafeBrowsingDatabaseManager(sb_service_));
  LocalSafeBrowsingDatabaseManager::SafeBrowsingCheck* check_ptr = check.get();
  db_manager_->checks_[check_ptr] = std::move(check);

  bool result = db_manager_->HandleOneCheck(check_ptr, hash_results);
  db_manager_->checks_.erase(check_ptr);
  return result;
}

TEST_F(LocalDatabaseManagerTest, CheckCorrespondsListTypeForHash) {
  EXPECT_FALSE(RunSBHashTest(BINURL, malware_threat_, {kMalwareList}));
  EXPECT_TRUE(RunSBHashTest(BINURL, malware_threat_, {kBinUrlList}));

  // Check for multiple threats
  EXPECT_FALSE(RunSBHashTest(MALWARE, multiple_threats_, {kBinUrlList}));
  EXPECT_TRUE(RunSBHashTest(MALWARE, multiple_threats_, {kMalwareList}));

  // Check for multiple hash hits
  std::vector<std::string> hash_hits = {kMalwareList, kUnwantedUrlList};
  EXPECT_TRUE(RunSBHashTest(UNWANTEDURL, unwanted_threat_, hash_hits));
}

TEST_F(LocalDatabaseManagerTest, CheckCorrespondsListTypeForUrl) {
  const GURL url("http://www.host.com/index.html");
  const std::string host1 = "host.com/";
  const std::string host2 = "www.host.com/";
  const std::vector<HostListPair> malware_list_result =
      {{host1, kMalwareList}};
  const std::vector<HostListPair> binurl_list_result =
      {{host2, kBinUrlList}};

  EXPECT_FALSE(RunUrlTest(url, BINURL, malware_threat_, malware_list_result));
  EXPECT_TRUE(RunUrlTest(url, BINURL, malware_threat_, binurl_list_result));

  // Check for multiple expected threats
  EXPECT_FALSE(RunUrlTest(url, MALWARE, multiple_threats_, binurl_list_result));
  EXPECT_TRUE(RunUrlTest(url, MALWARE, multiple_threats_, malware_list_result));

  // Check for multiple database hits
  std::vector<HostListPair> multiple_results = {
    {host1, kMalwareList}, {host2, kUnwantedUrlList}};
  EXPECT_TRUE(RunUrlTest(url, UNWANTEDURL, unwanted_threat_, multiple_results));
}

TEST_F(LocalDatabaseManagerTest, GetUrlSeverestThreatType) {
  std::vector<SBFullHashResult> full_hashes;

  const GURL kMalwareUrl("http://www.malware.com/page.html");
  const GURL kPhishingUrl("http://www.phishing.com/page.html");
  const GURL kUnwantedUrl("http://www.unwanted.com/page.html");
  const GURL kUnwantedAndMalwareUrl(
      "http://www.unwantedandmalware.com/page.html");
  const GURL kBlacklistedResourceUrl("http://www.blacklisted.com/script.js");
  const GURL kUnwantedResourceUrl("http://www.unwantedresource.com/script.js");
  const GURL kMalwareResourceUrl("http://www.malwareresource.com/script.js");
  const GURL kSafeUrl("http://www.safe.com/page.html");

  const SBFullHash kMalwareHostHash = SBFullHashForString("malware.com/");
  const SBFullHash kPhishingHostHash = SBFullHashForString("phishing.com/");
  const SBFullHash kUnwantedHostHash = SBFullHashForString("unwanted.com/");
  const SBFullHash kUnwantedAndMalwareHostHash =
      SBFullHashForString("unwantedandmalware.com/");
  const SBFullHash kBlacklistedResourceHostHash =
      SBFullHashForString("blacklisted.com/");
  const SBFullHash kUnwantedResourceHostHash =
      SBFullHashForString("unwantedresource.com/");
  const SBFullHash kMalwareResourceHostHash =
      SBFullHashForString("malwareresource.com/");
  const SBFullHash kSafeHostHash = SBFullHashForString("www.safe.com/");

  {
    SBFullHashResult full_hash;
    full_hash.hash = kMalwareHostHash;
    full_hash.list_id = static_cast<int>(MALWARE);
    full_hashes.push_back(full_hash);
  }

  {
    SBFullHashResult full_hash;
    full_hash.hash = kPhishingHostHash;
    full_hash.list_id = static_cast<int>(PHISH);
    full_hashes.push_back(full_hash);
  }

  {
    SBFullHashResult full_hash;
    full_hash.hash = kUnwantedHostHash;
    full_hash.list_id = static_cast<int>(UNWANTEDURL);
    full_hashes.push_back(full_hash);
  }

  {
    SBFullHashResult full_hash;
    full_hash.hash = kBlacklistedResourceHostHash;
    full_hash.list_id = static_cast<int>(RESOURCEBLACKLIST);
    full_hashes.push_back(full_hash);
  }

  {
    // Add both MALWARE and UNWANTEDURL list IDs for
    // kUnwantedAndMalwareHostHash.
    SBFullHashResult full_hash_malware;
    full_hash_malware.hash = kUnwantedAndMalwareHostHash;
    full_hash_malware.list_id = static_cast<int>(MALWARE);
    full_hashes.push_back(full_hash_malware);

    SBFullHashResult full_hash_unwanted;
    full_hash_unwanted.hash = kUnwantedAndMalwareHostHash;
    full_hash_unwanted.list_id = static_cast<int>(UNWANTEDURL);
    full_hashes.push_back(full_hash_unwanted);
  }

  {
    SBFullHashResult full_hash_unwanted =
        {kUnwantedResourceHostHash, static_cast<int>(UNWANTEDURL)};
    full_hashes.push_back(full_hash_unwanted);

    SBFullHashResult full_hash_resource =
        {kUnwantedResourceHostHash, static_cast<int>(RESOURCEBLACKLIST)};
    full_hashes.push_back(full_hash_resource);
  }

  {
    SBFullHashResult full_hash_malware =
        {kMalwareResourceHostHash, static_cast<int>(MALWARE)};
    full_hashes.push_back(full_hash_malware);

    SBFullHashResult full_hash_resource =
        {kMalwareResourceHostHash, static_cast<int>(RESOURCEBLACKLIST)};
    full_hashes.push_back(full_hash_resource);
  }

  EXPECT_EQ(SB_THREAT_TYPE_URL_MALWARE,
            LocalSafeBrowsingDatabaseManager::GetHashSeverestThreatType(
                kMalwareHostHash, full_hashes));

  EXPECT_EQ(SB_THREAT_TYPE_URL_PHISHING,
            LocalSafeBrowsingDatabaseManager::GetHashSeverestThreatType(
                kPhishingHostHash, full_hashes));

  EXPECT_EQ(SB_THREAT_TYPE_URL_UNWANTED,
            LocalSafeBrowsingDatabaseManager::GetHashSeverestThreatType(
                kUnwantedHostHash, full_hashes));

  EXPECT_EQ(SB_THREAT_TYPE_URL_MALWARE,
            LocalSafeBrowsingDatabaseManager::GetHashSeverestThreatType(
                kUnwantedAndMalwareHostHash, full_hashes));

  EXPECT_EQ(SB_THREAT_TYPE_BLACKLISTED_RESOURCE,
            LocalSafeBrowsingDatabaseManager::GetHashSeverestThreatType(
                kBlacklistedResourceHostHash, full_hashes));

  EXPECT_EQ(SB_THREAT_TYPE_URL_UNWANTED,
            LocalSafeBrowsingDatabaseManager::GetHashSeverestThreatType(
                kUnwantedResourceHostHash, full_hashes));

  EXPECT_EQ(SB_THREAT_TYPE_URL_MALWARE,
            LocalSafeBrowsingDatabaseManager::GetHashSeverestThreatType(
                kMalwareResourceHostHash, full_hashes));

  EXPECT_EQ(SB_THREAT_TYPE_SAFE,
            LocalSafeBrowsingDatabaseManager::GetHashSeverestThreatType(
                kSafeHostHash, full_hashes));

  const size_t kArbitraryValue = 123456U;
  size_t index = kArbitraryValue;
  EXPECT_EQ(SB_THREAT_TYPE_URL_MALWARE,
            LocalSafeBrowsingDatabaseManager::GetUrlSeverestThreatType(
                kMalwareUrl, full_hashes, &index));
  EXPECT_EQ(0U, index);

  EXPECT_EQ(SB_THREAT_TYPE_URL_PHISHING,
            LocalSafeBrowsingDatabaseManager::GetUrlSeverestThreatType(
                kPhishingUrl, full_hashes, &index));
  EXPECT_EQ(1U, index);

  EXPECT_EQ(SB_THREAT_TYPE_URL_UNWANTED,
            LocalSafeBrowsingDatabaseManager::GetUrlSeverestThreatType(
                kUnwantedUrl, full_hashes, &index));
  EXPECT_EQ(2U, index);

  EXPECT_EQ(SB_THREAT_TYPE_BLACKLISTED_RESOURCE,
            LocalSafeBrowsingDatabaseManager::GetUrlSeverestThreatType(
                kBlacklistedResourceUrl, full_hashes, &index));
  EXPECT_EQ(3U, index);

  EXPECT_EQ(SB_THREAT_TYPE_URL_MALWARE,
            LocalSafeBrowsingDatabaseManager::GetUrlSeverestThreatType(
                kUnwantedAndMalwareUrl, full_hashes, &index));
  EXPECT_EQ(4U, index);

  EXPECT_EQ(SB_THREAT_TYPE_URL_UNWANTED,
            LocalSafeBrowsingDatabaseManager::GetUrlSeverestThreatType(
                kUnwantedResourceUrl, full_hashes, &index));
  EXPECT_EQ(6U, index);

  EXPECT_EQ(SB_THREAT_TYPE_URL_MALWARE,
            LocalSafeBrowsingDatabaseManager::GetUrlSeverestThreatType(
                kMalwareResourceUrl, full_hashes, &index));
  EXPECT_EQ(8U, index);

  index = kArbitraryValue;
  EXPECT_EQ(SB_THREAT_TYPE_SAFE,
            LocalSafeBrowsingDatabaseManager::GetUrlSeverestThreatType(
                kSafeUrl, full_hashes, &index));
  EXPECT_EQ(kArbitraryValue, index);
}

TEST_F(LocalDatabaseManagerTest, ServiceStopWithPendingChecks) {
  scoped_refptr<SafeBrowsingService> sb_service(
      SafeBrowsingService::CreateSafeBrowsingService());
  scoped_refptr<LocalSafeBrowsingDatabaseManager> db_manager(
      new LocalSafeBrowsingDatabaseManager(sb_service));
  SafeBrowsingDatabaseManager::Client client;

  // Start the service and flush tasks to ensure database is made available.
  db_manager->StartOnIOThread(NULL, GetTestV4ProtocolConfig());
  content::RunAllTasksUntilIdle();
  base::RunLoop().RunUntilIdle();
  EXPECT_TRUE(db_manager->DatabaseAvailable());

  // Start an extension check operation, which is done asynchronously.
  std::set<std::string> extension_ids;
  extension_ids.insert("testtesttesttesttesttesttesttest");
  db_manager->CheckExtensionIDs(extension_ids, &client);

  // Stop the service without first flushing above tasks.
  db_manager->StopOnIOThread(false);

  // Now run posted tasks, whish should include the extension check which has
  // been posted to the safe browsing task runner. This should not crash.
  content::RunAllTasksUntilIdle();
  base::RunLoop().RunUntilIdle();
}

}  // namespace safe_browsing
