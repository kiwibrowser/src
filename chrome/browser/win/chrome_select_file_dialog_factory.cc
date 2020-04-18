// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/win/chrome_select_file_dialog_factory.h"

#include <utility>
#include <vector>

#include "base/bind.h"
#include "base/bind_helpers.h"
#include "base/feature_list.h"
#include "base/strings/string16.h"
#include "base/strings/string_util.h"
#include "base/win/win_util.h"
#include "chrome/services/util_win/public/mojom/constants.mojom.h"
#include "chrome/services/util_win/public/mojom/shell_util_win.mojom.h"
#include "content/public/common/service_manager_connection.h"
#include "services/service_manager/public/cpp/connector.h"
#include "ui/base/win/open_file_name_win.h"
#include "ui/shell_dialogs/select_file_dialog_win.h"
#include "ui/shell_dialogs/select_file_policy.h"

namespace {

constexpr base::Feature kIsolateShellOperations{
    "IsolateShellOperations", base::FEATURE_DISABLED_BY_DEFAULT};

chrome::mojom::ShellUtilWinPtr BindShellUtilWin() {
  chrome::mojom::ShellUtilWinPtr shell_util_win_ptr;
  content::ServiceManagerConnection::GetForProcess()
      ->GetConnector()
      ->BindInterface(chrome::mojom::kUtilWinServiceName, &shell_util_win_ptr);
  return shell_util_win_ptr;
}

}  // namespace

ChromeSelectFileDialogFactory::ChromeSelectFileDialogFactory() = default;

ChromeSelectFileDialogFactory::~ChromeSelectFileDialogFactory() = default;

ui::SelectFileDialog* ChromeSelectFileDialogFactory::Create(
    ui::SelectFileDialog::Listener* listener,
    std::unique_ptr<ui::SelectFilePolicy> policy) {
  return ui::CreateWinSelectFileDialog(
      listener, std::move(policy),
      base::Bind(&ChromeSelectFileDialogFactory::BlockingGetOpenFileName),
      base::Bind(&ChromeSelectFileDialogFactory::BlockingGetSaveFileName));
}

// static
bool ChromeSelectFileDialogFactory::BlockingGetOpenFileName(OPENFILENAME* ofn) {
  if (!base::FeatureList::IsEnabled(kIsolateShellOperations))
    return ::GetOpenFileName(ofn) == TRUE;

  mojo::SyncCallRestrictions::ScopedAllowSyncCall allow_sync_call;
  std::vector<base::FilePath> files;
  base::FilePath directory;

  // Sync operation, it's OK for the shell_util_win_ptr to go out of scope right
  // after the call.
  chrome::mojom::ShellUtilWinPtr shell_util_win_ptr = BindShellUtilWin();
  shell_util_win_ptr->CallGetOpenFileName(
      base::win::HandleToUint32(ofn->hwndOwner),
      static_cast<uint32_t>(ofn->Flags & ~OFN_ENABLEHOOK),
      ui::win::OpenFileName::GetFilters(ofn),
      ofn->lpstrInitialDir ? base::FilePath(ofn->lpstrInitialDir)
                           : base::FilePath(),
      base::FilePath(ofn->lpstrFile), &directory, &files);

  if (files.empty())
    return false;

  ui::win::OpenFileName::SetResult(directory, files, ofn);
  return true;
}

// static
bool ChromeSelectFileDialogFactory::BlockingGetSaveFileName(OPENFILENAME* ofn) {
  if (!base::FeatureList::IsEnabled(kIsolateShellOperations))
    return ::GetSaveFileName(ofn) == TRUE;


  mojo::SyncCallRestrictions::ScopedAllowSyncCall allow_sync_call;
  uint32_t filter_index = 0;
  base::FilePath path;

  // Sync operation, it's OK for the shell_util_win_ptr to go out of scope right
  // after the call.
  chrome::mojom::ShellUtilWinPtr shell_util_win_ptr = BindShellUtilWin();
  shell_util_win_ptr->CallGetSaveFileName(
      base::win::HandleToUint32(ofn->hwndOwner),
      static_cast<uint32_t>(ofn->Flags & ~OFN_ENABLEHOOK),
      ui::win::OpenFileName::GetFilters(ofn), ofn->nFilterIndex,
      ofn->lpstrInitialDir ? base::FilePath(ofn->lpstrInitialDir)
                           : base::FilePath(),
      base::FilePath(ofn->lpstrFile),
      ofn->lpstrDefExt ? base::string16(ofn->lpstrDefExt) : base::string16(),
      &path, &filter_index);

  if (path.empty())
    return false;

  base::wcslcpy(ofn->lpstrFile, path.value().c_str(), ofn->nMaxFile);
  ofn->nFilterIndex = static_cast<DWORD>(filter_index);
  return true;
}
