// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/shell_dialogs/select_file_dialog_win.h"

#include <shlobj.h>
#include <stddef.h>
#include <wrl/client.h>

#include <algorithm>
#include <memory>
#include <set>
#include <tuple>
#include <utility>
#include <vector>

#include "base/bind.h"
#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/i18n/case_conversion.h"
#include "base/logging.h"
#include "base/macros.h"
#include "base/message_loop/message_loop.h"
#include "base/single_thread_task_runner.h"
#include "base/threading/thread.h"
#include "base/threading/thread_task_runner_handle.h"
#include "base/win/registry.h"
#include "base/win/shortcut.h"
#include "ui/aura/window.h"
#include "ui/aura/window_event_dispatcher.h"
#include "ui/aura/window_tree_host.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/base/win/open_file_name_win.h"
#include "ui/gfx/native_widget_types.h"
#include "ui/shell_dialogs/base_shell_dialog_win.h"
#include "ui/shell_dialogs/select_file_policy.h"
#include "ui/strings/grit/ui_strings.h"

namespace {

bool CallBuiltinGetOpenFileName(OPENFILENAME* ofn) {
  return ::GetOpenFileName(ofn) == TRUE;
}

bool CallBuiltinGetSaveFileName(OPENFILENAME* ofn) {
  return ::GetSaveFileName(ofn) == TRUE;
}

// Given |extension|, if it's not empty, then remove the leading dot.
std::wstring GetExtensionWithoutLeadingDot(const std::wstring& extension) {
  DCHECK(extension.empty() || extension[0] == L'.');
  return extension.empty() ? extension : extension.substr(1);
}

// Distinguish directories from regular files.
bool IsDirectory(const base::FilePath& path) {
  base::File::Info file_info;
  return base::GetFileInfo(path, &file_info) ?
      file_info.is_directory : path.EndsWithSeparator();
}

// Get the file type description from the registry. This will be "Text Document"
// for .txt files, "JPEG Image" for .jpg files, etc. If the registry doesn't
// have an entry for the file type, we return false, true if the description was
// found. 'file_ext' must be in form ".txt".
static bool GetRegistryDescriptionFromExtension(const std::wstring& file_ext,
                                                std::wstring* reg_description) {
  DCHECK(reg_description);
  base::win::RegKey reg_ext(HKEY_CLASSES_ROOT, file_ext.c_str(), KEY_READ);
  std::wstring reg_app;
  if (reg_ext.ReadValue(NULL, &reg_app) == ERROR_SUCCESS && !reg_app.empty()) {
    base::win::RegKey reg_link(HKEY_CLASSES_ROOT, reg_app.c_str(), KEY_READ);
    if (reg_link.ReadValue(NULL, reg_description) == ERROR_SUCCESS)
      return true;
  }
  return false;
}

// Set up a filter for a Save/Open dialog, which will consist of |file_ext| file
// extensions (internally separated by semicolons), |ext_desc| as the text
// descriptions of the |file_ext| types (optional), and (optionally) the default
// 'All Files' view. The purpose of the filter is to show only files of a
// particular type in a Windows Save/Open dialog box. The resulting filter is
// returned. The filters created here are:
//   1. only files that have 'file_ext' as their extension
//   2. all files (only added if 'include_all_files' is true)
// Example:
//   file_ext: { "*.txt", "*.htm;*.html" }
//   ext_desc: { "Text Document" }
//   returned: "Text Document\0*.txt\0HTML Document\0*.htm;*.html\0"
//             "All Files\0*.*\0\0" (in one big string)
// If a description is not provided for a file extension, it will be retrieved
// from the registry. If the file extension does not exist in the registry, it
// will be omitted from the filter, as it is likely a bogus extension.
std::wstring FormatFilterForExtensions(
    const std::vector<std::wstring>& file_ext,
    const std::vector<std::wstring>& ext_desc,
    bool include_all_files) {
  const std::wstring all_ext = L"*.*";
  const std::wstring all_desc =
      l10n_util::GetStringUTF16(IDS_APP_SAVEAS_ALL_FILES);

  DCHECK(file_ext.size() >= ext_desc.size());

  if (file_ext.empty())
    include_all_files = true;

  std::wstring result;

  for (size_t i = 0; i < file_ext.size(); ++i) {
    std::wstring ext = file_ext[i];
    std::wstring desc;
    if (i < ext_desc.size())
      desc = ext_desc[i];

    if (ext.empty()) {
      // Force something reasonable to appear in the dialog box if there is no
      // extension provided.
      include_all_files = true;
      continue;
    }

    if (desc.empty()) {
      DCHECK(ext.find(L'.') != std::wstring::npos);
      std::wstring first_extension = ext.substr(ext.find(L'.'));
      size_t first_separator_index = first_extension.find(L';');
      if (first_separator_index != std::wstring::npos)
        first_extension = first_extension.substr(0, first_separator_index);

      // Find the extension name without the preceeding '.' character.
      std::wstring ext_name = first_extension;
      size_t ext_index = ext_name.find_first_not_of(L'.');
      if (ext_index != std::wstring::npos)
        ext_name = ext_name.substr(ext_index);

      if (!GetRegistryDescriptionFromExtension(first_extension, &desc)) {
        // The extension doesn't exist in the registry. Create a description
        // based on the unknown extension type (i.e. if the extension is .qqq,
        // the we create a description "QQQ File (.qqq)").
        include_all_files = true;
        desc = l10n_util::GetStringFUTF16(IDS_APP_SAVEAS_EXTENSION_FORMAT,
                                          base::i18n::ToUpper(ext_name),
                                          ext_name);
      }
      if (desc.empty())
        desc = L"*." + ext_name;
    }

    result.append(desc.c_str(), desc.size() + 1);  // Append NULL too.
    result.append(ext.c_str(), ext.size() + 1);
  }

  if (include_all_files) {
    result.append(all_desc.c_str(), all_desc.size() + 1);
    result.append(all_ext.c_str(), all_ext.size() + 1);
  }

  result.append(1, '\0');  // Double NULL required.
  return result;
}

// Implementation of SelectFileDialog that shows a Windows common dialog for
// choosing a file or folder.
class SelectFileDialogImpl : public ui::SelectFileDialog,
                             public ui::BaseShellDialogImpl {
 public:
  SelectFileDialogImpl(
      Listener* listener,
      std::unique_ptr<ui::SelectFilePolicy> policy,
      const base::Callback<bool(OPENFILENAME*)>& get_open_file_name_impl,
      const base::Callback<bool(OPENFILENAME*)>& get_save_file_name_impl);

  // BaseShellDialog implementation:
  bool IsRunning(gfx::NativeWindow owning_window) const override;
  void ListenerDestroyed() override;

 protected:
  // SelectFileDialog implementation:
  void SelectFileImpl(
      Type type,
      const base::string16& title,
      const base::FilePath& default_path,
      const FileTypeInfo* file_types,
      int file_type_index,
      const base::FilePath::StringType& default_extension,
      gfx::NativeWindow owning_window,
      void* params) override;

 private:
  ~SelectFileDialogImpl() override;

  // A struct for holding all the state necessary for displaying a Save dialog.
  struct ExecuteSelectParams {
    ExecuteSelectParams(Type type,
                        const std::wstring& title,
                        const base::FilePath& default_path,
                        const FileTypeInfo* file_types,
                        int file_type_index,
                        const std::wstring& default_extension,
                        RunState run_state,
                        HWND owner,
                        void* params)
        : type(type),
          title(title),
          default_path(default_path),
          file_type_index(file_type_index),
          default_extension(default_extension),
          run_state(run_state),
          ui_task_runner(base::ThreadTaskRunnerHandle::Get()),
          owner(owner),
          params(params) {
      DCHECK(base::MessageLoopForUI::IsCurrent());
      if (file_types)
        this->file_types = *file_types;
    }
    SelectFileDialog::Type type;
    std::wstring title;
    base::FilePath default_path;
    FileTypeInfo file_types;
    int file_type_index;
    std::wstring default_extension;
    RunState run_state;
    scoped_refptr<base::SingleThreadTaskRunner> ui_task_runner;
    HWND owner;
    void* params;
  };

  struct SelectFolderDialogOptions {
    const wchar_t* default_path;
    bool is_upload;
  };

  // Shows the file selection dialog modal to |owner| and calls the result
  // back on the ui thread. Run on the dialog thread.
  void ExecuteSelectFile(const ExecuteSelectParams& params);

  // Prompt the user for location to save a file.
  // Callers should provide the filter string, and also a filter index.
  // The parameter |index| indicates the initial index of filter description
  // and filter pattern for the dialog box. If |index| is zero or greater than
  // the number of total filter types, the system uses the first filter in the
  // |filter| buffer. |index| is used to specify the initial selected extension,
  // and when done contains the extension the user chose. The parameter
  // |final_name| returns the file name which contains the drive designator,
  // path, file name, and extension of the user selected file name. |def_ext| is
  // the default extension to give to the file if the user did not enter an
  // extension. If |ignore_suggested_ext| is true, any file extension contained
  // in |suggested_name| will not be used to generate the file name. This is
  // useful in the case of saving web pages, where we know the extension type
  // already and where |suggested_name| may contain a '.' character as a valid
  // part of the name, thus confusing our extension detection code.
  bool SaveFileAsWithFilter(HWND owner,
                            const std::wstring& suggested_name,
                            const std::wstring& filter,
                            const std::wstring& def_ext,
                            bool ignore_suggested_ext,
                            unsigned* index,
                            std::wstring* final_name);

  // Notifies the listener that a folder was chosen. Run on the ui thread.
  void FileSelected(const base::FilePath& path, int index,
                    void* params, RunState run_state);

  // Notifies listener that multiple files were chosen. Run on the ui thread.
  void MultiFilesSelected(const std::vector<base::FilePath>& paths,
                         void* params,
                         RunState run_state);

  // Notifies the listener that no file was chosen (the action was canceled).
  // Run on the ui thread.
  void FileNotSelected(void* params, RunState run_state);

  // Runs a Folder selection dialog box, passes back the selected folder in
  // |path| and returns true if the user clicks OK. If the user cancels the
  // dialog box the value in |path| is not modified and returns false. Run
  // on the dialog thread.
  bool RunSelectFolderDialog(const ExecuteSelectParams& params,
                             base::FilePath* path);

  // Runs an Open file dialog box, with similar semantics for input paramaters
  // as RunSelectFolderDialog.
  bool RunOpenFileDialog(const std::wstring& title,
                         const std::wstring& filters,
                         HWND owner,
                         base::FilePath* path);

  // Runs an Open file dialog box that supports multi-select, with similar
  // semantics for input paramaters as RunOpenFileDialog.
  bool RunOpenMultiFileDialog(const std::wstring& title,
                              const std::wstring& filter,
                              const base::FilePath& initial_path,
                              HWND owner,
                              std::vector<base::FilePath>* paths);

  // The callback function for when the select folder dialog is opened.
  static int CALLBACK BrowseCallbackProc(HWND window, UINT message,
                                         LPARAM parameter,
                                         LPARAM data);

  bool HasMultipleFileTypeChoicesImpl() override;

  // Returns the filter to be used while displaying the open/save file dialog.
  // This is computed from the extensions for the file types being opened.
  // |file_types| can be NULL in which case the returned filter will be empty.
  base::string16 GetFilterForFileTypes(const FileTypeInfo* file_types);

  bool has_multiple_file_type_choices_;
  base::Callback<bool(OPENFILENAME*)> get_open_file_name_impl_;
  base::Callback<bool(OPENFILENAME*)> get_save_file_name_impl_;

  DISALLOW_COPY_AND_ASSIGN(SelectFileDialogImpl);
};

SelectFileDialogImpl::SelectFileDialogImpl(
    Listener* listener,
    std::unique_ptr<ui::SelectFilePolicy> policy,
    const base::Callback<bool(OPENFILENAME*)>& get_open_file_name_impl,
    const base::Callback<bool(OPENFILENAME*)>& get_save_file_name_impl)
    : SelectFileDialog(listener, std::move(policy)),
      BaseShellDialogImpl(),
      has_multiple_file_type_choices_(false),
      get_open_file_name_impl_(get_open_file_name_impl),
      get_save_file_name_impl_(get_save_file_name_impl) {}

SelectFileDialogImpl::~SelectFileDialogImpl() {
}

void SelectFileDialogImpl::SelectFileImpl(
    Type type,
    const base::string16& title,
    const base::FilePath& default_path,
    const FileTypeInfo* file_types,
    int file_type_index,
    const base::FilePath::StringType& default_extension,
    gfx::NativeWindow owning_window,
    void* params) {
  has_multiple_file_type_choices_ =
      file_types ? file_types->extensions.size() > 1 : true;
  HWND owner = owning_window && owning_window->GetRootWindow()
      ? owning_window->GetHost()->GetAcceleratedWidget() : NULL;

  ExecuteSelectParams execute_params(type, title,
                                     default_path, file_types, file_type_index,
                                     default_extension, BeginRun(owner),
                                     owner, params);
  execute_params.run_state.dialog_thread->task_runner()->PostTask(
      FROM_HERE, base::BindOnce(&SelectFileDialogImpl::ExecuteSelectFile, this,
                                execute_params));
}

bool SelectFileDialogImpl::HasMultipleFileTypeChoicesImpl() {
  return has_multiple_file_type_choices_;
}

bool SelectFileDialogImpl::IsRunning(gfx::NativeWindow owning_window) const {
  if (!owning_window->GetRootWindow())
    return false;
  HWND owner = owning_window->GetHost()->GetAcceleratedWidget();
  return listener_ && IsRunningDialogForOwner(owner);
}

void SelectFileDialogImpl::ListenerDestroyed() {
  // Our associated listener has gone away, so we shouldn't call back to it if
  // our worker thread returns after the listener is dead.
  listener_ = NULL;
}

void SelectFileDialogImpl::ExecuteSelectFile(
    const ExecuteSelectParams& params) {
  base::string16 filter = GetFilterForFileTypes(&params.file_types);

  base::FilePath path = params.default_path;
  bool success = false;
  unsigned filter_index = params.file_type_index;
  if (params.type == SELECT_FOLDER || params.type == SELECT_UPLOAD_FOLDER ||
      params.type == SELECT_EXISTING_FOLDER) {
    success = RunSelectFolderDialog(params, &path);
  } else if (params.type == SELECT_SAVEAS_FILE) {
    std::wstring path_as_wstring = path.value();
    success = SaveFileAsWithFilter(params.run_state.owner,
        params.default_path.value(), filter,
        params.default_extension, false, &filter_index, &path_as_wstring);
    if (success)
      path = base::FilePath(path_as_wstring);
    DisableOwner(params.run_state.owner);
  } else if (params.type == SELECT_OPEN_FILE) {
    success = RunOpenFileDialog(params.title, filter,
                                params.run_state.owner, &path);
  } else if (params.type == SELECT_OPEN_MULTI_FILE) {
    std::vector<base::FilePath> paths;
    if (RunOpenMultiFileDialog(params.title, filter, path,
                               params.run_state.owner, &paths)) {
      params.ui_task_runner->PostTask(
          FROM_HERE,
          base::BindOnce(&SelectFileDialogImpl::MultiFilesSelected, this, paths,
                         params.params, params.run_state));
      return;
    }
  }

  if (success) {
    params.ui_task_runner->PostTask(
        FROM_HERE,
        base::BindOnce(&SelectFileDialogImpl::FileSelected, this, path,
                       filter_index, params.params, params.run_state));
  } else {
    params.ui_task_runner->PostTask(
        FROM_HERE, base::BindOnce(&SelectFileDialogImpl::FileNotSelected, this,
                                  params.params, params.run_state));
  }
}

bool SelectFileDialogImpl::SaveFileAsWithFilter(
    HWND owner,
    const std::wstring& suggested_name,
    const std::wstring& filter,
    const std::wstring& def_ext,
    bool ignore_suggested_ext,
    unsigned* index,
    std::wstring* final_name) {
  DCHECK(final_name);
  // Having an empty filter makes for a bad user experience. We should always
  // specify a filter when saving.
  DCHECK(!filter.empty());

  ui::win::OpenFileName save_as(owner,
                                OFN_OVERWRITEPROMPT | OFN_EXPLORER |
                                    OFN_ENABLESIZING | OFN_NOCHANGEDIR |
                                    OFN_PATHMUSTEXIST);

  const base::FilePath suggested_path = base::FilePath(suggested_name);
  if (!suggested_name.empty()) {
    base::FilePath suggested_file_name;
    base::FilePath suggested_directory;
    if (IsDirectory(suggested_path)) {
      suggested_directory = suggested_path;
    } else {
      suggested_directory = suggested_path.DirName();
      suggested_file_name = suggested_path.BaseName();
      // If the suggested_name is a root directory, file_part will be '\', and
      // the call to GetSaveFileName below will fail.
      if (suggested_file_name.value() == L"\\")
        suggested_file_name.clear();
    }
    save_as.SetInitialSelection(suggested_directory, suggested_file_name);
  }

  save_as.GetOPENFILENAME()->lpstrFilter =
      filter.empty() ? NULL : filter.c_str();
  save_as.GetOPENFILENAME()->nFilterIndex = *index;
  save_as.GetOPENFILENAME()->lpstrDefExt = &def_ext[0];

  if (!get_save_file_name_impl_.Run(save_as.GetOPENFILENAME()))
    return false;

  // Return the user's choice.
  final_name->assign(save_as.GetOPENFILENAME()->lpstrFile);
  *index = save_as.GetOPENFILENAME()->nFilterIndex;

  // Figure out what filter got selected. The filter index is 1-based.
  std::wstring filter_selected;
  if (*index > 0) {
    std::vector<std::tuple<base::string16, base::string16>> filters =
        ui::win::OpenFileName::GetFilters(save_as.GetOPENFILENAME());
    if (*index > filters.size())
      NOTREACHED() << "Invalid filter index.";
    else
      filter_selected = std::get<1>(filters[*index - 1]);
  }

  // Get the extension that was suggested to the user (when the Save As dialog
  // was opened). For saving web pages, we skip this step since there may be
  // 'extension characters' in the title of the web page.
  std::wstring suggested_ext;
  if (!ignore_suggested_ext)
    suggested_ext = GetExtensionWithoutLeadingDot(suggested_path.Extension());

  // If we can't get the extension from the suggested_name, we use the default
  // extension passed in. This is to cover cases like when saving a web page,
  // where we get passed in a name without an extension and a default extension
  // along with it.
  if (suggested_ext.empty())
    suggested_ext = def_ext;

  *final_name =
      ui::AppendExtensionIfNeeded(*final_name, filter_selected, suggested_ext);
  return true;
}

void SelectFileDialogImpl::FileSelected(const base::FilePath& selected_folder,
                                        int index,
                                        void* params,
                                        RunState run_state) {
  if (listener_)
    listener_->FileSelected(selected_folder, index, params);
  EndRun(run_state);
}

void SelectFileDialogImpl::MultiFilesSelected(
    const std::vector<base::FilePath>& selected_files,
    void* params,
    RunState run_state) {
  if (listener_)
    listener_->MultiFilesSelected(selected_files, params);
  EndRun(run_state);
}

void SelectFileDialogImpl::FileNotSelected(void* params, RunState run_state) {
  if (listener_)
    listener_->FileSelectionCanceled(params);
  EndRun(run_state);
}

int CALLBACK SelectFileDialogImpl::BrowseCallbackProc(HWND window,
                                                      UINT message,
                                                      LPARAM parameter,
                                                      LPARAM data) {
  if (message == BFFM_INITIALIZED) {
    SelectFolderDialogOptions* options =
        reinterpret_cast<SelectFolderDialogOptions*>(data);
    if (options->is_upload) {
      SendMessage(window, BFFM_SETOKTEXT, 0,
                  reinterpret_cast<LPARAM>(
                      l10n_util::GetStringUTF16(
                          IDS_SELECT_UPLOAD_FOLDER_DIALOG_UPLOAD_BUTTON)
                          .c_str()));
    }
    if (options->default_path) {
      SendMessage(window, BFFM_SETSELECTION, TRUE,
                  reinterpret_cast<LPARAM>(options->default_path));
    }
  }
  return 0;
}

bool SelectFileDialogImpl::RunSelectFolderDialog(
    const ExecuteSelectParams& params,
    base::FilePath* path) {
  DCHECK(path);
  std::wstring title = params.title;
  if (title.empty() && params.type == SELECT_UPLOAD_FOLDER) {
    // If it's for uploading don't use default dialog title to
    // make sure we clearly tell it's for uploading.
    title = l10n_util::GetStringUTF16(IDS_SELECT_UPLOAD_FOLDER_DIALOG_TITLE);
  }

  wchar_t dir_buffer[MAX_PATH + 1];

  bool result = false;
  BROWSEINFO browse_info = {0};
  browse_info.hwndOwner = params.run_state.owner;
  browse_info.lpszTitle = title.c_str();
  browse_info.pszDisplayName = dir_buffer;
  browse_info.ulFlags = BIF_USENEWUI | BIF_RETURNONLYFSDIRS;

  // If uploading or a default path was provided, update the BROWSEINFO
  // and set the callback function for the dialog so the strings can be set in
  // the callback.
  SelectFolderDialogOptions dialog_options = {0};
  if (path->value().length())
    dialog_options.default_path = path->value().c_str();
  if (params.type == SELECT_UPLOAD_FOLDER) {
    dialog_options.is_upload = true;
  }
  if (params.type == SELECT_UPLOAD_FOLDER ||
      params.type == SELECT_EXISTING_FOLDER)
    browse_info.ulFlags |= BIF_NONEWFOLDERBUTTON;
  if (dialog_options.is_upload || dialog_options.default_path) {
    browse_info.lParam = reinterpret_cast<LPARAM>(&dialog_options);
    browse_info.lpfn = &BrowseCallbackProc;
  }

  LPITEMIDLIST list = SHBrowseForFolder(&browse_info);
  DisableOwner(params.run_state.owner);
  if (list) {
    STRRET out_dir_buffer;
    ZeroMemory(&out_dir_buffer, sizeof(out_dir_buffer));
    out_dir_buffer.uType = STRRET_WSTR;
    Microsoft::WRL::ComPtr<IShellFolder> shell_folder;
    if (SHGetDesktopFolder(shell_folder.GetAddressOf()) == NOERROR) {
      HRESULT hr = shell_folder->GetDisplayNameOf(list, SHGDN_FORPARSING,
                                                  &out_dir_buffer);
      if (SUCCEEDED(hr) && out_dir_buffer.uType == STRRET_WSTR) {
        *path = base::FilePath(out_dir_buffer.pOleStr);
        CoTaskMemFree(out_dir_buffer.pOleStr);
        result = true;
      } else {
        // Use old way if we don't get what we want.
        wchar_t old_out_dir_buffer[MAX_PATH + 1];
        if (SHGetPathFromIDList(list, old_out_dir_buffer)) {
          *path = base::FilePath(old_out_dir_buffer);
          result = true;
        }
      }

      // According to MSDN, win2000 will not resolve shortcuts, so we do it
      // ourself.
      base::win::ResolveShortcut(*path, path, NULL);
    }
    CoTaskMemFree(list);
  }
  return result;
}

bool SelectFileDialogImpl::RunOpenFileDialog(const std::wstring& title,
                                             const std::wstring& filter,
                                             HWND owner,
                                             base::FilePath* path) {
  // We use OFN_NOCHANGEDIR so that the user can rename or delete the
  // directory without having to close Chrome first.
  ui::win::OpenFileName ofn(owner, OFN_FILEMUSTEXIST | OFN_NOCHANGEDIR);
  if (!path->empty()) {
    if (IsDirectory(*path))
      ofn.SetInitialSelection(*path, base::FilePath());
    else
      ofn.SetInitialSelection(path->DirName(), path->BaseName());
  }

  if (!filter.empty())
    ofn.GetOPENFILENAME()->lpstrFilter = filter.c_str();

  bool success = get_open_file_name_impl_.Run(ofn.GetOPENFILENAME());
  DisableOwner(owner);
  if (success)
    *path = ofn.GetSingleResult();
  return success;
}

bool SelectFileDialogImpl::RunOpenMultiFileDialog(
    const std::wstring& title,
    const std::wstring& filter,
    const base::FilePath& initial_path,
    HWND owner,
    std::vector<base::FilePath>* paths) {
  // We use OFN_NOCHANGEDIR so that the user can rename or delete the directory
  // without having to close Chrome first.
  ui::win::OpenFileName ofn(owner,
                            OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST |
                                OFN_EXPLORER | OFN_HIDEREADONLY |
                                OFN_ALLOWMULTISELECT | OFN_NOCHANGEDIR);
  if (!initial_path.empty()) {
    if (IsDirectory(initial_path))
      ofn.SetInitialSelection(initial_path, base::FilePath());
    else
      ofn.SetInitialSelection(initial_path.DirName(), base::FilePath());
  }
  if (!filter.empty())
    ofn.GetOPENFILENAME()->lpstrFilter = filter.c_str();

  base::FilePath directory;
  std::vector<base::FilePath> filenames;

  if (get_open_file_name_impl_.Run(ofn.GetOPENFILENAME()))
    ofn.GetResult(&directory, &filenames);

  DisableOwner(owner);

  for (std::vector<base::FilePath>::iterator it = filenames.begin();
       it != filenames.end();
       ++it) {
    paths->push_back(directory.Append(*it));
  }

  return !paths->empty();
}

base::string16 SelectFileDialogImpl::GetFilterForFileTypes(
    const FileTypeInfo* file_types) {
  if (!file_types)
    return base::string16();

  std::vector<base::string16> exts;
  for (size_t i = 0; i < file_types->extensions.size(); ++i) {
    const std::vector<base::string16>& inner_exts = file_types->extensions[i];
    base::string16 ext_string;
    for (size_t j = 0; j < inner_exts.size(); ++j) {
      if (!ext_string.empty())
        ext_string.push_back(L';');
      ext_string.append(L"*.");
      ext_string.append(inner_exts[j]);
    }
    exts.push_back(ext_string);
  }
  return FormatFilterForExtensions(
      exts,
      file_types->extension_description_overrides,
      file_types->include_all_files);
}

}  // namespace

