// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/offline_pages/core/downloads/download_notifying_observer.h"

#include "components/offline_pages/core/background/save_page_request.h"
#include "components/offline_pages/core/client_namespace_constants.h"
#include "components/offline_pages/core/client_policy_controller.h"
#include "components/offline_pages/core/downloads/offline_page_download_notifier.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace offline_pages {

namespace {
static const int64_t kTestOfflineId = 42L;
static const char kTestUrl[] = "http://foo.com/bar";
static const char kTestGuid[] = "cccccccc-cccc-4ccc-0ccc-ccccccccccc1";
static const ClientId kTestClientId(kDownloadNamespace, kTestGuid);
static const base::Time kTestCreationTime = base::Time::Now();
static const bool kTestUserRequested = true;

enum class LastNotificationType {
  NONE,
  DOWNLOAD_SUCCESSFUL,
  DOWNLOAD_FAILED,
  DOWNLOAD_PROGRESS,
  DOWNLOAD_PAUSED,
  DOWNLOAD_INTERRUPTED,
  DOWNLOAD_CANCELED,
  DOWNLOAD_SUPPRESSED,
};

class TestNotifier : public OfflinePageDownloadNotifier {
 public:
  TestNotifier();
  ~TestNotifier() override;

  // OfflinePageDownloadNotifier implementation:
  void NotifyDownloadSuccessful(const OfflineItem& item) override;
  void NotifyDownloadFailed(const OfflineItem& item) override;
  void NotifyDownloadProgress(const OfflineItem& item) override;
  void NotifyDownloadPaused(const OfflineItem& item) override;
  void NotifyDownloadInterrupted(const OfflineItem& item) override;
  void NotifyDownloadCanceled(const OfflineItem& item) override;
  bool MaybeSuppressNotification(const std::string& origin,
                                 const OfflineItem& item) override;

  void Reset();

  LastNotificationType last_notification_type() const {
    return last_notification_type_;
  }

  OfflineItem* offline_item() const { return offline_item_.get(); }

  void SetShouldSuppressNotification(bool should_suppress) {
    should_suppress_notification_ = should_suppress;
  }

 private:
  LastNotificationType last_notification_type_;
  std::unique_ptr<OfflineItem> offline_item_;
  bool should_suppress_notification_ = false;
};

TestNotifier::TestNotifier()
    : last_notification_type_(LastNotificationType::NONE) {}

TestNotifier::~TestNotifier() {}

void TestNotifier::NotifyDownloadSuccessful(const OfflineItem& item) {
  last_notification_type_ = LastNotificationType::DOWNLOAD_SUCCESSFUL;
  offline_item_.reset(new OfflineItem(item));
}

void TestNotifier::NotifyDownloadFailed(const OfflineItem& item) {
  last_notification_type_ = LastNotificationType::DOWNLOAD_FAILED;
  offline_item_.reset(new OfflineItem(item));
}

void TestNotifier::NotifyDownloadProgress(const OfflineItem& item) {
  last_notification_type_ = LastNotificationType::DOWNLOAD_PROGRESS;
  offline_item_.reset(new OfflineItem(item));
}

void TestNotifier::NotifyDownloadPaused(const OfflineItem& item) {
  last_notification_type_ = LastNotificationType::DOWNLOAD_PAUSED;
  offline_item_.reset(new OfflineItem(item));
}

void TestNotifier::NotifyDownloadInterrupted(const OfflineItem& item) {
  last_notification_type_ = LastNotificationType::DOWNLOAD_INTERRUPTED;
  offline_item_.reset(new OfflineItem(item));
}

void TestNotifier::NotifyDownloadCanceled(const OfflineItem& item) {
  last_notification_type_ = LastNotificationType::DOWNLOAD_CANCELED;
  offline_item_.reset(new OfflineItem(item));
}

bool TestNotifier::MaybeSuppressNotification(const std::string& origin,
                                             const OfflineItem& item) {
  last_notification_type_ = LastNotificationType::DOWNLOAD_SUPPRESSED;
  offline_item_.reset(new OfflineItem(item));
  return should_suppress_notification_;
}

void TestNotifier::Reset() {
  last_notification_type_ = LastNotificationType::NONE;
  offline_item_.reset(nullptr);
  should_suppress_notification_ = false;
}

}  // namespace

class DownloadNotifyingObserverTest : public testing::Test {
 public:
  DownloadNotifyingObserverTest();
  ~DownloadNotifyingObserverTest() override;

  // testing::Test implementation:
  void SetUp() override;

  TestNotifier* notifier() const { return notifier_; }
  DownloadNotifyingObserver* observer() { return observer_.get(); }

 private:
  TestNotifier* notifier_;
  std::unique_ptr<DownloadNotifyingObserver> observer_;
  std::unique_ptr<ClientPolicyController> policy_controller_;
};

DownloadNotifyingObserverTest::DownloadNotifyingObserverTest() {}

DownloadNotifyingObserverTest::~DownloadNotifyingObserverTest() {}

