// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/safe_browsing/notification_image_reporter.h"

#include "base/callback.h"
#include "base/memory/ptr_util.h"
#include "base/run_loop.h"
#include "base/test/scoped_feature_list.h"
#include "chrome/browser/safe_browsing/ping_manager.h"
#include "chrome/browser/safe_browsing/test_safe_browsing_service.h"
#include "chrome/test/base/testing_browser_process.h"
#include "chrome/test/base/testing_profile.h"
#include "components/safe_browsing/common/safe_browsing_prefs.h"
#include "components/safe_browsing/db/test_database_manager.h"
#include "components/safe_browsing/proto/csd.pb.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/test/test_browser_thread_bundle.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/skia/include/core/SkBitmap.h"
#include "third_party/skia/include/core/SkColor.h"
#include "url/gurl.h"

using content::BrowserThread;

using testing::_;
using testing::Return;

namespace safe_browsing {

namespace {

class TestingNotificationImageReporter : public NotificationImageReporter {
 public:
  TestingNotificationImageReporter() : NotificationImageReporter(nullptr) {}

  void WaitForReportSkipped() {
    base::RunLoop run_loop;
    skipped_quit_closure_ = run_loop.QuitClosure();
    run_loop.Run();
  }

  void WaitForReportSent() {
    base::RunLoop run_loop;
    send_quit_closure_ = run_loop.QuitClosure();
    run_loop.Run();
  }

  void SetReportingChance(bool reporting_chance) {
    reporting_chance_ = reporting_chance;
  }

  int sent_report_count() { return sent_report_count_; }
  const GURL& last_report_url() { return last_report_url_; }
  const std::string& last_content_type() { return last_content_type_; }
  const std::string& last_report() { return last_report_; }

 protected:
  double GetReportChance() const override { return reporting_chance_; }
  void SkippedReporting() override {
    if (skipped_quit_closure_) {
      skipped_quit_closure_.Run();
      skipped_quit_closure_.Reset();
    }
  }

  void SendReportInternal(const GURL& url,
                          const std::string& content_type,
                          const std::string& report) override {
    sent_report_count_++;
    last_report_url_ = url;
    last_content_type_ = content_type;
    last_report_ = report;
    if (send_quit_closure_) {
      send_quit_closure_.Run();
      send_quit_closure_.Reset();
    }
  }

 private:
  base::Closure send_quit_closure_;
  base::Closure skipped_quit_closure_;
  double reporting_chance_ = 1.0;
  int sent_report_count_ = 0;
  GURL last_report_url_;
  std::string last_content_type_;
  std::string last_report_;
};

class MockSafeBrowsingDatabaseManager : public TestSafeBrowsingDatabaseManager {
 public:
  MOCK_METHOD2(CheckCsdWhitelistUrl,
               AsyncMatch(const GURL&, SafeBrowsingDatabaseManager::Client*));

 protected:
  ~MockSafeBrowsingDatabaseManager() override {}
};

SkBitmap CreateBitmap(int width, int height) {
  SkBitmap bitmap;
  bitmap.allocN32Pixels(width, height);
  bitmap.eraseColor(SK_ColorGREEN);
  return bitmap;
}

}  // namespace

class NotificationImageReporterTest : public ::testing::Test {
 public:
  NotificationImageReporterTest();

  void SetUp() override;
  void TearDown() override;

 private:
  content::TestBrowserThreadBundle thread_bundle_;  // Should be first member.

 protected:
  void SetExtendedReportingLevel(ExtendedReportingLevel level);
  void ReportNotificationImage();

  scoped_refptr<SafeBrowsingService> safe_browsing_service_;

  std::unique_ptr<TestingProfile> profile_;

  TestingNotificationImageReporter* notification_image_reporter_;

  std::unique_ptr<base::test::ScopedFeatureList> feature_list_;

  GURL origin_;     // Written on UI, read on IO.
  SkBitmap image_;  // Written on UI, read on IO.

