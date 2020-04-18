// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_WEBUI_PRINT_PREVIEW_PRINT_PREVIEW_UI_H_
#define CHROME_BROWSER_UI_WEBUI_PRINT_PREVIEW_PRINT_PREVIEW_UI_H_

#include <stdint.h>

#include <memory>

#include "base/callback_forward.h"
#include "base/gtest_prod_util.h"
#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "base/time/time.h"
#include "chrome/browser/ui/webui/constrained_web_dialog_ui.h"
#include "printing/buildflags/buildflags.h"

class PrintPreviewHandler;
struct PrintHostMsg_DidGetPreviewPageCount_Params;
struct PrintHostMsg_RequestPrintPreview_Params;
struct PrintHostMsg_SetOptionsFromDocument_Params;

namespace base {
class DictionaryValue;
class FilePath;
class RefCountedMemory;
}

namespace gfx {
class Rect;
}

namespace printing {
struct PageSizeMargins;
}

class PrintPreviewUI : public ConstrainedWebDialogUI {
 public:
  explicit PrintPreviewUI(content::WebUI* web_ui);

  ~PrintPreviewUI() override;

  // Gets the print preview |data|. |index| is zero-based, and can be
  // |printing::COMPLETE_PREVIEW_DOCUMENT_INDEX| to get the entire preview
  // document.
  virtual void GetPrintPreviewDataForIndex(
      int index,
      scoped_refptr<base::RefCountedMemory>* data) const;

  // Sets the print preview |data|. |index| is zero-based, and can be
  // |printing::COMPLETE_PREVIEW_DOCUMENT_INDEX| to set the entire preview
  // document.
  void SetPrintPreviewDataForIndex(int index,
                                   scoped_refptr<base::RefCountedMemory> data);

  // Clear the existing print preview data.
  void ClearAllPreviewData();

  // Setters
  void SetInitiatorTitle(const base::string16& initiator_title);

  const base::string16& initiator_title() const { return initiator_title_; }

  bool source_is_modifiable() const { return source_is_modifiable_; }

  bool source_has_selection() const { return source_has_selection_; }

  bool print_selection_only() const { return print_selection_only_; }

  // Set initial settings for PrintPreviewUI.
  static void SetInitialParams(
      content::WebContents* print_preview_dialog,
      const PrintHostMsg_RequestPrintPreview_Params& params);

  // Determines whether to cancel a print preview request based on
  // |preview_ui_id| and |request_id|.
  // Can be called from any thread.
  static void GetCurrentPrintPreviewStatus(int32_t preview_ui_id,
                                           int request_id,
                                           bool* cancel);

  // Returns an id to uniquely identify this PrintPreviewUI.
  int32_t GetIDForPrintPreviewUI() const;

  // Notifies the Web UI of a print preview request with |request_id|.
  virtual void OnPrintPreviewRequest(int request_id);

  // Notifies the Web UI about the page count of the request preview.
  void OnDidGetPreviewPageCount(
      const PrintHostMsg_DidGetPreviewPageCount_Params& params);

  // Notifies the Web UI of the default page layout according to the currently
  // selected printer and page size.
  void OnDidGetDefaultPageLayout(const printing::PageSizeMargins& page_layout,
                                 const gfx::Rect& printable_area,
                                 bool has_custom_page_size_style);

  // Notifies the Web UI that the 0-based page |page_number| has been rendered.
  // |preview_request_id| indicates wich request resulted in this response.
  void OnDidPreviewPage(int page_number, int preview_request_id);

  // Notifies the Web UI renderer that preview data is available.
  // |expected_pages_count| specifies the total number of pages.
  // |preview_request_id| indicates which request resulted in this response.
  void OnPreviewDataIsAvailable(int expected_pages_count,
                                int preview_request_id);

  // Notifies the Web UI that the print preview failed to render.
  void OnPrintPreviewFailed();

  // Notified the Web UI that this print preview dialog's RenderProcess has been
  // closed, which may occur for several reasons, e.g. tab closure or crash.
  void OnPrintPreviewDialogClosed();

  // Notifies the Web UI that the preview request was cancelled.
  void OnPrintPreviewCancelled();

  // Notifies the Web UI that initiator is closed, so we can disable all the
  // controls that need the initiator for generating the preview data.
  void OnInitiatorClosed();

  // Notifies the Web UI that the printer is unavailable or its settings are
  // invalid.
  void OnInvalidPrinterSettings();

  // Notifies the Web UI to cancel the pending preview request.
  virtual void OnCancelPendingPreviewRequest();

  // Hides the print preview dialog.
  virtual void OnHidePreviewDialog();

  // Closes the print preview dialog.
  virtual void OnClosePrintPreviewDialog();

  // Notifies the WebUI to set print preset options from source PDF.
  void OnSetOptionsFromDocument(
      const PrintHostMsg_SetOptionsFromDocument_Params& params);

  // Allows tests to wait until the print preview dialog is loaded.
  class TestingDelegate {
   public:
    virtual void DidGetPreviewPageCount(int page_count) = 0;
    virtual void DidRenderPreviewPage(content::WebContents* preview_dialog) = 0;
  };

  static void SetDelegateForTesting(TestingDelegate* delegate);

  // Allows for tests to set a file path to print a PDF to. This also initiates
  // the printing without having to click a button on the print preview dialog.
  void SetSelectedFileForTesting(const base::FilePath& path);

  // Passes |closure| to PrintPreviewHandler::SetPdfSavedClosureForTesting().
  void SetPdfSavedClosureForTesting(const base::Closure& closure);

  // Tell the handler to send the enable-manipulate-settings-for-test WebUI
  // event.
  void SendEnableManipulateSettingsForTest();

  // Tell the handler to send the manipulate-settings-for-test WebUI event
  // to set the print preview settings contained in |settings|.
  void SendManipulateSettingsForTest(const base::DictionaryValue& settings);

 protected:
  // Alternate constructor for tests
  PrintPreviewUI(content::WebUI* web_ui,
                 std::unique_ptr<PrintPreviewHandler> handler);

 private:
  FRIEND_TEST_ALL_PREFIXES(PrintPreviewDialogControllerUnitTest,
                           TitleAfterReload);
  friend class FakePrintPreviewUI;

  base::TimeTicks initial_preview_start_time_;

  // The unique ID for this class instance. Stored here to avoid calling
  // GetIDForPrintPreviewUI() everywhere.
  const int32_t id_;

  // Weak pointer to the WebUI handler.
  PrintPreviewHandler* handler_;

  // Indicates whether the source document can be modified.
  bool source_is_modifiable_;

  // Indicates whether the source document has selection.
  bool source_has_selection_;

  // Indicates whether only the selection should be printed.
  bool print_selection_only_;

  // Store the initiator title, used for populating the print preview dialog
  // title.
  base::string16 initiator_title_;

  // Keeps track of whether OnClosePrintPreviewDialog() has been called or not.
  bool dialog_closed_;

  DISALLOW_COPY_AND_ASSIGN(PrintPreviewUI);
};

#endif  // CHROME_BROWSER_UI_WEBUI_PRINT_PREVIEW_PRINT_PREVIEW_UI_H_
