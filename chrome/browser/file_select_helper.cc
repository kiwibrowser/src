// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/file_select_helper.h"

#include <stddef.h>

#include <string>
#include <utility>

#include "base/bind.h"
#include "base/files/file_enumerator.h"
#include "base/files/file_util.h"
#include "base/strings/string_split.h"
#include "base/strings/string_util.h"
#include "base/strings/utf_string_conversions.h"
#include "base/task_scheduler/post_task.h"
#include "build/build_config.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/platform_util.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/profiles/profile_manager.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_dialogs.h"
#include "chrome/browser/ui/browser_list.h"
#include "chrome/browser/ui/chrome_select_file_policy.h"
#include "chrome/grit/generated_resources.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/render_process_host.h"
#include "content/public/browser/render_view_host.h"
#include "content/public/browser/render_widget_host.h"
#include "content/public/browser/render_widget_host_view.h"
#include "content/public/browser/storage_partition.h"
#include "content/public/browser/web_contents.h"
#include "content/public/common/file_chooser_file_info.h"
#include "content/public/common/file_chooser_params.h"
#include "net/base/filename_util.h"
#include "net/base/mime_util.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/shell_dialogs/selected_file_info.h"

#if defined(OS_CHROMEOS)
#include "chrome/browser/chromeos/file_manager/fileapi_util.h"
#include "content/public/browser/site_instance.h"
#endif

#if defined(FULL_SAFE_BROWSING)
#include "chrome/browser/safe_browsing/download_protection/download_protection_service.h"
#include "chrome/browser/safe_browsing/download_protection/download_protection_util.h"
#include "chrome/browser/safe_browsing/safe_browsing_service.h"
#endif

using content::BrowserThread;
using content::FileChooserParams;
using content::RenderViewHost;
using content::RenderWidgetHost;
using content::WebContents;

namespace {

// There is only one file-selection happening at any given time,
// so we allocate an enumeration ID for that purpose.  All IDs from
// the renderer must start at 0 and increase.
const int kFileSelectEnumerationId = -1;

// Converts a list of FilePaths to a list of ui::SelectedFileInfo.
std::vector<ui::SelectedFileInfo> FilePathListToSelectedFileInfoList(
    const std::vector<base::FilePath>& paths) {
  std::vector<ui::SelectedFileInfo> selected_files;
  for (size_t i = 0; i < paths.size(); ++i) {
    selected_files.push_back(
        ui::SelectedFileInfo(paths[i], paths[i]));
  }
  return selected_files;
}

void DeleteFiles(std::vector<base::FilePath> paths) {
  for (auto& file_path : paths)
    base::DeleteFile(file_path, false);
}

bool IsValidProfile(Profile* profile) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  // No profile manager in unit tests.
  if (!g_browser_process->profile_manager())
    return true;
  return g_browser_process->profile_manager()->IsValidProfile(profile);
}

#if defined(FULL_SAFE_BROWSING)

bool IsDownloadAllowedBySafeBrowsing(
    safe_browsing::DownloadCheckResult result) {
  using Result = safe_browsing::DownloadCheckResult;
  switch (result) {
    // Only allow downloads that are marked as SAFE or UNKNOWN by SafeBrowsing.
    // All other types are going to be blocked. UNKNOWN could be the result of a
    // failed safe browsing ping.
    case Result::UNKNOWN:
    case Result::SAFE:
    case Result::WHITELISTED_BY_POLICY:
      return true;

    case Result::DANGEROUS:
    case Result::UNCOMMON:
    case Result::DANGEROUS_HOST:
    case Result::POTENTIALLY_UNWANTED:
      return false;
  }
  NOTREACHED();
  return false;
}

void InterpretSafeBrowsingVerdict(const base::Callback<void(bool)>& recipient,
                                  safe_browsing::DownloadCheckResult result) {
  recipient.Run(IsDownloadAllowedBySafeBrowsing(result));
}

#endif

}  // namespace

struct FileSelectHelper::ActiveDirectoryEnumeration {
  explicit ActiveDirectoryEnumeration(const base::FilePath& path)
      : rvh_(NULL), path_(path) {}

  std::unique_ptr<DirectoryListerDispatchDelegate> delegate_;
  std::unique_ptr<net::DirectoryLister> lister_;
  RenderViewHost* rvh_;
  const base::FilePath path_;
  std::vector<base::FilePath> results_;
};

