// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <stdint.h>

#include <vector>

#include "base/macros.h"
#include "base/memory/ref_counted_memory.h"
#include "chrome/browser/printing/print_preview_dialog_controller.h"
#include "chrome/browser/printing/print_preview_test.h"
#include "chrome/browser/printing/print_view_manager.h"
#include "chrome/browser/ui/browser_commands.h"
#include "chrome/browser/ui/browser_tabstrip.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/browser/ui/webui/print_preview/print_preview_ui.h"
#include "chrome/test/base/browser_with_test_window_test.h"
#include "components/prefs/pref_service.h"
#include "components/web_modal/web_contents_modal_dialog_manager.h"
#include "content/public/browser/plugin_service.h"
#include "content/public/browser/site_instance.h"
#include "content/public/browser/web_contents.h"
#include "printing/print_job_constants.h"

using content::WebContents;
using web_modal::WebContentsModalDialogManager;

namespace {

scoped_refptr<base::RefCountedBytes> CreateTestData() {
  const unsigned char blob1[] =
      "12346102356120394751634516591348710478123649165419234519234512349134";
  std::vector<unsigned char> preview_data(blob1, blob1 + sizeof(blob1));
  return base::MakeRefCounted<base::RefCountedBytes>(preview_data);
}

bool IsShowingWebContentsModalDialog(WebContents* tab) {
  WebContentsModalDialogManager* web_contents_modal_dialog_manager =
      WebContentsModalDialogManager::FromWebContents(tab);
  return web_contents_modal_dialog_manager->IsDialogActive();
}

}  // namespace

class PrintPreviewUIUnitTest : public PrintPreviewTest {
 public:
  PrintPreviewUIUnitTest();
  ~PrintPreviewUIUnitTest() override;

 protected:
  void SetUp() override;

 private:
  DISALLOW_COPY_AND_ASSIGN(PrintPreviewUIUnitTest);
};

PrintPreviewUIUnitTest::PrintPreviewUIUnitTest() {}
PrintPreviewUIUnitTest::~PrintPreviewUIUnitTest() {}

void PrintPreviewUIUnitTest::SetUp() {
  PrintPreviewTest::SetUp();

  chrome::NewTab(browser());
}

// Create/Get a preview tab for initiator.
TEST_F(PrintPreviewUIUnitTest, PrintPreviewData) {
  WebContents* initiator = browser()->tab_strip_model()->GetActiveWebContents();
  ASSERT_TRUE(initiator);
  EXPECT_FALSE(IsShowingWebContentsModalDialog(initiator));

  printing::PrintPreviewDialogController* controller =
      printing::PrintPreviewDialogController::GetInstance();
  ASSERT_TRUE(controller);

  printing::PrintViewManager* print_view_manager =
      printing::PrintViewManager::FromWebContents(initiator);
  print_view_manager->PrintPreviewNow(initiator->GetMainFrame(), false);
  WebContents* preview_dialog = controller->GetOrCreatePreviewDialog(initiator);

  EXPECT_NE(initiator, preview_dialog);
  EXPECT_EQ(1, browser()->tab_strip_model()->count());
  EXPECT_TRUE(IsShowingWebContentsModalDialog(initiator));

  PrintPreviewUI* preview_ui = static_cast<PrintPreviewUI*>(
      preview_dialog->GetWebUI()->GetController());
  ASSERT_TRUE(preview_ui);

  scoped_refptr<base::RefCountedMemory> data;
  preview_ui->GetPrintPreviewDataForIndex(
      printing::COMPLETE_PREVIEW_DOCUMENT_INDEX,
      &data);
  EXPECT_FALSE(data);

  scoped_refptr<base::RefCountedBytes> dummy_data = CreateTestData();

  preview_ui->SetPrintPreviewDataForIndex(
      printing::COMPLETE_PREVIEW_DOCUMENT_INDEX,
      dummy_data.get());
  preview_ui->GetPrintPreviewDataForIndex(
      printing::COMPLETE_PREVIEW_DOCUMENT_INDEX,
      &data);
  EXPECT_EQ(dummy_data->size(), data->size());
  EXPECT_EQ(dummy_data.get(), data.get());

  // This should not cause any memory leaks.
  dummy_data = base::MakeRefCounted<base::RefCountedBytes>();
  preview_ui->SetPrintPreviewDataForIndex(printing::FIRST_PAGE_INDEX,
                                          dummy_data.get());

  // Clear the preview data.
  preview_ui->ClearAllPreviewData();

  preview_ui->GetPrintPreviewDataForIndex(
      printing::COMPLETE_PREVIEW_DOCUMENT_INDEX,
      &data);
  EXPECT_FALSE(data);
}

