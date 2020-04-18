// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_SHELL_DIALOGS_SELECT_FILE_DIALOG_WIN_H_
#define UI_SHELL_DIALOGS_SELECT_FILE_DIALOG_WIN_H_

#include <Windows.h>
#include <commdlg.h>

#include "base/callback_forward.h"
#include "ui/gfx/native_widget_types.h"
#include "ui/shell_dialogs/select_file_dialog.h"
#include "ui/shell_dialogs/shell_dialogs_export.h"

namespace ui {

class SelectFilePolicy;

// Implementation detail exported for unit tests.
SHELL_DIALOGS_EXPORT std::wstring AppendExtensionIfNeeded(
    const std::wstring& filename,
    const std::wstring& filter_selected,
    const std::wstring& suggested_ext);

SHELL_DIALOGS_EXPORT SelectFileDialog* CreateWinSelectFileDialog(
    SelectFileDialog::Listener* listener,
    std::unique_ptr<SelectFilePolicy> policy,
    const base::Callback<bool(OPENFILENAME* ofn)>& get_open_file_name_impl,
    const base::Callback<bool(OPENFILENAME* ofn)>& get_save_file_name_impl);

}  // namespace ui

#endif  //  UI_SHELL_DIALOGS_SELECT_FILE_DIALOG_WIN_H_