FileSelectHelper::FileSelectHelper(Profile* profile)
    : profile_(profile),
      render_frame_host_(nullptr),
      web_contents_(nullptr),
      select_file_dialog_(),
      select_file_types_(),
      dialog_type_(ui::SelectFileDialog::SELECT_OPEN_FILE),
      dialog_mode_(FileChooserParams::Open),
      observer_(this) {}

FileSelectHelper::~FileSelectHelper() {
  // There may be pending file dialogs, we need to tell them that we've gone
  // away so they don't try and call back to us.
  if (select_file_dialog_.get())
    select_file_dialog_->ListenerDestroyed();

  // Stop any pending directory enumeration, prevent a callback, and free
  // allocated memory.
  std::map<int, ActiveDirectoryEnumeration*>::iterator iter;
  for (iter = directory_enumerations_.begin();
       iter != directory_enumerations_.end();
       ++iter) {
    iter->second->lister_.reset();
    delete iter->second;
  }
}

void FileSelectHelper::DirectoryListerDispatchDelegate::OnListFile(
    const net::DirectoryLister::DirectoryListerData& data) {
  parent_->OnListFile(id_, data);
}

void FileSelectHelper::DirectoryListerDispatchDelegate::OnListDone(int error) {
  parent_->OnListDone(id_, error);
}

void FileSelectHelper::FileSelected(const base::FilePath& path,
                                    int index, void* params) {
  FileSelectedWithExtraInfo(ui::SelectedFileInfo(path, path), index, params);
}

void FileSelectHelper::FileSelectedWithExtraInfo(
    const ui::SelectedFileInfo& file,
    int index,
    void* params) {
  if (IsValidProfile(profile_)) {
    base::FilePath path = file.file_path;
    if (dialog_mode_ != FileChooserParams::UploadFolder)
      path = path.DirName();
    profile_->set_last_selected_directory(path);
  }

  if (!render_frame_host_) {
    RunFileChooserEnd();
    return;
  }

  const base::FilePath& path = file.local_path;
  if (dialog_type_ == ui::SelectFileDialog::SELECT_UPLOAD_FOLDER) {
    StartNewEnumeration(path, kFileSelectEnumerationId,
                        render_frame_host_->GetRenderViewHost());
    return;
  }

  std::vector<ui::SelectedFileInfo> files;
  files.push_back(file);

#if defined(OS_MACOSX)
  base::PostTaskWithTraits(
      FROM_HERE,
      {base::MayBlock(), base::TaskShutdownBehavior::CONTINUE_ON_SHUTDOWN},
      base::Bind(&FileSelectHelper::ProcessSelectedFilesMac, this, files));
#else
  NotifyRenderFrameHostAndEnd(files);
#endif  // defined(OS_MACOSX)
}

void FileSelectHelper::MultiFilesSelected(
    const std::vector<base::FilePath>& files,
    void* params) {
  std::vector<ui::SelectedFileInfo> selected_files =
      FilePathListToSelectedFileInfoList(files);

  MultiFilesSelectedWithExtraInfo(selected_files, params);
}

void FileSelectHelper::MultiFilesSelectedWithExtraInfo(
    const std::vector<ui::SelectedFileInfo>& files,
    void* params) {
  if (!files.empty() && IsValidProfile(profile_)) {
    base::FilePath path = files[0].file_path;
    if (dialog_mode_ != FileChooserParams::UploadFolder)
      path = path.DirName();
    profile_->set_last_selected_directory(path);
  }
#if defined(OS_MACOSX)
  base::PostTaskWithTraits(
      FROM_HERE,
      {base::MayBlock(), base::TaskShutdownBehavior::CONTINUE_ON_SHUTDOWN},
      base::Bind(&FileSelectHelper::ProcessSelectedFilesMac, this, files));
#else
  NotifyRenderFrameHostAndEnd(files);
#endif  // defined(OS_MACOSX)
}

void FileSelectHelper::FileSelectionCanceled(void* params) {
  NotifyRenderFrameHostAndEnd(std::vector<ui::SelectedFileInfo>());
}

void FileSelectHelper::StartNewEnumeration(const base::FilePath& path,
                                           int request_id,
                                           RenderViewHost* render_view_host) {
  auto entry = std::make_unique<ActiveDirectoryEnumeration>(path);
  entry->rvh_ = render_view_host;
  entry->delegate_.reset(new DirectoryListerDispatchDelegate(this, request_id));
  entry->lister_.reset(new net::DirectoryLister(
      path, net::DirectoryLister::NO_SORT_RECURSIVE, entry->delegate_.get()));
  entry->lister_->Start();
  directory_enumerations_[request_id] = entry.release();
}