void DownloadNotifyingObserverTest::SetUp() {
  std::unique_ptr<TestNotifier> notifier(new TestNotifier);
  policy_controller_.reset(new ClientPolicyController());
  notifier_ = notifier.get();
  observer_.reset(new DownloadNotifyingObserver(std::move(notifier),
                                                policy_controller_.get()));
}

TEST_F(DownloadNotifyingObserverTest, OnAddedAsAvailable) {
  SavePageRequest request(kTestOfflineId, GURL(kTestUrl), kTestClientId,
                          kTestCreationTime, kTestUserRequested);
  request.set_request_state(SavePageRequest::RequestState::AVAILABLE);
  observer()->OnAdded(request);
  EXPECT_EQ(LastNotificationType::DOWNLOAD_INTERRUPTED,
            notifier()->last_notification_type());
  EXPECT_EQ(kTestGuid, notifier()->offline_item()->id.id);
  EXPECT_EQ(GURL(kTestUrl), notifier()->offline_item()->page_url);
  EXPECT_EQ(kTestCreationTime, notifier()->offline_item()->creation_time);
}

TEST_F(DownloadNotifyingObserverTest, OnAddedAsOfflining) {
  SavePageRequest request(kTestOfflineId, GURL(kTestUrl), kTestClientId,
                          kTestCreationTime, kTestUserRequested);
  request.set_request_state(SavePageRequest::RequestState::OFFLINING);
  observer()->OnAdded(request);
  EXPECT_EQ(LastNotificationType::DOWNLOAD_PROGRESS,
            notifier()->last_notification_type());
  EXPECT_EQ(kTestGuid, notifier()->offline_item()->id.id);
}

TEST_F(DownloadNotifyingObserverTest, OnAddedAsPaused) {
  SavePageRequest request(kTestOfflineId, GURL(kTestUrl), kTestClientId,
                          kTestCreationTime, kTestUserRequested);
  request.set_request_state(SavePageRequest::RequestState::PAUSED);
  observer()->OnAdded(request);
  EXPECT_EQ(LastNotificationType::DOWNLOAD_PAUSED,
            notifier()->last_notification_type());
  EXPECT_EQ(kTestGuid, notifier()->offline_item()->id.id);
}

TEST_F(DownloadNotifyingObserverTest, OnChangedToPaused) {
  SavePageRequest request(kTestOfflineId, GURL(kTestUrl), kTestClientId,
                          kTestCreationTime, kTestUserRequested);
  request.set_request_state(SavePageRequest::RequestState::PAUSED);
  observer()->OnChanged(request);
  EXPECT_EQ(LastNotificationType::DOWNLOAD_PAUSED,
            notifier()->last_notification_type());
  EXPECT_EQ(kTestGuid, notifier()->offline_item()->id.id);
  EXPECT_EQ(GURL(kTestUrl), notifier()->offline_item()->page_url);
  EXPECT_EQ(kTestCreationTime, notifier()->offline_item()->creation_time);
}

TEST_F(DownloadNotifyingObserverTest, OnChangedToAvailable) {
  SavePageRequest request(kTestOfflineId, GURL(kTestUrl), kTestClientId,
                          kTestCreationTime, kTestUserRequested);
  request.set_request_state(SavePageRequest::RequestState::AVAILABLE);
  observer()->OnChanged(request);
  EXPECT_EQ(LastNotificationType::DOWNLOAD_INTERRUPTED,
            notifier()->last_notification_type());
  EXPECT_EQ(kTestGuid, notifier()->offline_item()->id.id);
  EXPECT_EQ(GURL(kTestUrl), notifier()->offline_item()->page_url);
  EXPECT_EQ(kTestCreationTime, notifier()->offline_item()->creation_time);
}

TEST_F(DownloadNotifyingObserverTest, OnChangedToOfflining) {
  SavePageRequest request(kTestOfflineId, GURL(kTestUrl), kTestClientId,
                          kTestCreationTime, kTestUserRequested);
  request.set_request_state(SavePageRequest::RequestState::OFFLINING);
  observer()->OnChanged(request);
  EXPECT_EQ(LastNotificationType::DOWNLOAD_PROGRESS,
            notifier()->last_notification_type());
  EXPECT_EQ(kTestGuid, notifier()->offline_item()->id.id);
  EXPECT_EQ(GURL(kTestUrl), notifier()->offline_item()->page_url);
  EXPECT_EQ(kTestCreationTime, notifier()->offline_item()->creation_time);
}

TEST_F(DownloadNotifyingObserverTest, OnCompletedSuccess) {
  SavePageRequest request(kTestOfflineId, GURL(kTestUrl), kTestClientId,
                          kTestCreationTime, kTestUserRequested);
  observer()->OnCompleted(request,
                          RequestNotifier::BackgroundSavePageResult::SUCCESS);
  EXPECT_EQ(LastNotificationType::DOWNLOAD_SUCCESSFUL,
            notifier()->last_notification_type());
  EXPECT_EQ(kTestGuid, notifier()->offline_item()->id.id);
  EXPECT_EQ(GURL(kTestUrl), notifier()->offline_item()->page_url);
  EXPECT_EQ(kTestCreationTime, notifier()->offline_item()->creation_time);
}

