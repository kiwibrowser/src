// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/safe_browsing/incident_reporting/resource_request_detector.h"

#include <utility>

#include "base/bind.h"
#include "base/memory/ptr_util.h"
#include "base/run_loop.h"
#include "chrome/browser/safe_browsing/incident_reporting/incident.h"
#include "chrome/browser/safe_browsing/incident_reporting/incident_receiver.h"
#include "chrome/browser/safe_browsing/incident_reporting/mock_incident_receiver.h"
#include "components/safe_browsing/db/test_database_manager.h"
#include "components/safe_browsing/proto/csd.pb.h"
#include "content/public/browser/resource_request_info.h"
#include "content/public/common/previews_state.h"
#include "content/public/common/resource_type.h"
#include "content/public/test/test_browser_thread_bundle.h"
#include "crypto/sha2.h"
#include "ipc/ipc_message.h"
#include "net/base/request_priority.h"
#include "net/traffic_annotation/network_traffic_annotation_test_helper.h"
#include "net/url_request/url_request.h"
#include "net/url_request/url_request_test_util.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

using ::testing::IsNull;
using ::testing::Return;
using ::testing::StrictMock;
using ::testing::WithArg;
using ::testing::_;

namespace safe_browsing {

namespace {

class FakeResourceRequestDetector : public ResourceRequestDetector {
 public:
  FakeResourceRequestDetector(
      scoped_refptr<SafeBrowsingDatabaseManager> database_manager,
      std::unique_ptr<IncidentReceiver> incident_receiver)
      : ResourceRequestDetector(database_manager,
                                std::move(incident_receiver)) {
    FakeResourceRequestDetector::set_allow_null_profile_for_testing(true);
  }
};

class MockSafeBrowsingDatabaseManager : public TestSafeBrowsingDatabaseManager {
 public:
  MockSafeBrowsingDatabaseManager() {}

  MOCK_METHOD2(CheckResourceUrl, bool(
      const GURL& url,
      SafeBrowsingDatabaseManager::Client* client));

 protected:
  ~MockSafeBrowsingDatabaseManager() override {}

 private:
  DISALLOW_COPY_AND_ASSIGN(MockSafeBrowsingDatabaseManager);
};

}  // namespace

ACTION_P3(CallClientWithResult, url, threat_type, threat_hash) {
  arg1->OnCheckResourceUrlResult(url, threat_type, threat_hash);
  return false;
}

class ResourceRequestDetectorTest : public testing::Test {
 protected:
  using ResourceRequestIncidentMessage =
      ClientIncidentReport::IncidentData::ResourceRequestIncident;

  ResourceRequestDetectorTest()
      : mock_incident_receiver_(
            new StrictMock<safe_browsing::MockIncidentReceiver>()),
        mock_database_manager_(new StrictMock<MockSafeBrowsingDatabaseManager>),
        fake_resource_request_detector_(
            std::make_unique<FakeResourceRequestDetector>(
                mock_database_manager_,
                base::WrapUnique(mock_incident_receiver_))) {}

  void TearDown() override {
    fake_resource_request_detector_.reset();
    mock_database_manager_ = nullptr;
    base::RunLoop().RunUntilIdle();
  }

  std::unique_ptr<net::URLRequest> GetTestURLRequest(
      const std::string& url,
      content::ResourceType resource_type) const {
    std::unique_ptr<net::URLRequest> url_request(context_.CreateRequest(
        GURL(url), net::DEFAULT_PRIORITY, NULL, TRAFFIC_ANNOTATION_FOR_TESTS));

    content::ResourceRequestInfo::AllocateForTesting(
        url_request.get(), resource_type,
        /*resource_context=*/NULL,
        /*render_process_id=*/0,
        /*render_view_id=*/0,
        /*render_frame_id=*/MSG_ROUTING_NONE,
        /*is_main_frame=*/true,
        /*allow_download=*/true,
        /*is_async=*/false, content::PREVIEWS_OFF,
        /*navigation_ui_data*/ nullptr);

    return url_request;
  }

  void ExpectNoDatabaseCheck() {
    EXPECT_CALL(*mock_database_manager_, CheckResourceUrl(_, _))
        .Times(0);
  }

  void ExpectNegativeSyncDatabaseCheck(const std::string& url) {
    EXPECT_CALL(*mock_database_manager_, CheckResourceUrl(GURL(url), _))
        .WillOnce(Return(true));
  }

  void ExpectAsyncDatabaseCheck(const std::string& url,
                                bool is_blacklisted,
                                const std::string& digest) {
    SBThreatType threat_type = is_blacklisted
        ? SB_THREAT_TYPE_BLACKLISTED_RESOURCE
        : SB_THREAT_TYPE_SAFE;
    const GURL gurl(url);
    EXPECT_CALL(*mock_database_manager_, CheckResourceUrl(gurl, _))
        .WillOnce(CallClientWithResult(gurl, threat_type, digest));
  }