void FileSelectHelper::OnListFile(
    int id,
    const net::DirectoryLister::DirectoryListerData& data) {
  ActiveDirectoryEnumeration* entry = directory_enumerations_[id];

  // Directory upload only cares about files.
  if (data.info.IsDirectory())
    return;

  entry->results_.push_back(data.path);
}

void FileSelectHelper::LaunchConfirmationDialog(
    const base::FilePath& path,
    std::vector<ui::SelectedFileInfo> selected_files) {
  ShowFolderUploadConfirmationDialog(
      path,
      base::BindOnce(&FileSelectHelper::NotifyRenderFrameHostAndEnd, this),
      std::move(selected_files), web_contents_);
}

void FileSelectHelper::OnListDone(int id, int error) {
  // This entry needs to be cleaned up when this function is done.
  std::unique_ptr<ActiveDirectoryEnumeration> entry(
      directory_enumerations_[id]);
  directory_enumerations_.erase(id);
  if (!entry->rvh_)
    return;
  if (error) {
    FileSelectionCanceled(NULL);
    return;
  }

  std::vector<ui::SelectedFileInfo> selected_files =
      FilePathListToSelectedFileInfoList(entry->results_);

  if (id == kFileSelectEnumerationId) {
    LaunchConfirmationDialog(entry->path_, std::move(selected_files));
  } else {
    entry->rvh_->DirectoryEnumerationFinished(id, entry->results_);
    EnumerateDirectoryEnd();
  }
}

void FileSelectHelper::NotifyRenderFrameHostAndEnd(
    const std::vector<ui::SelectedFileInfo>& files) {
  if (!render_frame_host_) {
    RunFileChooserEnd();
    return;
  }

#if defined(OS_CHROMEOS)
  if (!files.empty()) {
    if (!IsValidProfile(profile_)) {
      RunFileChooserEnd();
      return;
    }
    // Converts |files| into FileChooserFileInfo with handling of non-native
    // files.
    content::SiteInstance* site_instance =
        render_frame_host_->GetSiteInstance();
    storage::FileSystemContext* file_system_context =
        content::BrowserContext::GetStoragePartition(profile_, site_instance)
            ->GetFileSystemContext();
    file_manager::util::ConvertSelectedFileInfoListToFileChooserFileInfoList(
        file_system_context, site_instance->GetSiteURL(), files,
        base::Bind(
            &FileSelectHelper::NotifyRenderFrameHostAndEndAfterConversion,
            this));
    return;
  }
#endif  // defined(OS_CHROMEOS)

  std::vector<content::FileChooserFileInfo> chooser_files;
  for (const auto& file : files) {
    content::FileChooserFileInfo chooser_file;
    chooser_file.file_path = file.local_path;
    chooser_file.display_name = file.display_name;
    chooser_files.push_back(chooser_file);
  }

  NotifyRenderFrameHostAndEndAfterConversion(chooser_files);
}

void FileSelectHelper::NotifyRenderFrameHostAndEndAfterConversion(
    const std::vector<content::FileChooserFileInfo>& list) {
  if (render_frame_host_)
    render_frame_host_->FilesSelectedInChooser(list, dialog_mode_);

  // No members should be accessed from here on.
  RunFileChooserEnd();
}

void FileSelectHelper::DeleteTemporaryFiles() {
  base::PostTaskWithTraits(
      FROM_HERE,
      {base::MayBlock(), base::TaskPriority::BACKGROUND,
       base::TaskShutdownBehavior::BLOCK_SHUTDOWN},
      base::BindOnce(&DeleteFiles, std::move(temporary_files_)));
}

void FileSelectHelper::CleanUp() {
  if (!temporary_files_.empty()) {
    DeleteTemporaryFiles();

    // Now that the temporary files have been scheduled for deletion, there
    // is no longer any reason to keep this instance around.
    Release();
  }
}

bool FileSelectHelper::AbortIfWebContentsDestroyed() {
  if (render_frame_host_ && web_contents_)
    return false;

  RunFileChooserEnd();
  return true;
}

