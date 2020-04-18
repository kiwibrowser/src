// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <memory>
#include <string>

#include "base/macros.h"
#include "base/test/histogram_tester.h"
#include "chrome/browser/external_protocol/external_protocol_handler.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/external_protocol_dialog_delegate.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/browser/ui/test/test_browser_dialog.h"
#include "chrome/browser/ui/views/external_protocol_dialog.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/render_process_host.h"
#include "content/public/browser/render_view_host.h"
#include "content/public/browser/web_contents.h"
#include "ui/views/controls/button/checkbox.h"
#include "url/gurl.h"

namespace test {

class ExternalProtocolDialogTestApi {
 public:
  explicit ExternalProtocolDialogTestApi(ExternalProtocolDialog* dialog)
      : dialog_(dialog) {}

  void SetCheckBoxSelected(bool checked) {
    dialog_->remember_decision_checkbox_->SetChecked(checked);
  }

 private:
  ExternalProtocolDialog* dialog_;

  DISALLOW_COPY_AND_ASSIGN(ExternalProtocolDialogTestApi);
};

}  // namespace test

// Wrapper dialog delegate that sets |called|, |accept|, |cancel|, and
// |remember| bools based on what is called by the ExternalProtocolDialog.
class TestExternalProtocolDialogDelegate
    : public ExternalProtocolDialogDelegate {
 public:
  TestExternalProtocolDialogDelegate(const GURL& url,
                                     int render_process_host_id,
                                     int routing_id,
                                     bool* called,
                                     bool* accept,
                                     bool* remember)
      : ExternalProtocolDialogDelegate(url, render_process_host_id, routing_id),
        called_(called),
        accept_(accept),
        remember_(remember) {}

  // ExternalProtocolDialogDelegate:
  void DoAccept(const GURL& url, bool remember) const override {
    *called_ = true;
    *accept_ = true;
    *remember_ = remember;
    ExternalProtocolDialogDelegate::DoAccept(url, remember);
  }

 private:
  bool* called_;
  bool* accept_;
  bool* remember_;

  DISALLOW_COPY_AND_ASSIGN(TestExternalProtocolDialogDelegate);
};

class ExternalProtocolDialogBrowserTest
    : public DialogBrowserTest,
      public ExternalProtocolHandler::Delegate {
 public:
  ExternalProtocolDialogBrowserTest() {
    ExternalProtocolHandler::SetDelegateForTesting(this);
  }

  ~ExternalProtocolDialogBrowserTest() override {
    ExternalProtocolHandler::SetDelegateForTesting(nullptr);
  }

  // DialogBrowserTest:
  void ShowUi(const std::string& name) override {
    content::WebContents* web_contents =
        browser()->tab_strip_model()->GetActiveWebContents();
    int render_view_process_id =
        web_contents->GetRenderViewHost()->GetProcess()->GetID();
    int render_view_routing_id =
        web_contents->GetRenderViewHost()->GetRoutingID();
    dialog_ = new ExternalProtocolDialog(
        std::make_unique<TestExternalProtocolDialogDelegate>(
            GURL("telnet://12345"), render_view_process_id,
            render_view_routing_id, &called_, &accept_, &remember_),
        render_view_process_id, render_view_routing_id);
  }

  void SetChecked(bool checked) {
    test::ExternalProtocolDialogTestApi(dialog_).SetCheckBoxSelected(checked);
  }

  // ExternalProtocolHander::Delegate:
  scoped_refptr<shell_integration::DefaultProtocolClientWorker>
  CreateShellWorker(
      const shell_integration::DefaultWebClientWorkerCallback& callback,
      const std::string& protocol) override {
    return nullptr;
  }
  ExternalProtocolHandler::BlockState GetBlockState(const std::string& scheme,
                                                    Profile* profile) override {
    return ExternalProtocolHandler::DONT_BLOCK;
  }
  void BlockRequest() override {}
  void RunExternalProtocolDialog(const GURL& url,
                                 int render_process_host_id,
                                 int routing_id,
                                 ui::PageTransition page_transition,
                                 bool has_user_gesture) override {}
  void LaunchUrlWithoutSecurityCheck(
      const GURL& url,
      content::WebContents* web_contents) override {
    url_did_launch_ = true;
  }
  void FinishedProcessingCheck() override {}

  base::HistogramTester histogram_tester_;

 protected:
  ExternalProtocolDialog* dialog_ = nullptr;
  bool called_ = false;
  bool accept_ = false;
  bool remember_ = false;
  bool url_did_launch_ = false;

 private:
  DISALLOW_COPY_AND_ASSIGN(ExternalProtocolDialogBrowserTest);
};

