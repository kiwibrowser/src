// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/webui/print_preview/pdf_printer_handler.h"

#include <windows.h>  // Must be in front of other Windows header files.

#include <commdlg.h>

#include "base/memory/ref_counted.h"
#include "base/memory/ref_counted_memory.h"
#include "base/run_loop.h"
#include "chrome/browser/platform_util.h"
#include "chrome/browser/ui/browser_commands.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/test/base/browser_with_test_window_test.h"
#include "ui/shell_dialogs/select_file_dialog_win.h"
#include "ui/shell_dialogs/select_file_policy.h"

using content::WebContents;

namespace {

class FakePdfPrinterHandler;
bool GetOpenFileNameImpl(OPENFILENAME* ofn);
bool GetSaveFileNameImpl(FakePdfPrinterHandler* handler, OPENFILENAME* ofn);

class FakePdfPrinterHandler : public PdfPrinterHandler {
 public:
  FakePdfPrinterHandler(Profile* profile,
                        content::WebContents* contents,
                        printing::StickySettings* sticky_settings)
      : PdfPrinterHandler(profile, contents, sticky_settings),
        init_called_(false),
        save_failed_(false) {}

  void FileSelected(const base::FilePath& path,
                    int index,
                    void* params) override {
    // Since we always cancel the dialog as soon as it is initialized, this
    // should never be called.
    NOTREACHED();
  }

  void FileSelectionCanceled(void* params) override {
    save_failed_ = true;
    run_loop_.Quit();
  }

  void StartPrintToPdf(const base::string16& job_title) {
    StartPrint("", "", job_title, "", gfx::Size(), nullptr, base::DoNothing());
    run_loop_.Run();
  }

  bool save_failed() const { return save_failed_; }

  bool init_called() const { return init_called_; }

  void set_init_called() { init_called_ = true; }

 private:
  // Simplified version of select file to avoid checking preferences and sticky
  // settings in the test
  void SelectFile(const base::FilePath& default_filename,
                  content::WebContents* /* initiator */,
                  bool prompt_user) override {
    ui::SelectFileDialog::FileTypeInfo file_type_info;
    file_type_info.extensions.resize(1);
    file_type_info.extensions[0].push_back(FILE_PATH_LITERAL("pdf"));
    select_file_dialog_ = ui::CreateWinSelectFileDialog(
        this, nullptr /*policy already checked*/,
        base::Bind(GetOpenFileNameImpl), base::Bind(GetSaveFileNameImpl, this));
    select_file_dialog_->SelectFile(
        ui::SelectFileDialog::SELECT_SAVEAS_FILE, base::string16(),
        default_filename, &file_type_info, 0, base::FilePath::StringType(),
        platform_util::GetTopLevel(preview_web_contents_->GetNativeView()),
        nullptr);
  }

  bool init_called_;
  bool save_failed_;
  base::RunLoop run_loop_;
};

// Hook function to cancel the dialog when it is successfully initialized.
UINT_PTR CALLBACK PdfPrinterHandlerTestHookFunction(HWND hdlg,
                                                    UINT message,
                                                    WPARAM wparam,
                                                    LPARAM lparam) {
  if (message != WM_INITDIALOG)
    return 0;
  OPENFILENAME* ofn = reinterpret_cast<OPENFILENAME*>(lparam);
  FakePdfPrinterHandler* handler =
      reinterpret_cast<FakePdfPrinterHandler*>(ofn->lCustData);
  handler->set_init_called();
  PostMessage(GetParent(hdlg), WM_COMMAND, MAKEWPARAM(IDCANCEL, 0), 0);
  return 1;
}

bool GetOpenFileNameImpl(OPENFILENAME* ofn) {
  return ::GetOpenFileName(ofn);
}

bool GetSaveFileNameImpl(FakePdfPrinterHandler* handler, OPENFILENAME* ofn) {
  // Modify ofn so that the hook function will be called.
  ofn->Flags |= OFN_ENABLEHOOK;
  ofn->lpfnHook = PdfPrinterHandlerTestHookFunction;
  ofn->lCustData = reinterpret_cast<LPARAM>(handler);
  return ::GetSaveFileName(ofn);
}

}  // namespace

class PdfPrinterHandlerWinTest : public BrowserWithTestWindowTest {
 public:
  PdfPrinterHandlerWinTest() {}
  ~PdfPrinterHandlerWinTest() override {}

  void SetUp() override {
    BrowserWithTestWindowTest::SetUp();

    // Create a new tab
    chrome::NewTab(browser());
    AddTab(browser(), GURL("chrome://print"));

    // Create the PDF printer
    pdf_printer_ = std::make_unique<FakePdfPrinterHandler>(
        profile(), browser()->tab_strip_model()->GetWebContentsAt(0), nullptr);
  }

 protected:
  std::unique_ptr<FakePdfPrinterHandler> pdf_printer_;

 private:
  DISALLOW_COPY_AND_ASSIGN(PdfPrinterHandlerWinTest);
};

TEST_F(PdfPrinterHandlerWinTest, TestSaveAsPdf) {
  pdf_printer_->StartPrintToPdf(L"111111111111111111111.html");
  EXPECT_TRUE(pdf_printer_->init_called());
  EXPECT_TRUE(pdf_printer_->save_failed());
}

TEST_F(PdfPrinterHandlerWinTest, TestSaveAsPdfLongFileName) {
  pdf_printer_->StartPrintToPdf(
      L"11111111111111111111111111111111111111111111111111111111111111111111111"
      L"11111111111111111111111111111111111111111111111111111111111111111111111"
      L"11111111111111111111111111111111111111111111111111111111111111111111111"
      L"11111111111111111111111111111111111111111111111111111111111111111111111"
      L"1111111111111111111111111111111111111111111111111.html");
  EXPECT_TRUE(pdf_printer_->init_called());
  EXPECT_TRUE(pdf_printer_->save_failed());
}