  void ExpectNoIncident(const std::string& url,
                        content::ResourceType resource_type) {
    std::unique_ptr<net::URLRequest> request(
        GetTestURLRequest(url, resource_type));

    EXPECT_CALL(*mock_incident_receiver_, DoAddIncidentForProfile(IsNull(), _))
        .Times(0);

    ResourceRequestInfo info =
        ResourceRequestDetector::GetRequestInfo(request.get());
    fake_resource_request_detector_->ProcessResourceRequest(&info);
    base::RunLoop().RunUntilIdle();
  }

  void ExpectIncidentAdded(
      const std::string& url,
      content::ResourceType resource_type,
      ResourceRequestIncidentMessage::Type expected_type,
      const std::string& expected_digest) {
    std::unique_ptr<net::URLRequest> request(
        GetTestURLRequest(url, resource_type));
    std::unique_ptr<Incident> incident;
    EXPECT_CALL(*mock_incident_receiver_, DoAddIncidentForProfile(IsNull(), _))
        .WillOnce(WithArg<1>(TakeIncident(&incident)));

    ResourceRequestInfo info =
        ResourceRequestDetector::GetRequestInfo(request.get());
    fake_resource_request_detector_->ProcessResourceRequest(&info);
    base::RunLoop().RunUntilIdle();

    ASSERT_TRUE(incident);
    std::unique_ptr<ClientIncidentReport_IncidentData> incident_data =
        incident->TakePayload();
    ASSERT_TRUE(incident_data && incident_data->has_resource_request());
    const ResourceRequestIncidentMessage& script_request_incident =
        incident_data->resource_request();
    EXPECT_TRUE(script_request_incident.has_digest());
    EXPECT_EQ(expected_digest, script_request_incident.digest());
    EXPECT_EQ(expected_type, script_request_incident.type());
  }

  StrictMock<safe_browsing::MockIncidentReceiver>* mock_incident_receiver_;
  scoped_refptr<MockSafeBrowsingDatabaseManager> mock_database_manager_;
  std::unique_ptr<FakeResourceRequestDetector> fake_resource_request_detector_;

 private:
  // UrlRequest requires a message loop. This provides one.
  content::TestBrowserThreadBundle thread_bundle_;
  net::TestURLRequestContext context_;
};

TEST_F(ResourceRequestDetectorTest, NoDbCheckForIgnoredResourceTypes) {
  ExpectNoDatabaseCheck();
  ExpectNoIncident(
      "http://www.example.com/index.html", content::RESOURCE_TYPE_MAIN_FRAME);
}

TEST_F(ResourceRequestDetectorTest, NoDbCheckForUnsupportedSchemes) {
  ExpectNoDatabaseCheck();
  ExpectNoIncident(
      "file:///usr/local/script.js", content::RESOURCE_TYPE_SCRIPT);
  ExpectNoIncident(
      "chrome-extension://abcdefghi/script.js", content::RESOURCE_TYPE_SCRIPT);
}

TEST_F(ResourceRequestDetectorTest, NoEventForNegativeSynchronousDbCheck) {
  const std::string url = "http://www.example.com/script.js";
  ExpectNegativeSyncDatabaseCheck(url);
  ExpectNoIncident(url, content::RESOURCE_TYPE_SCRIPT);
}

TEST_F(ResourceRequestDetectorTest, NoEventForNegativeAsynchronousDbCheck) {
  const std::string url = "http://www.example.com/script.js";
  ExpectAsyncDatabaseCheck(url, false, "");
  ExpectNoIncident(url, content::RESOURCE_TYPE_SCRIPT);
}

TEST_F(ResourceRequestDetectorTest, EventAddedForSupportedSchemes) {
  std::string schemes[] = {"http", "https"};
  const std::string digest = "dummydigest";
  const std::string domain_path = "www.example.com/script.js";

  for (const auto& scheme : schemes) {
    const std::string url = scheme + "://" + domain_path;
    ExpectAsyncDatabaseCheck(url, true, digest);
    ExpectIncidentAdded(
        url, content::RESOURCE_TYPE_SCRIPT,
        ResourceRequestIncidentMessage::TYPE_PATTERN, digest);
  }
}

TEST_F(ResourceRequestDetectorTest, EventAddedForSupportedResourceTypes) {
  content::ResourceType supported_types[] = {
    content::RESOURCE_TYPE_SCRIPT,
    content::RESOURCE_TYPE_SUB_FRAME,
    content::RESOURCE_TYPE_OBJECT,
  };
  const std::string url = "http://www.example.com/";
  const std::string digest = "dummydigest";
  for (auto resource_type : supported_types) {
    ExpectAsyncDatabaseCheck(url, true, digest);
    ExpectIncidentAdded(url, resource_type,
                        ResourceRequestIncidentMessage::TYPE_PATTERN, digest);
  }
}

}  // namespace safe_browsing
