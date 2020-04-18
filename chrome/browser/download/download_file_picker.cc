// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/download/download_file_picker.h"

#include "base/metrics/histogram_macros.h"
#include "chrome/browser/download/download_prefs.h"
#include "chrome/browser/platform_util.h"
#include "chrome/browser/ui/chrome_select_file_policy.h"
#include "components/download/public/common/download_item.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/download_item_utils.h"
#include "content/public/browser/download_manager.h"
#include "content/public/browser/web_contents.h"

using download::DownloadItem;
using content::DownloadManager;
using content::WebContents;

namespace {

enum FilePickerResult {
  FILE_PICKER_SAME,
  FILE_PICKER_DIFFERENT_DIR,
  FILE_PICKER_DIFFERENT_NAME,
  FILE_PICKER_CANCEL,
  FILE_PICKER_MAX,
};

// Record how the File Picker was used during a download. This UMA is only
// recorded for profiles that do not always prompt for save locations on
// downloads.
void RecordFilePickerResult(const base::FilePath& suggested_path,
                            const base::FilePath& actual_path) {
  FilePickerResult result;
  if (suggested_path == actual_path)
    result = FILE_PICKER_SAME;
  else if (actual_path.empty())
    result = FILE_PICKER_CANCEL;
  else if (suggested_path.DirName() != actual_path.DirName())
    result = FILE_PICKER_DIFFERENT_DIR;
  else
    result = FILE_PICKER_DIFFERENT_NAME;

  UMA_HISTOGRAM_ENUMERATION("Download.FilePickerResult",
                            result,
                            FILE_PICKER_MAX);
}

}  // namespace

DownloadFilePicker::DownloadFilePicker(DownloadItem* item,
                                       const base::FilePath& suggested_path,
                                       const ConfirmationCallback& callback)
    : suggested_path_(suggested_path),
      file_selected_callback_(callback),
      should_record_file_picker_result_(false) {
  const DownloadPrefs* prefs = DownloadPrefs::FromBrowserContext(
      content::DownloadItemUtils::GetBrowserContext(item));
  DCHECK(prefs);
  // Only record UMA if we aren't prompting the user for all downloads.
  should_record_file_picker_result_ = !prefs->PromptForDownload();

  WebContents* web_contents = content::DownloadItemUtils::GetWebContents(item);
  if (!web_contents || !web_contents->GetNativeView())
    return;

  select_file_dialog_ = ui::SelectFileDialog::Create(
      this, std::make_unique<ChromeSelectFilePolicy>(web_contents));
  // |select_file_dialog_| could be null in Linux. See CreateSelectFileDialog()
  // in shell_dialog_linux.cc.
  if (!select_file_dialog_.get())
    return;

  ui::SelectFileDialog::FileTypeInfo file_type_info;
  // Platform file pickers, notably on Mac and Windows, tend to break
  // with double extensions like .tar.gz, so only pass in normal ones.
  base::FilePath::StringType extension = suggested_path_.FinalExtension();
  if (!extension.empty()) {
    extension.erase(extension.begin());  // drop the .
    file_type_info.extensions.resize(1);
    file_type_info.extensions[0].push_back(extension);
  }
  file_type_info.include_all_files = true;
  file_type_info.allowed_paths =
      ui::SelectFileDialog::FileTypeInfo::NATIVE_OR_DRIVE_PATH;
  gfx::NativeWindow owning_window = web_contents ?
      platform_util::GetTopLevel(web_contents->GetNativeView()) : NULL;

  select_file_dialog_->SelectFile(ui::SelectFileDialog::SELECT_SAVEAS_FILE,
                                  base::string16(),
                                  suggested_path_,
                                  &file_type_info,
                                  0,
                                  base::FilePath::StringType(),
                                  owning_window,
                                  NULL);
}

DownloadFilePicker::~DownloadFilePicker() {
  if (select_file_dialog_)
    select_file_dialog_->ListenerDestroyed();
}

void DownloadFilePicker::OnFileSelected(const base::FilePath& path) {
  if (should_record_file_picker_result_)
    RecordFilePickerResult(suggested_path_, path);
  file_selected_callback_.Run(path.empty()
                                  ? DownloadConfirmationResult::CANCELED
                                  : DownloadConfirmationResult::CONFIRMED,
                              path);
  delete this;
}

void DownloadFilePicker::FileSelected(const base::FilePath& path,
                                      int index,
                                      void* params) {
  OnFileSelected(path);
  // Deletes |this|
}

void DownloadFilePicker::FileSelectionCanceled(void* params) {
  OnFileSelected(base::FilePath());
  // Deletes |this|
}

// static
void DownloadFilePicker::ShowFilePicker(DownloadItem* item,
                                        const base::FilePath& suggested_path,
                                        const ConfirmationCallback& callback) {
  new DownloadFilePicker(item, suggested_path, callback);
  // DownloadFilePicker deletes itself.
}
