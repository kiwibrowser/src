// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chromeos/extensions/file_manager/private_api_dialog.h"

#include <stddef.h>

#include "chrome/browser/chromeos/extensions/file_manager/private_api_util.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/browser/ui/views/select_file_dialog_extension.h"
#include "chrome/common/extensions/api/file_manager_private.h"
#include "content/public/browser/browser_thread.h"
#include "extensions/browser/extension_function_dispatcher.h"
#include "ui/shell_dialogs/selected_file_info.h"

using content::BrowserThread;

namespace extensions {

namespace {

// TODO(https://crbug.com/844654): This should be using something more
// deterministic.
content::WebContents* GetAssociatedWebContentsDeprecated(
    ChromeAsyncExtensionFunction* function) {
  if (function->dispatcher()) {
    content::WebContents* web_contents =
        function->dispatcher()->GetAssociatedWebContents();
    if (web_contents)
      return web_contents;
  }

  Browser* browser =
      ChromeExtensionFunctionDetails(function).GetCurrentBrowser();
  if (!browser)
    return nullptr;
  return browser->tab_strip_model()->GetActiveWebContents();
}

// Computes the routing ID for SelectFileDialogExtension from the |function|.
SelectFileDialogExtension::RoutingID GetFileDialogRoutingID(
    ChromeAsyncExtensionFunction* function) {
  return SelectFileDialogExtension::GetRoutingIDFromWebContents(
      GetAssociatedWebContentsDeprecated(function));
}

}  // namespace

bool FileManagerPrivateCancelDialogFunction::RunAsync() {
  SelectFileDialogExtension::OnFileSelectionCanceled(
      GetFileDialogRoutingID(this));
  SendResponse(true);
  return true;
}

bool FileManagerPrivateSelectFileFunction::RunAsync() {
  using extensions::api::file_manager_private::SelectFile::Params;
  const std::unique_ptr<Params> params(Params::Create(*args_));
  EXTENSION_FUNCTION_VALIDATE(params);

  std::vector<GURL> file_paths;
  file_paths.push_back(GURL(params->selected_path));

  file_manager::util::GetSelectedFileInfoLocalPathOption option =
      file_manager::util::NO_LOCAL_PATH_RESOLUTION;
  if (params->should_return_local_path) {
    option = params->for_opening ?
        file_manager::util::NEED_LOCAL_PATH_FOR_OPENING :
        file_manager::util::NEED_LOCAL_PATH_FOR_SAVING;
  }

  file_manager::util::GetSelectedFileInfo(
      render_frame_host(),
      GetProfile(),
      file_paths,
      option,
      base::Bind(
          &FileManagerPrivateSelectFileFunction::GetSelectedFileInfoResponse,
          this,
          params->index));
  return true;
}

void FileManagerPrivateSelectFileFunction::GetSelectedFileInfoResponse(
    int index,
    const std::vector<ui::SelectedFileInfo>& files) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  if (files.size() != 1) {
    SendResponse(false);
    return;
  }
  SelectFileDialogExtension::OnFileSelected(GetFileDialogRoutingID(this),
                                            files[0], index);
  SendResponse(true);
}

bool FileManagerPrivateSelectFilesFunction::RunAsync() {
  using extensions::api::file_manager_private::SelectFiles::Params;
  const std::unique_ptr<Params> params(Params::Create(*args_));
  EXTENSION_FUNCTION_VALIDATE(params);

  std::vector<GURL> file_urls;
  for (size_t i = 0; i < params->selected_paths.size(); ++i)
    file_urls.push_back(GURL(params->selected_paths[i]));

  file_manager::util::GetSelectedFileInfo(
      render_frame_host(),
      GetProfile(),
      file_urls,
      params->should_return_local_path ?
          file_manager::util::NEED_LOCAL_PATH_FOR_OPENING :
          file_manager::util::NO_LOCAL_PATH_RESOLUTION,
      base::Bind(
          &FileManagerPrivateSelectFilesFunction::GetSelectedFileInfoResponse,
          this));
  return true;
}

void FileManagerPrivateSelectFilesFunction::GetSelectedFileInfoResponse(
    const std::vector<ui::SelectedFileInfo>& files) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  if (files.empty()) {
    SendResponse(false);
    return;
  }

  SelectFileDialogExtension::OnMultiFilesSelected(GetFileDialogRoutingID(this),
                                                  files);
  SendResponse(true);
}

}  // namespace extensions
