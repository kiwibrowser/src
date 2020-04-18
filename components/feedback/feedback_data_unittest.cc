// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/feedback/feedback_data.h"

#include <memory>

#include "base/memory/ref_counted.h"
#include "base/run_loop.h"
#include "components/feedback/feedback_report.h"
#include "components/feedback/feedback_uploader.h"
#include "components/feedback/feedback_uploader_factory.h"
#include "components/prefs/testing_pref_service.h"
#include "content/public/test/test_browser_context.h"
#include "content/public/test/test_browser_thread_bundle.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace feedback {

namespace {

constexpr char kHistograms[] = "";
constexpr char kImageData[] = "";
constexpr char kFileData[] = "";

class MockUploader : public FeedbackUploader {
 public:
  MockUploader(content::BrowserContext* context,
               const base::Closure& on_report_sent)
      : FeedbackUploader(context,
                         FeedbackUploaderFactory::CreateUploaderTaskRunner()),
        on_report_sent_(on_report_sent) {}
  ~MockUploader() override {}

  // feedback::FeedbackUploader:
  void StartDispatchingReport() override { on_report_sent_.Run(); }

 private:
  base::Closure on_report_sent_;

  DISALLOW_COPY_AND_ASSIGN(MockUploader);
};

std::unique_ptr<std::string> MakeScoped(const char* str) {
  return std::make_unique<std::string>(str);
}

}  // namespace

class FeedbackDataTest : public testing::Test {
 protected:
  FeedbackDataTest()
      : uploader_(&context_,
                  base::Bind(&FeedbackDataTest::set_send_report_callback,
                             base::Unretained(this))),
        data_(base::MakeRefCounted<FeedbackData>(&uploader_)) {}
  ~FeedbackDataTest() override = default;

  void Send() {
    bool attached_file_completed =
        data_->attached_file_uuid().empty();
    bool screenshot_completed =
        data_->screenshot_uuid().empty();

    if (screenshot_completed && attached_file_completed) {
      data_->OnFeedbackPageDataComplete();
    }
  }

  void RunMessageLoop() {
    run_loop_.reset(new base::RunLoop());
    quit_closure_ = run_loop_->QuitClosure();
    run_loop_->Run();
  }

  void set_send_report_callback() { quit_closure_.Run(); }

  base::Closure quit_closure_;
  std::unique_ptr<base::RunLoop> run_loop_;
  content::TestBrowserThreadBundle test_browser_thread_bundle_;
  content::TestBrowserContext context_;
  MockUploader uploader_;
  scoped_refptr<FeedbackData> data_;
};

TEST_F(FeedbackDataTest, ReportSending) {
  data_->SetAndCompressHistograms(MakeScoped(kHistograms));
  data_->set_image(MakeScoped(kImageData));
  data_->AttachAndCompressFileData(MakeScoped(kFileData));
  Send();
  RunMessageLoop();
  EXPECT_TRUE(data_->IsDataComplete());
}

}  // namespace feedback
