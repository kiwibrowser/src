// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/download/save_package_file_picker.h"

#include <stddef.h>

#include <memory>

#include "base/bind.h"
#include "base/command_line.h"
#include "base/i18n/file_util_icu.h"
#include "base/metrics/histogram_macros.h"
#include "base/strings/utf_string_conversions.h"
#include "build/build_config.h"
#include "chrome/browser/download/chrome_download_manager_delegate.h"
#include "chrome/browser/download/download_prefs.h"
#include "chrome/browser/platform_util.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/chrome_select_file_policy.h"
#include "chrome/common/chrome_switches.h"
#include "chrome/grit/generated_resources.h"
#include "components/prefs/pref_member.h"
#include "components/prefs/pref_service.h"
#include "content/public/browser/download_manager.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/render_process_host.h"
#include "content/public/browser/save_page_type.h"
#include "content/public/browser/web_contents.h"
#include "ui/base/l10n/l10n_util.h"

#if defined(OS_CHROMEOS)
#include "chrome/browser/chromeos/drive/download_handler.h"
#include "chrome/browser/chromeos/drive/file_system_util.h"
#endif

using content::RenderProcessHost;
using content::SavePageType;
using content::WebContents;

namespace {

// If false, we don't prompt the user as to where to save the file.  This
// exists only for testing.
bool g_should_prompt_for_filename = true;

void OnSavePackageDownloadCreated(download::DownloadItem* download) {
  ChromeDownloadManagerDelegate::DisableSafeBrowsing(download);
}

#if defined(OS_CHROMEOS)
void OnSavePackageDownloadCreatedChromeOS(Profile* profile,
                                          const base::FilePath& drive_path,
                                          download::DownloadItem* download) {
  drive::DownloadHandler::GetForProfile(profile)->SetDownloadParams(
      drive_path, download);
  OnSavePackageDownloadCreated(download);
}

// Trampoline callback between SubstituteDriveDownloadPath() and |callback|.
void ContinueSettingUpDriveDownload(
    const content::SavePackagePathPickedCallback& callback,
    content::SavePageType save_type,
    Profile* profile,
    const base::FilePath& drive_path,
    const base::FilePath& drive_tmp_download_path) {
  if (drive_tmp_download_path.empty())  // Substitution failed.
    return;
  callback.Run(drive_tmp_download_path, save_type, base::Bind(
      &OnSavePackageDownloadCreatedChromeOS, profile, drive_path));
}
#endif

// Adds "Webpage, HTML Only" type to FileTypeInfo.
void AddHtmlOnlyFileTypeInfo(
    ui::SelectFileDialog::FileTypeInfo* file_type_info,
    const base::FilePath::StringType& extra_extension) {
  file_type_info->extension_description_overrides.push_back(
      l10n_util::GetStringUTF16(IDS_SAVE_PAGE_DESC_HTML_ONLY));

  std::vector<base::FilePath::StringType> extensions;
  extensions.push_back(FILE_PATH_LITERAL("html"));
  extensions.push_back(FILE_PATH_LITERAL("htm"));
  if (!extra_extension.empty())
    extensions.push_back(extra_extension);
  file_type_info->extensions.push_back(extensions);
}

// Adds "Web Archive, Single File" type to FileTypeInfo.
void AddSingleFileFileTypeInfo(
    ui::SelectFileDialog::FileTypeInfo* file_type_info) {
  file_type_info->extension_description_overrides.push_back(
      l10n_util::GetStringUTF16(IDS_SAVE_PAGE_DESC_SINGLE_FILE));

  std::vector<base::FilePath::StringType> extensions;
  extensions.push_back(FILE_PATH_LITERAL("mhtml"));
  file_type_info->extensions.push_back(extensions);
}

// Chrome OS doesn't support HTML-Complete. crbug.com/154823
#if !defined(OS_CHROMEOS)
// Adds "Webpage, Complete" type to FileTypeInfo.
void AddCompleteFileTypeInfo(
    ui::SelectFileDialog::FileTypeInfo* file_type_info,
    const base::FilePath::StringType& extra_extension) {
  file_type_info->extension_description_overrides.push_back(
      l10n_util::GetStringUTF16(IDS_SAVE_PAGE_DESC_COMPLETE));

  std::vector<base::FilePath::StringType> extensions;
  extensions.push_back(FILE_PATH_LITERAL("htm"));
  extensions.push_back(FILE_PATH_LITERAL("html"));
  if (!extra_extension.empty())
    extensions.push_back(extra_extension);
  file_type_info->extensions.push_back(extensions);
}
#endif

}  // anonymous namespace

bool SavePackageFilePicker::ShouldSaveAsMHTML() const {
#if !defined(OS_CHROMEOS)
  if (!base::CommandLine::ForCurrentProcess()->HasSwitch(
          switches::kSavePageAsMHTML))
    return false;
#endif
  return can_save_as_complete_;
}