std::unique_ptr<ui::SelectFileDialog::FileTypeInfo>
FileSelectHelper::GetFileTypesFromAcceptType(
    const std::vector<base::string16>& accept_types) {
  std::unique_ptr<ui::SelectFileDialog::FileTypeInfo> base_file_type(
      new ui::SelectFileDialog::FileTypeInfo());
  if (accept_types.empty())
    return base_file_type;

  // Create FileTypeInfo and pre-allocate for the first extension list.
  std::unique_ptr<ui::SelectFileDialog::FileTypeInfo> file_type(
      new ui::SelectFileDialog::FileTypeInfo(*base_file_type));
  file_type->include_all_files = true;
  file_type->extensions.resize(1);
  std::vector<base::FilePath::StringType>* extensions =
      &file_type->extensions.back();

  // Find the corresponding extensions.
  int valid_type_count = 0;
  int description_id = 0;
  for (size_t i = 0; i < accept_types.size(); ++i) {
    std::string ascii_type = base::UTF16ToASCII(accept_types[i]);
    if (!IsAcceptTypeValid(ascii_type))
      continue;

    size_t old_extension_size = extensions->size();
    if (ascii_type[0] == '.') {
      // If the type starts with a period it is assumed to be a file extension
      // so we just have to add it to the list.
      base::FilePath::StringType ext(ascii_type.begin(), ascii_type.end());
      extensions->push_back(ext.substr(1));
    } else {
      if (ascii_type == "image/*")
        description_id = IDS_IMAGE_FILES;
      else if (ascii_type == "audio/*")
        description_id = IDS_AUDIO_FILES;
      else if (ascii_type == "video/*")
        description_id = IDS_VIDEO_FILES;

      net::GetExtensionsForMimeType(ascii_type, extensions);
    }

    if (extensions->size() > old_extension_size)
      valid_type_count++;
  }

  // If no valid extension is added, bail out.
  if (valid_type_count == 0)
    return base_file_type;

  // Use a generic description "Custom Files" if either of the following is
  // true:
  // 1) There're multiple types specified, like "audio/*,video/*"
  // 2) There're multiple extensions for a MIME type without parameter, like
  //    "ehtml,shtml,htm,html" for "text/html". On Windows, the select file
  //    dialog uses the first extension in the list to form the description,
  //    like "EHTML Files". This is not what we want.
  if (valid_type_count > 1 ||
      (valid_type_count == 1 && description_id == 0 && extensions->size() > 1))
    description_id = IDS_CUSTOM_FILES;

  if (description_id) {
    file_type->extension_description_overrides.push_back(
        l10n_util::GetStringUTF16(description_id));
  }

  return file_type;
}

// static
void FileSelectHelper::RunFileChooser(
    content::RenderFrameHost* render_frame_host,
    const FileChooserParams& params) {
  Profile* profile = Profile::FromBrowserContext(
      render_frame_host->GetProcess()->GetBrowserContext());
  // FileSelectHelper will keep itself alive until it sends the result message.
  scoped_refptr<FileSelectHelper> file_select_helper(
      new FileSelectHelper(profile));
  file_select_helper->RunFileChooser(
      render_frame_host, std::make_unique<content::FileChooserParams>(params));
}

// static
void FileSelectHelper::EnumerateDirectory(content::WebContents* tab,
                                          int request_id,
                                          const base::FilePath& path) {
  Profile* profile = Profile::FromBrowserContext(tab->GetBrowserContext());
  // FileSelectHelper will keep itself alive until it sends the result message.
  scoped_refptr<FileSelectHelper> file_select_helper(
      new FileSelectHelper(profile));
  file_select_helper->EnumerateDirectory(
      request_id, tab->GetRenderViewHost(), path);
}

void FileSelectHelper::RunFileChooser(
    content::RenderFrameHost* render_frame_host,
    std::unique_ptr<FileChooserParams> params) {
  DCHECK(!render_frame_host_);
  DCHECK(!web_contents_);
  DCHECK(params->default_file_name.empty() ||
         params->mode == FileChooserParams::Save)
      << "The default_file_name parameter should only be specified for Save "
         "file choosers";
  DCHECK(params->default_file_name == params->default_file_name.BaseName())
      << "The default_file_name parameter should not contain path separators";

  render_frame_host_ = render_frame_host;
  web_contents_ = WebContents::FromRenderFrameHost(render_frame_host);
  observer_.RemoveAll();
  content::WebContentsObserver::Observe(web_contents_);
  observer_.Add(render_frame_host_->GetRenderViewHost()->GetWidget());

  base::PostTaskWithTraits(
      FROM_HERE, {base::MayBlock()},
      base::BindOnce(&FileSelectHelper::GetFileTypesInThreadPool, this,
                     std::move(params)));

  // Because this class returns notifications to the RenderViewHost, it is
  // difficult for callers to know how long to keep a reference to this
  // instance. We AddRef() here to keep the instance alive after we return
  // to the caller, until the last callback is received from the file dialog.
  // At that point, we must call RunFileChooserEnd().
  AddRef();
}