TEST_F(DownloadNotifyingObserverTest, OnCompletedSuccess_SuppressNotification) {
  SavePageRequest request(kTestOfflineId, GURL(kTestUrl), kTestClientId,
                          kTestCreationTime, kTestUserRequested);
  notifier()->SetShouldSuppressNotification(true);
  observer()->OnCompleted(request,
                          RequestNotifier::BackgroundSavePageResult::SUCCESS);
  EXPECT_EQ(LastNotificationType::DOWNLOAD_SUPPRESSED,
            notifier()->last_notification_type());
  EXPECT_EQ(kTestGuid, notifier()->offline_item()->id.id);
  EXPECT_EQ(GURL(kTestUrl), notifier()->offline_item()->page_url);
  EXPECT_EQ(kTestCreationTime, notifier()->offline_item()->creation_time);
}

TEST_F(DownloadNotifyingObserverTest, OnCompletedCanceled) {
  SavePageRequest request(kTestOfflineId, GURL(kTestUrl), kTestClientId,
                          kTestCreationTime, kTestUserRequested);
  observer()->OnCompleted(
      request, RequestNotifier::BackgroundSavePageResult::USER_CANCELED);
  EXPECT_EQ(LastNotificationType::DOWNLOAD_CANCELED,
            notifier()->last_notification_type());
  EXPECT_EQ(kTestGuid, notifier()->offline_item()->id.id);
  EXPECT_EQ(GURL(kTestUrl), notifier()->offline_item()->page_url);
  EXPECT_EQ(kTestCreationTime, notifier()->offline_item()->creation_time);
}

TEST_F(DownloadNotifyingObserverTest, OnCompletedFailure) {
  SavePageRequest request(kTestOfflineId, GURL(kTestUrl), kTestClientId,
                          kTestCreationTime, kTestUserRequested);
  observer()->OnCompleted(
      request, RequestNotifier::BackgroundSavePageResult::LOADING_FAILURE);
  EXPECT_EQ(LastNotificationType::DOWNLOAD_FAILED,
            notifier()->last_notification_type());
  EXPECT_EQ(kTestGuid, notifier()->offline_item()->id.id);
  EXPECT_EQ(GURL(kTestUrl), notifier()->offline_item()->page_url);
  EXPECT_EQ(kTestCreationTime, notifier()->offline_item()->creation_time);

  notifier()->Reset();
  observer()->OnCompleted(
      request, RequestNotifier::BackgroundSavePageResult::FOREGROUND_CANCELED);
  EXPECT_EQ(LastNotificationType::DOWNLOAD_FAILED,
            notifier()->last_notification_type());
  EXPECT_EQ(kTestGuid, notifier()->offline_item()->id.id);
  EXPECT_EQ(GURL(kTestUrl), notifier()->offline_item()->page_url);
  EXPECT_EQ(kTestCreationTime, notifier()->offline_item()->creation_time);

  notifier()->Reset();
  observer()->OnCompleted(
      request, RequestNotifier::BackgroundSavePageResult::SAVE_FAILED);
  EXPECT_EQ(LastNotificationType::DOWNLOAD_FAILED,
            notifier()->last_notification_type());
  EXPECT_EQ(kTestGuid, notifier()->offline_item()->id.id);
  EXPECT_EQ(GURL(kTestUrl), notifier()->offline_item()->page_url);
  EXPECT_EQ(kTestCreationTime, notifier()->offline_item()->creation_time);

  notifier()->Reset();
  observer()->OnCompleted(request,
                          RequestNotifier::BackgroundSavePageResult::EXPIRED);
  EXPECT_EQ(LastNotificationType::DOWNLOAD_FAILED,
            notifier()->last_notification_type());
  EXPECT_EQ(kTestGuid, notifier()->offline_item()->id.id);
  EXPECT_EQ(GURL(kTestUrl), notifier()->offline_item()->page_url);
  EXPECT_EQ(kTestCreationTime, notifier()->offline_item()->creation_time);

  notifier()->Reset();
  observer()->OnCompleted(
      request, RequestNotifier::BackgroundSavePageResult::RETRY_COUNT_EXCEEDED);
  EXPECT_EQ(LastNotificationType::DOWNLOAD_FAILED,
            notifier()->last_notification_type());
  EXPECT_EQ(kTestGuid, notifier()->offline_item()->id.id);
  EXPECT_EQ(GURL(kTestUrl), notifier()->offline_item()->page_url);
  EXPECT_EQ(kTestCreationTime, notifier()->offline_item()->creation_time);
}

TEST_F(DownloadNotifyingObserverTest, NamespacesNotVisibleInUI) {
  std::vector<std::string> name_spaces = {kBookmarkNamespace, kLastNNamespace,
                                          kCCTNamespace, kDefaultNamespace};

  for (auto name_space : name_spaces) {
    ClientId invisible_client_id(name_space, kTestGuid);
    SavePageRequest request(kTestOfflineId, GURL(kTestUrl), invisible_client_id,
                            kTestCreationTime, kTestUserRequested);
    observer()->OnAdded(request);
    EXPECT_EQ(LastNotificationType::NONE, notifier()->last_notification_type());
  }
}

}  // namespace offline_pages