SavePackageFilePicker::SavePackageFilePicker(
    content::WebContents* web_contents,
    const base::FilePath& suggested_path,
    const base::FilePath::StringType& default_extension,
    bool can_save_as_complete,
    DownloadPrefs* download_prefs,
    const content::SavePackagePathPickedCallback& callback)
    : render_process_id_(web_contents->GetMainFrame()->GetProcess()->GetID()),
      can_save_as_complete_(can_save_as_complete),
      download_prefs_(download_prefs),
      callback_(callback) {
  base::FilePath suggested_path_copy = suggested_path;
  base::FilePath::StringType default_extension_copy = default_extension;
  int file_type_index = 0;
  ui::SelectFileDialog::FileTypeInfo file_type_info;

  file_type_info.allowed_paths =
      ui::SelectFileDialog::FileTypeInfo::NATIVE_OR_DRIVE_PATH;

  if (can_save_as_complete_) {
    // The option index is not zero-based. Put a dummy entry.
    save_types_.push_back(content::SAVE_PAGE_TYPE_UNKNOWN);

    base::FilePath::StringType extra_extension;
    if (ShouldSaveAsMHTML()) {
      default_extension_copy = FILE_PATH_LITERAL("mhtml");
      suggested_path_copy = suggested_path_copy.ReplaceExtension(
          default_extension_copy);
    } else {
      if (!suggested_path_copy.FinalExtension().empty() &&
          !suggested_path_copy.MatchesExtension(FILE_PATH_LITERAL(".htm")) &&
          !suggested_path_copy.MatchesExtension(FILE_PATH_LITERAL(".html"))) {
        extra_extension = suggested_path_copy.FinalExtension().substr(1);
      }
    }

    AddHtmlOnlyFileTypeInfo(&file_type_info, extra_extension);
    save_types_.push_back(content::SAVE_PAGE_TYPE_AS_ONLY_HTML);

    if (ShouldSaveAsMHTML()) {
      AddSingleFileFileTypeInfo(&file_type_info);
      save_types_.push_back(content::SAVE_PAGE_TYPE_AS_MHTML);
    }

#if !defined(OS_CHROMEOS)
    AddCompleteFileTypeInfo(&file_type_info, extra_extension);
    save_types_.push_back(content::SAVE_PAGE_TYPE_AS_COMPLETE_HTML);
#endif

    file_type_info.include_all_files = false;

    content::SavePageType preferred_save_type =
        static_cast<content::SavePageType>(download_prefs_->save_file_type());
    if (ShouldSaveAsMHTML())
      preferred_save_type = content::SAVE_PAGE_TYPE_AS_MHTML;

    // Select the item saved in the pref.
    for (size_t i = 0; i < save_types_.size(); ++i) {
      if (save_types_[i] == preferred_save_type) {
        file_type_index = i;
        break;
      }
    }

    // If the item saved in the pref was not found, use the last item.
    if (!file_type_index)
      file_type_index = save_types_.size() - 1;
  } else {
    // The contents can not be saved as complete-HTML, so do not show the file
    // filters.
    file_type_info.extensions.resize(1);
    file_type_info.extensions[0].push_back(
        suggested_path_copy.FinalExtension());

    if (!file_type_info.extensions[0][0].empty()) {
      // Drop the .
      file_type_info.extensions[0][0].erase(0, 1);
    }

    file_type_info.include_all_files = true;
    file_type_index = 1;
  }

  if (g_should_prompt_for_filename) {
    select_file_dialog_ = ui::SelectFileDialog::Create(
        this, std::make_unique<ChromeSelectFilePolicy>(web_contents));
    select_file_dialog_->SelectFile(
        ui::SelectFileDialog::SELECT_SAVEAS_FILE,
        base::string16(),
        suggested_path_copy,
        &file_type_info,
        file_type_index,
        default_extension_copy,
        platform_util::GetTopLevel(web_contents->GetNativeView()),
        NULL);
  } else {
    // Just use 'suggested_path_copy' instead of opening the dialog prompt.
    // Go through FileSelected() for consistency.
    FileSelected(suggested_path_copy, file_type_index, NULL);
  }
}

SavePackageFilePicker::~SavePackageFilePicker() {
}

void SavePackageFilePicker::SetShouldPromptUser(bool should_prompt) {
  g_should_prompt_for_filename = should_prompt;
}

void SavePackageFilePicker::FileSelected(
    const base::FilePath& path, int index, void* unused_params) {
  std::unique_ptr<SavePackageFilePicker> delete_this(this);
  RenderProcessHost* process = RenderProcessHost::FromID(render_process_id_);
  if (!process)
    return;
  SavePageType save_type = content::SAVE_PAGE_TYPE_UNKNOWN;

  if (can_save_as_complete_) {
    DCHECK_LT(index, static_cast<int>(save_types_.size()));
    save_type = save_types_[index];
    if (select_file_dialog_.get() &&
        select_file_dialog_->HasMultipleFileTypeChoices())
      download_prefs_->SetSaveFileType(save_type);

    UMA_HISTOGRAM_ENUMERATION("Download.SavePageType",
                              save_type,
                              content::SAVE_PAGE_TYPE_MAX);
  } else {
    // Use "HTML Only" type as a dummy.
    save_type = content::SAVE_PAGE_TYPE_AS_ONLY_HTML;
  }

  base::FilePath path_copy(path);
  base::i18n::NormalizeFileNameEncoding(&path_copy);

  download_prefs_->SetSaveFilePath(path_copy.DirName());

#if defined(OS_CHROMEOS)
  if (drive::util::IsUnderDriveMountPoint(path_copy)) {
    // Here's a map to the callback chain:
    // SubstituteDriveDownloadPath ->
    //   ContinueSettingUpDriveDownload ->
    //     callback_ = SavePackage::OnPathPicked ->
    //       download_created_callback = OnSavePackageDownloadCreatedChromeOS
    Profile* profile = Profile::FromBrowserContext(
        process->GetBrowserContext());
    drive::DownloadHandler* drive_download_handler =
        drive::DownloadHandler::GetForProfile(profile);
    drive_download_handler->SubstituteDriveDownloadPath(
        path_copy, NULL, base::Bind(&ContinueSettingUpDriveDownload,
                                    callback_,
                                    save_type,
                                    profile,
                                    path_copy));
    return;
  }
#endif

  callback_.Run(path_copy, save_type,
                base::Bind(&OnSavePackageDownloadCreated));
}

void SavePackageFilePicker::FileSelectionCanceled(void* unused_params) {
  delete this;
}