// Set and get the individual draft pages.
TEST_F(PrintPreviewUIUnitTest, PrintPreviewDraftPages) {
  WebContents* initiator = browser()->tab_strip_model()->GetActiveWebContents();
  ASSERT_TRUE(initiator);

  printing::PrintPreviewDialogController* controller =
      printing::PrintPreviewDialogController::GetInstance();
  ASSERT_TRUE(controller);

  printing::PrintViewManager* print_view_manager =
      printing::PrintViewManager::FromWebContents(initiator);
  print_view_manager->PrintPreviewNow(initiator->GetMainFrame(), false);
  WebContents* preview_dialog = controller->GetOrCreatePreviewDialog(initiator);

  EXPECT_NE(initiator, preview_dialog);
  EXPECT_EQ(1, browser()->tab_strip_model()->count());
  EXPECT_TRUE(IsShowingWebContentsModalDialog(initiator));

  PrintPreviewUI* preview_ui = static_cast<PrintPreviewUI*>(
      preview_dialog->GetWebUI()->GetController());
  ASSERT_TRUE(preview_ui);

  scoped_refptr<base::RefCountedMemory> data;
  preview_ui->GetPrintPreviewDataForIndex(printing::FIRST_PAGE_INDEX, &data);
  EXPECT_FALSE(data);

  scoped_refptr<base::RefCountedBytes> dummy_data = CreateTestData();

  preview_ui->SetPrintPreviewDataForIndex(printing::FIRST_PAGE_INDEX,
                                          dummy_data.get());
  preview_ui->GetPrintPreviewDataForIndex(printing::FIRST_PAGE_INDEX, &data);
  EXPECT_EQ(dummy_data->size(), data->size());
  EXPECT_EQ(dummy_data.get(), data.get());

  // Set and get the third page data.
  preview_ui->SetPrintPreviewDataForIndex(printing::FIRST_PAGE_INDEX + 2,
                                          dummy_data.get());
  preview_ui->GetPrintPreviewDataForIndex(printing::FIRST_PAGE_INDEX + 2,
                                          &data);
  EXPECT_EQ(dummy_data->size(), data->size());
  EXPECT_EQ(dummy_data.get(), data.get());

  // Get the second page data.
  preview_ui->GetPrintPreviewDataForIndex(printing::FIRST_PAGE_INDEX + 1,
                                          &data);
  EXPECT_FALSE(data);

  preview_ui->SetPrintPreviewDataForIndex(printing::FIRST_PAGE_INDEX + 1,
                                          dummy_data.get());
  preview_ui->GetPrintPreviewDataForIndex(printing::FIRST_PAGE_INDEX + 1,
                                          &data);
  EXPECT_EQ(dummy_data->size(), data->size());
  EXPECT_EQ(dummy_data.get(), data.get());

  // Clear the preview data.
  preview_ui->ClearAllPreviewData();
  preview_ui->GetPrintPreviewDataForIndex(printing::FIRST_PAGE_INDEX, &data);
  EXPECT_FALSE(data);
}

// Test the browser-side print preview cancellation functionality.
TEST_F(PrintPreviewUIUnitTest, GetCurrentPrintPreviewStatus) {
  WebContents* initiator = browser()->tab_strip_model()->GetActiveWebContents();
  ASSERT_TRUE(initiator);

  printing::PrintPreviewDialogController* controller =
      printing::PrintPreviewDialogController::GetInstance();
  ASSERT_TRUE(controller);

  printing::PrintViewManager* print_view_manager =
      printing::PrintViewManager::FromWebContents(initiator);
  print_view_manager->PrintPreviewNow(initiator->GetMainFrame(), false);
  WebContents* preview_dialog = controller->GetOrCreatePreviewDialog(initiator);

  EXPECT_NE(initiator, preview_dialog);
  EXPECT_EQ(1, browser()->tab_strip_model()->count());
  EXPECT_TRUE(IsShowingWebContentsModalDialog(initiator));

  PrintPreviewUI* preview_ui = static_cast<PrintPreviewUI*>(
      preview_dialog->GetWebUI()->GetController());
  ASSERT_TRUE(preview_ui);

  // Test with invalid |preview_ui_addr|.
  bool cancel = false;
  const int32_t kInvalidId = -5;
  preview_ui->GetCurrentPrintPreviewStatus(kInvalidId, 0, &cancel);
  EXPECT_TRUE(cancel);

  const int kFirstRequestId = 1000;
  const int kSecondRequestId = 1001;
  const int32_t preview_ui_addr = preview_ui->GetIDForPrintPreviewUI();

  // Test with kFirstRequestId.
  preview_ui->OnPrintPreviewRequest(kFirstRequestId);
  cancel = true;
  preview_ui->GetCurrentPrintPreviewStatus(preview_ui_addr, kFirstRequestId,
                                           &cancel);
  EXPECT_FALSE(cancel);

  cancel = false;
  preview_ui->GetCurrentPrintPreviewStatus(preview_ui_addr, kSecondRequestId,
                                           &cancel);
  EXPECT_TRUE(cancel);

  // Test with kSecondRequestId.
  preview_ui->OnPrintPreviewRequest(kSecondRequestId);
  cancel = false;
  preview_ui->GetCurrentPrintPreviewStatus(preview_ui_addr, kFirstRequestId,
                                           &cancel);
  EXPECT_TRUE(cancel);

  cancel = true;
  preview_ui->GetCurrentPrintPreviewStatus(preview_ui_addr, kSecondRequestId,
                                           &cancel);
  EXPECT_FALSE(cancel);
}
