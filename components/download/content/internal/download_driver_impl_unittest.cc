// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/download/content/internal/download_driver_impl.h"

#include <memory>
#include <string>

#include "base/guid.h"
#include "base/test/test_simple_task_runner.h"
#include "base/threading/thread_task_runner_handle.h"
#include "components/download/content/public/all_download_item_notifier.h"
#include "components/download/internal/background_service/test/mock_download_driver_client.h"
#include "content/public/test/fake_download_item.h"
#include "content/public/test/mock_download_manager.h"
#include "net/http/http_response_headers.h"
#include "net/http/http_util.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

using testing::_;
using testing::Invoke;
using testing::NiceMock;
using testing::Return;

namespace download {

namespace {

ACTION_P(PopulateVector, items) {
  arg0->insert(arg0->begin(), items.begin(), items.end());
}

const char kFakeGuid[] = "fake_guid";

// Matcher to compare driver entries. Not all the memeber fields are compared.
// Currently no comparison in non test code, so no operator== override for
// driver entry.
MATCHER_P(DriverEntryEqual, entry, "") {
  return entry.guid == arg.guid && entry.state == arg.state &&
         entry.done == arg.done &&
         entry.bytes_downloaded == arg.bytes_downloaded &&
         entry.current_file_path.value() == arg.current_file_path.value();
}

}  // namespace

class DownloadDriverImplTest : public testing::Test {
 public:
  DownloadDriverImplTest()
      : task_runner_(new base::TestSimpleTaskRunner), handle_(task_runner_) {}

  ~DownloadDriverImplTest() override = default;

  void SetUp() override {
    EXPECT_CALL(mock_client_, IsTrackingDownload(_))
        .WillRepeatedly(Return(true));
    driver_ = std::make_unique<DownloadDriverImpl>(&mock_manager_);
  }

  // TODO(xingliu): implements test download manager for embedders to test.
  NiceMock<content::MockDownloadManager> mock_manager_;
  MockDriverClient mock_client_;
  std::unique_ptr<DownloadDriverImpl> driver_;

 protected:
  scoped_refptr<base::TestSimpleTaskRunner> task_runner_;
  base::ThreadTaskRunnerHandle handle_;