void FileSelectHelper::GetFileTypesInThreadPool(
    std::unique_ptr<FileChooserParams> params) {
  select_file_types_ = GetFileTypesFromAcceptType(params->accept_types);
  select_file_types_->allowed_paths =
      params->need_local_path ? ui::SelectFileDialog::FileTypeInfo::NATIVE_PATH
                              : ui::SelectFileDialog::FileTypeInfo::ANY_PATH;

  BrowserThread::PostTask(
      BrowserThread::UI, FROM_HERE,
      base::BindOnce(&FileSelectHelper::GetSanitizedFilenameOnUIThread, this,
                     std::move(params)));
}

void FileSelectHelper::GetSanitizedFilenameOnUIThread(
    std::unique_ptr<FileChooserParams> params) {
  if (AbortIfWebContentsDestroyed())
    return;

  base::FilePath default_file_path = profile_->last_selected_directory().Append(
      GetSanitizedFileName(params->default_file_name));
#if defined(FULL_SAFE_BROWSING)
  if (params->mode == FileChooserParams::Save) {
    CheckDownloadRequestWithSafeBrowsing(default_file_path, std::move(params));
    return;
  }
#endif
  RunFileChooserOnUIThread(default_file_path, std::move(params));
}

#if defined(FULL_SAFE_BROWSING)
void FileSelectHelper::CheckDownloadRequestWithSafeBrowsing(
    const base::FilePath& default_file_path,
    std::unique_ptr<FileChooserParams> params) {
  safe_browsing::SafeBrowsingService* sb_service =
      g_browser_process->safe_browsing_service();

  if (!sb_service || !sb_service->download_protection_service() ||
      !sb_service->download_protection_service()->enabled()) {
    RunFileChooserOnUIThread(default_file_path, std::move(params));
    return;
  }

  std::vector<base::FilePath::StringType> alternate_extensions;
  if (select_file_types_) {
    for (const auto& extensions_list : select_file_types_->extensions) {
      for (const auto& extension_in_list : extensions_list) {
        base::FilePath::StringType extension =
            default_file_path.ReplaceExtension(extension_in_list)
                .FinalExtension();
        alternate_extensions.push_back(extension);
      }
    }
  }

  GURL requestor_url = params->requestor;
  sb_service->download_protection_service()->CheckPPAPIDownloadRequest(
      requestor_url,
      render_frame_host_? render_frame_host_->GetLastCommittedURL() : GURL(),
      WebContents::FromRenderFrameHost(render_frame_host_),
      default_file_path, alternate_extensions, profile_,
      base::Bind(&InterpretSafeBrowsingVerdict,
                 base::Bind(&FileSelectHelper::ProceedWithSafeBrowsingVerdict,
                            this, default_file_path, base::Passed(&params))));
}

void FileSelectHelper::ProceedWithSafeBrowsingVerdict(
    const base::FilePath& default_file_path,
    std::unique_ptr<content::FileChooserParams> params,
    bool allowed_by_safe_browsing) {
  if (!allowed_by_safe_browsing) {
    NotifyRenderFrameHostAndEnd(std::vector<ui::SelectedFileInfo>());
    return;
  }
  RunFileChooserOnUIThread(default_file_path, std::move(params));
}
#endif