  scoped_refptr<MockSafeBrowsingDatabaseManager> mock_database_manager_;
};

NotificationImageReporterTest::NotificationImageReporterTest()
    // Use REAL_IO_THREAD so DCHECK_CURRENTLY_ON distinguishes IO from UI.
    : thread_bundle_(content::TestBrowserThreadBundle::REAL_IO_THREAD),
      origin_("https://example.com") {
  image_ = CreateBitmap(1 /* w */, 1 /* h */);
}

void NotificationImageReporterTest::SetUp() {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);

  // Initialize SafeBrowsingService with FakeSafeBrowsingDatabaseManager.
  TestSafeBrowsingServiceFactory sb_service_factory;

  mock_database_manager_ = new MockSafeBrowsingDatabaseManager();
  ON_CALL(*mock_database_manager_.get(), CheckCsdWhitelistUrl(_, _))
      .WillByDefault(Return(AsyncMatch::NO_MATCH));
  sb_service_factory.SetTestDatabaseManager(mock_database_manager_.get());
  SafeBrowsingService::RegisterFactory(&sb_service_factory);
  safe_browsing_service_ = sb_service_factory.CreateSafeBrowsingService();
  SafeBrowsingService::RegisterFactory(nullptr);
  TestingBrowserProcess::GetGlobal()->SetSafeBrowsingService(
      safe_browsing_service_.get());
  g_browser_process->safe_browsing_service()->Initialize();
  base::RunLoop().RunUntilIdle();  // TODO(johnme): Might still be tasks on IO.

  profile_ = std::make_unique<TestingProfile>();

  notification_image_reporter_ = new TestingNotificationImageReporter();
  safe_browsing_service_->ping_manager()->notification_image_reporter_ =
      base::WrapUnique(notification_image_reporter_);
}

void NotificationImageReporterTest::TearDown() {
  TestingBrowserProcess::GetGlobal()->safe_browsing_service()->ShutDown();
  // Ensure no races between internal SafeBrowsingService's IO thread
  // initialization that will create a NetworkChangeNotifier, which is also used
  // by Profile's destructor.
  thread_bundle_.RunIOThreadUntilIdle();
  thread_bundle_.RunUntilIdle();
  TestingBrowserProcess::GetGlobal()->SetSafeBrowsingService(nullptr);
}

void NotificationImageReporterTest::SetExtendedReportingLevel(
    ExtendedReportingLevel level) {
  feature_list_ = std::make_unique<base::test::ScopedFeatureList>();
  if (level != SBER_LEVEL_SCOUT)
    // Explicitly disable CanShowScoutOptIn, which is on by default.
    feature_list_->InitWithFeatures({}, {safe_browsing::kCanShowScoutOptIn});

  InitializeSafeBrowsingPrefs(profile_->GetPrefs());
  SetExtendedReportingPref(profile_->GetPrefs(), level != SBER_LEVEL_OFF);
}

void NotificationImageReporterTest::ReportNotificationImage() {
  if (!safe_browsing_service_->enabled_by_prefs())
    return;
  safe_browsing_service_->ping_manager()->ReportNotificationImage(
      profile_.get(), safe_browsing_service_->database_manager(), origin_,
      image_);
}

// Disabled due to data race. https://crbug.com/836359
TEST_F(NotificationImageReporterTest, DISABLED_ReportSuccess) {
  SetExtendedReportingLevel(SBER_LEVEL_SCOUT);

  ReportNotificationImage();
  notification_image_reporter_->WaitForReportSent();

  EXPECT_EQ(GURL(NotificationImageReporter::kReportingUploadUrl),
            notification_image_reporter_->last_report_url());
  EXPECT_EQ("application/octet-stream",
            notification_image_reporter_->last_content_type());

  NotificationImageReportRequest report;
  ASSERT_TRUE(
      report.ParseFromString(notification_image_reporter_->last_report()));
  EXPECT_EQ(origin_.spec(), report.notification_origin());
  ASSERT_TRUE(report.has_image());
  EXPECT_GT(report.image().data().size(), 0U);
  ASSERT_TRUE(report.image().has_mime_type());
  EXPECT_EQ(report.image().mime_type(), "image/png");
  ASSERT_TRUE(report.image().has_dimensions());
  EXPECT_EQ(1, report.image().dimensions().width());
  EXPECT_EQ(1, report.image().dimensions().height());
  EXPECT_FALSE(report.image().has_original_dimensions());
}