namespace ui {

// This function takes the output of a SaveAs dialog: a filename, a filter and
// the extension originally suggested to the user (shown in the dialog box) and
// returns back the filename with the appropriate extension tacked on. If the
// user requests an unknown extension and is not using the 'All files' filter,
// the suggested extension will be appended, otherwise we will leave the
// filename unmodified. |filename| should contain the filename selected in the
// SaveAs dialog box and may include the path, |filter_selected| should be
// '*.something', for example '*.*' or it can be blank (which is treated as
// *.*). |suggested_ext| should contain the extension without the dot (.) in
// front, for example 'jpg'.
std::wstring AppendExtensionIfNeeded(
    const std::wstring& filename,
    const std::wstring& filter_selected,
    const std::wstring& suggested_ext) {
  DCHECK(!filename.empty());
  std::wstring return_value = filename;

  // If we wanted a specific extension, but the user's filename deleted it or
  // changed it to something that the system doesn't understand, re-append.
  // Careful: Checking net::GetMimeTypeFromExtension() will only find
  // extensions with a known MIME type, which many "known" extensions on Windows
  // don't have.  So we check directly for the "known extension" registry key.
  std::wstring file_extension(
      GetExtensionWithoutLeadingDot(base::FilePath(filename).Extension()));
  std::wstring key(L"." + file_extension);
  if (!(filter_selected.empty() || filter_selected == L"*.*") &&
      !base::win::RegKey(HKEY_CLASSES_ROOT, key.c_str(), KEY_READ).Valid() &&
      file_extension != suggested_ext) {
    if (return_value.back() != L'.')
      return_value.append(L".");
    return_value.append(suggested_ext);
  }

  // Strip any trailing dots, which Windows doesn't allow.
  size_t index = return_value.find_last_not_of(L'.');
  if (index < return_value.size() - 1)
    return_value.resize(index + 1);

  return return_value;
}

SelectFileDialog* CreateWinSelectFileDialog(
    SelectFileDialog::Listener* listener,
    std::unique_ptr<SelectFilePolicy> policy,
    const base::Callback<bool(OPENFILENAME* ofn)>& get_open_file_name_impl,
    const base::Callback<bool(OPENFILENAME* ofn)>& get_save_file_name_impl) {
  return new SelectFileDialogImpl(listener, std::move(policy),
                                  get_open_file_name_impl,
                                  get_save_file_name_impl);
}

SelectFileDialog* CreateSelectFileDialog(
    SelectFileDialog::Listener* listener,
    std::unique_ptr<SelectFilePolicy> policy) {
  return CreateWinSelectFileDialog(listener, std::move(policy),
                                   base::Bind(&CallBuiltinGetOpenFileName),
                                   base::Bind(&CallBuiltinGetSaveFileName));
}

}  // namespace ui