void FileSelectHelper::RunFileChooserOnUIThread(
    const base::FilePath& default_file_path,
    std::unique_ptr<FileChooserParams> params) {
  DCHECK(params);
  if (AbortIfWebContentsDestroyed())
    return;

  select_file_dialog_ = ui::SelectFileDialog::Create(
      this, std::make_unique<ChromeSelectFilePolicy>(web_contents_));
  if (!select_file_dialog_.get())
    return;

  dialog_mode_ = params->mode;
  switch (params->mode) {
    case FileChooserParams::Open:
      dialog_type_ = ui::SelectFileDialog::SELECT_OPEN_FILE;
      break;
    case FileChooserParams::OpenMultiple:
      dialog_type_ = ui::SelectFileDialog::SELECT_OPEN_MULTI_FILE;
      break;
    case FileChooserParams::UploadFolder:
      dialog_type_ = ui::SelectFileDialog::SELECT_UPLOAD_FOLDER;
      break;
    case FileChooserParams::Save:
      dialog_type_ = ui::SelectFileDialog::SELECT_SAVEAS_FILE;
      break;
    default:
      // Prevent warning.
      dialog_type_ = ui::SelectFileDialog::SELECT_OPEN_FILE;
      NOTREACHED();
  }

  gfx::NativeWindow owning_window =
      platform_util::GetTopLevel(web_contents_->GetNativeView());

#if defined(OS_ANDROID)
  // Android needs the original MIME types and an additional capture value.
  std::pair<std::vector<base::string16>, bool> accept_types =
      std::make_pair(params->accept_types, params->capture);
#endif

  select_file_dialog_->SelectFile(
      dialog_type_, params->title, default_file_path, select_file_types_.get(),
      select_file_types_.get() && !select_file_types_->extensions.empty()
          ? 1
          : 0,  // 1-based index of default extension to show.
      base::FilePath::StringType(),
      owning_window,
#if defined(OS_ANDROID)
      &accept_types);
#else
      NULL);
#endif

  select_file_types_.reset();
}

// This method is called when we receive the last callback from the file chooser
// dialog or if the renderer was destroyed. Perform any cleanup and release the
// reference we added in RunFileChooser().
void FileSelectHelper::RunFileChooserEnd() {
  // If there are temporary files, then this instance needs to stick around
  // until web_contents_ is destroyed, so that this instance can delete the
  // temporary files.
  if (!temporary_files_.empty())
    return;

  render_frame_host_ = nullptr;
  web_contents_ = nullptr;
  Release();
}

void FileSelectHelper::EnumerateDirectory(int request_id,
                                          RenderViewHost* render_view_host,
                                          const base::FilePath& path) {
  // Because this class returns notifications to the RenderViewHost, it is
  // difficult for callers to know how long to keep a reference to this
  // instance. We AddRef() here to keep the instance alive after we return
  // to the caller, until the last callback is received from the enumeration
  // code. At that point, we must call EnumerateDirectoryEnd().
  AddRef();
  StartNewEnumeration(path, request_id, render_view_host);
}

// This method is called when we receive the last callback from the enumeration
// code. Perform any cleanup and release the reference we added in
// EnumerateDirectory().
void FileSelectHelper::EnumerateDirectoryEnd() {
  Release();
}

void FileSelectHelper::RenderWidgetHostDestroyed(
    content::RenderWidgetHost* widget_host) {
  render_frame_host_ = nullptr;
  observer_.Remove(widget_host);
}

void FileSelectHelper::RenderFrameHostChanged(
    content::RenderFrameHost* old_host,
    content::RenderFrameHost* new_host) {
  if (old_host == render_frame_host_)
    render_frame_host_ = nullptr;
}

void FileSelectHelper::RenderFrameDeleted(
    content::RenderFrameHost* render_frame_host) {
  if (render_frame_host == render_frame_host_)
    render_frame_host_ = nullptr;
}

void FileSelectHelper::WebContentsDestroyed() {
  render_frame_host_ = nullptr;
  web_contents_ = nullptr;
  CleanUp();
}

// static
bool FileSelectHelper::IsAcceptTypeValid(const std::string& accept_type) {
  // TODO(raymes): This only does some basic checks, extend to test more cases.
  // A 1 character accept type will always be invalid (either a "." in the case
  // of an extension or a "/" in the case of a MIME type).
  std::string unused;
  if (accept_type.length() <= 1 ||
      base::ToLowerASCII(accept_type) != accept_type ||
      base::TrimWhitespaceASCII(accept_type, base::TRIM_ALL, &unused) !=
          base::TRIM_NONE) {
    return false;
  }
  return true;
}

// static
base::FilePath FileSelectHelper::GetSanitizedFileName(
    const base::FilePath& suggested_filename) {
  if (suggested_filename.empty())
    return base::FilePath();
  return net::GenerateFileName(
      GURL(), std::string(), std::string(), suggested_filename.AsUTF8Unsafe(),
      std::string(), l10n_util::GetStringUTF8(IDS_DEFAULT_DOWNLOAD_FILENAME));
}