IN_PROC_BROWSER_TEST_F(ExternalProtocolDialogBrowserTest, TestAccept) {
  ShowUi(std::string());
  EXPECT_TRUE(dialog_->Accept());
  EXPECT_TRUE(called_);
  EXPECT_TRUE(accept_);
  EXPECT_FALSE(remember_);
  EXPECT_TRUE(url_did_launch_);
  histogram_tester_.ExpectBucketCount(
      ExternalProtocolHandler::kHandleStateMetric,
      ExternalProtocolHandler::LAUNCH, 1);
}

IN_PROC_BROWSER_TEST_F(ExternalProtocolDialogBrowserTest,
                       TestAcceptWithChecked) {
  ShowUi(std::string());
  SetChecked(true);
  EXPECT_TRUE(dialog_->Accept());
  EXPECT_TRUE(called_);
  EXPECT_TRUE(accept_);
  EXPECT_TRUE(remember_);
  EXPECT_TRUE(url_did_launch_);
  histogram_tester_.ExpectBucketCount(
      ExternalProtocolHandler::kHandleStateMetric,
      ExternalProtocolHandler::CHECKED_LAUNCH, 1);
}

// Regression test for http://crbug.com/835216. The OS owns the dialog, so it
// may may outlive the WebContents it is attached to.
IN_PROC_BROWSER_TEST_F(ExternalProtocolDialogBrowserTest,
                       TestAcceptAfterCloseTab) {
  ShowUi(std::string());
  SetChecked(true);  // |remember_| must be true for the segfault to occur.
  browser()->tab_strip_model()->CloseAllTabs();
  EXPECT_TRUE(dialog_->Accept());
  EXPECT_TRUE(called_);
  EXPECT_TRUE(accept_);
  EXPECT_TRUE(remember_);
  EXPECT_FALSE(url_did_launch_);
  histogram_tester_.ExpectBucketCount(
      ExternalProtocolHandler::kHandleStateMetric,
      ExternalProtocolHandler::DONT_LAUNCH, 1);
}

IN_PROC_BROWSER_TEST_F(ExternalProtocolDialogBrowserTest, TestCancel) {
  ShowUi(std::string());
  EXPECT_TRUE(dialog_->Cancel());
  EXPECT_FALSE(called_);
  EXPECT_FALSE(accept_);
  EXPECT_FALSE(remember_);
  EXPECT_FALSE(url_did_launch_);
  histogram_tester_.ExpectBucketCount(
      ExternalProtocolHandler::kHandleStateMetric,
      ExternalProtocolHandler::DONT_LAUNCH, 1);
}

IN_PROC_BROWSER_TEST_F(ExternalProtocolDialogBrowserTest,
                       TestCancelWithChecked) {
  ShowUi(std::string());
  SetChecked(true);
  EXPECT_TRUE(dialog_->Cancel());
  EXPECT_FALSE(called_);
  EXPECT_FALSE(accept_);
  EXPECT_FALSE(remember_);
  EXPECT_FALSE(url_did_launch_);
  histogram_tester_.ExpectBucketCount(
      ExternalProtocolHandler::kHandleStateMetric,
      ExternalProtocolHandler::DONT_LAUNCH, 1);
}

IN_PROC_BROWSER_TEST_F(ExternalProtocolDialogBrowserTest, TestClose) {
  // Closing the dialog should be the same as canceling, except for histograms.
  ShowUi(std::string());
  EXPECT_TRUE(dialog_->Close());
  EXPECT_FALSE(called_);
  EXPECT_FALSE(accept_);
  EXPECT_FALSE(remember_);
  EXPECT_FALSE(url_did_launch_);
  histogram_tester_.ExpectBucketCount(
      ExternalProtocolHandler::kHandleStateMetric,
      ExternalProtocolHandler::DONT_LAUNCH, 1);
}

IN_PROC_BROWSER_TEST_F(ExternalProtocolDialogBrowserTest,
                       TestCloseWithChecked) {
  // Closing the dialog should be the same as canceling, except for histograms.
  ShowUi(std::string());
  SetChecked(true);
  EXPECT_TRUE(dialog_->Close());
  EXPECT_FALSE(called_);
  EXPECT_FALSE(accept_);
  EXPECT_FALSE(remember_);
  EXPECT_FALSE(url_did_launch_);
  histogram_tester_.ExpectBucketCount(
      ExternalProtocolHandler::kHandleStateMetric,
      ExternalProtocolHandler::DONT_LAUNCH, 1);
}

// Invokes a dialog that asks the user if an external application is allowed to
// run.
IN_PROC_BROWSER_TEST_F(ExternalProtocolDialogBrowserTest, InvokeUi_default) {
  ShowAndVerifyUi();
}