// Disabled due to data race. https://crbug.com/836359
TEST_F(NotificationImageReporterTest, DISABLED_ImageDownscaling) {
  SetExtendedReportingLevel(SBER_LEVEL_SCOUT);

  image_ = CreateBitmap(640 /* w */, 360 /* h */);

  ReportNotificationImage();
  notification_image_reporter_->WaitForReportSent();

  NotificationImageReportRequest report;
  ASSERT_TRUE(
      report.ParseFromString(notification_image_reporter_->last_report()));
  ASSERT_TRUE(report.has_image());
  EXPECT_GT(report.image().data().size(), 0U);
  ASSERT_TRUE(report.image().has_dimensions());
  EXPECT_EQ(512, report.image().dimensions().width());
  EXPECT_EQ(288, report.image().dimensions().height());
  ASSERT_TRUE(report.image().has_original_dimensions());
  EXPECT_EQ(640, report.image().original_dimensions().width());
  EXPECT_EQ(360, report.image().original_dimensions().height());
}

// Disabled due to data race. https://crbug.com/836359
TEST_F(NotificationImageReporterTest, DISABLED_NoReportWithoutSBER) {
  SetExtendedReportingLevel(SBER_LEVEL_OFF);

  ReportNotificationImage();
  notification_image_reporter_->WaitForReportSkipped();

  EXPECT_EQ(0, notification_image_reporter_->sent_report_count());
}

// Disabled due to data race. https://crbug.com/836359
TEST_F(NotificationImageReporterTest, DISABLED_NoReportWithoutScout) {
  SetExtendedReportingLevel(SBER_LEVEL_LEGACY);

  ReportNotificationImage();
  notification_image_reporter_->WaitForReportSkipped();

  EXPECT_EQ(0, notification_image_reporter_->sent_report_count());
}

// Disabled due to flakiness: crbug.com/831563
TEST_F(NotificationImageReporterTest,
       DISABLED_NoReportWithoutReportingEnabled) {
  SetExtendedReportingLevel(SBER_LEVEL_SCOUT);
  notification_image_reporter_->SetReportingChance(0.0);

  ReportNotificationImage();
  notification_image_reporter_->WaitForReportSkipped();

  EXPECT_EQ(0, notification_image_reporter_->sent_report_count());
}

TEST_F(NotificationImageReporterTest, NoReportOnWhitelistedUrl) {
  SetExtendedReportingLevel(SBER_LEVEL_SCOUT);

  EXPECT_CALL(*mock_database_manager_.get(), CheckCsdWhitelistUrl(origin_, _))
      .WillOnce(Return(AsyncMatch::MATCH));

  ReportNotificationImage();
  notification_image_reporter_->WaitForReportSkipped();

  EXPECT_EQ(0, notification_image_reporter_->sent_report_count());
}

// Disabled due to data race. https://crbug.com/836359
TEST_F(NotificationImageReporterTest, DISABLED_MaxReportsPerDay) {
  SetExtendedReportingLevel(SBER_LEVEL_SCOUT);

  const int kMaxReportsPerDay = 5;

  for (int i = 0; i < kMaxReportsPerDay; i++) {
    ReportNotificationImage();
    notification_image_reporter_->WaitForReportSent();
  }
  ReportNotificationImage();
  notification_image_reporter_->WaitForReportSkipped();

  EXPECT_EQ(kMaxReportsPerDay,
            notification_image_reporter_->sent_report_count());
}

}  // namespace safe_browsing