 private:
  DISALLOW_COPY_AND_ASSIGN(DownloadDriverImplTest);
};

// Ensure the download manager can be initialized after the download driver.
TEST_F(DownloadDriverImplTest, ManagerLateInitialization) {
  EXPECT_CALL(mock_manager_, IsManagerInitialized())
      .Times(1)
      .WillOnce(Return(false));
  driver_->Initialize(&mock_client_);

  EXPECT_CALL(mock_client_, OnDriverReady(true));
  static_cast<AllDownloadItemNotifier::Observer*>(driver_.get())
      ->OnManagerInitialized(&mock_manager_);
}

TEST_F(DownloadDriverImplTest, TestHardRecover) {
  EXPECT_CALL(mock_manager_, IsManagerInitialized())
      .Times(1)
      .WillOnce(Return(false));
  driver_->Initialize(&mock_client_);

  EXPECT_CALL(mock_client_, OnDriverHardRecoverComplete(true)).Times(1);
  driver_->HardRecover();
  task_runner_->RunUntilIdle();
}
// Ensure driver remove call before download created will result in content
// layer remove call and not propagating the event to driver's client.
TEST_F(DownloadDriverImplTest, RemoveBeforeCreated) {
  using DownloadState = download::DownloadItem::DownloadState;

  EXPECT_CALL(mock_manager_, IsManagerInitialized())
      .Times(1)
      .WillOnce(Return(false));
  driver_->Initialize(&mock_client_);

  EXPECT_CALL(mock_client_, OnDriverReady(true));
  static_cast<AllDownloadItemNotifier::Observer*>(driver_.get())
      ->OnManagerInitialized(&mock_manager_);

  const std::string kTestGuid = "abc";
  content::FakeDownloadItem fake_item;
  fake_item.SetGuid(kTestGuid);
  fake_item.SetState(DownloadState::IN_PROGRESS);

  // Download is not created yet in content layer.
  ON_CALL(mock_manager_, GetDownloadByGuid(kTestGuid))
      .WillByDefault(Return(nullptr));
  driver_->Remove(kTestGuid);
  task_runner_->RunUntilIdle();

  // Download is created in content layer.
  ON_CALL(mock_manager_, GetDownloadByGuid(kTestGuid))
      .WillByDefault(Return(&fake_item));
  EXPECT_CALL(mock_client_, OnDownloadCreated(_)).Times(0);
  static_cast<AllDownloadItemNotifier::Observer*>(driver_.get())
      ->OnDownloadCreated(&mock_manager_, &fake_item);
  task_runner_->RunUntilIdle();

  // Expect a remove call down to content layer download item.
  EXPECT_TRUE(fake_item.removed());
}

// Ensures download updates from download items are propagated correctly.
TEST_F(DownloadDriverImplTest, DownloadItemUpdateEvents) {
  using DownloadState = download::DownloadItem::DownloadState;
  using DownloadInterruptReason = download::DownloadInterruptReason;

  EXPECT_CALL(mock_manager_, IsManagerInitialized())
      .Times(1)
      .WillOnce(Return(true));
  EXPECT_CALL(mock_client_, OnDriverReady(true)).Times(1);
  driver_->Initialize(&mock_client_);

  content::FakeDownloadItem fake_item;
  fake_item.SetState(DownloadState::IN_PROGRESS);
  fake_item.SetGuid(kFakeGuid);
  fake_item.SetReceivedBytes(0);
  fake_item.SetTotalBytes(1024);
  DriverEntry entry = DownloadDriverImpl::CreateDriverEntry(&fake_item);

  EXPECT_CALL(mock_client_, OnDownloadUpdated(DriverEntryEqual(entry)))
      .Times(1)
      .RetiresOnSaturation();
  static_cast<AllDownloadItemNotifier::Observer*>(driver_.get())
      ->OnDownloadUpdated(&mock_manager_, &fake_item);

  // Nothing happens for cancelled state.
  fake_item.SetState(DownloadState::CANCELLED);
  static_cast<AllDownloadItemNotifier::Observer*>(driver_.get())
      ->OnDownloadUpdated(&mock_manager_, &fake_item);

  fake_item.SetReceivedBytes(1024);
  fake_item.SetState(DownloadState::COMPLETE);
  entry = DownloadDriverImpl::CreateDriverEntry(&fake_item);
  EXPECT_CALL(mock_client_, OnDownloadSucceeded(DriverEntryEqual(entry)))
      .Times(1)
      .RetiresOnSaturation();
  static_cast<AllDownloadItemNotifier::Observer*>(driver_.get())
      ->OnDownloadUpdated(&mock_manager_, &fake_item);

  fake_item.SetState(DownloadState::INTERRUPTED);
  fake_item.SetLastReason(
      DownloadInterruptReason::DOWNLOAD_INTERRUPT_REASON_SERVER_BAD_CONTENT);
  entry = DownloadDriverImpl::CreateDriverEntry(&fake_item);
  EXPECT_CALL(mock_client_, OnDownloadFailed(DriverEntryEqual(entry),
                                             FailureType::RECOVERABLE))
      .Times(1)
      .RetiresOnSaturation();
  static_cast<AllDownloadItemNotifier::Observer*>(driver_.get())
      ->OnDownloadUpdated(&mock_manager_, &fake_item);
}

TEST_F(DownloadDriverImplTest, TestGetActiveDownloadsCall) {
  using DownloadState = download::DownloadItem::DownloadState;
  content::FakeDownloadItem item1;
  item1.SetState(DownloadState::IN_PROGRESS);
  item1.SetGuid(base::GenerateGUID());

  content::FakeDownloadItem item2;
  item2.SetState(DownloadState::CANCELLED);
  item2.SetGuid(base::GenerateGUID());

  content::FakeDownloadItem item3;
  item3.SetState(DownloadState::COMPLETE);
  item3.SetGuid(base::GenerateGUID());

  content::FakeDownloadItem item4;
  item4.SetState(DownloadState::INTERRUPTED);
  item4.SetGuid(base::GenerateGUID());

  std::vector<download::DownloadItem*> items{&item1, &item2, &item3, &item4};

  ON_CALL(mock_manager_, GetAllDownloads(_))
      .WillByDefault(PopulateVector(items));

  EXPECT_CALL(mock_manager_, IsManagerInitialized())
      .Times(1)
      .WillOnce(Return(true));
  EXPECT_CALL(mock_client_, OnDriverReady(true)).Times(1);
  driver_->Initialize(&mock_client_);

  auto guids = driver_->GetActiveDownloads();

  EXPECT_EQ(1U, guids.size());
  EXPECT_NE(guids.end(), guids.find(item1.GetGuid()));
}

TEST_F(DownloadDriverImplTest, TestCreateDriverEntry) {
  using DownloadState = download::DownloadItem::DownloadState;
  content::FakeDownloadItem item;
  const std::string kGuid("dummy guid");
  const std::vector<GURL> kUrls = {GURL("http://www.example.com/foo.html"),
                                   GURL("http://www.example.com/bar.html")};
  scoped_refptr<net::HttpResponseHeaders> headers =
      new net::HttpResponseHeaders("HTTP/1.1 201\n");

  item.SetGuid(kGuid);
  item.SetUrlChain(kUrls);
  item.SetState(DownloadState::IN_PROGRESS);
  item.SetResponseHeaders(headers);

  DriverEntry entry = driver_->CreateDriverEntry(&item);

  EXPECT_EQ(kGuid, entry.guid);
  EXPECT_EQ(kUrls, entry.url_chain);
  EXPECT_EQ(DriverEntry::State::IN_PROGRESS, entry.state);
  EXPECT_EQ(headers, entry.response_headers);
}

}  // namespace download
