// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/webui/print_preview/pdf_printer_handler.h"

#include <memory>
#include <string>
#include <utility>

#include "base/callback.h"
#include "base/command_line.h"
#include "base/files/file_util.h"
#include "base/i18n/file_util_icu.h"
#include "base/memory/ref_counted.h"
#include "base/strings/string16.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/utf_string_conversions.h"
#include "base/task_scheduler/post_task.h"
#include "base/values.h"
#include "build/build_config.h"
#include "chrome/browser/app_mode/app_mode_utils.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/download/download_prefs.h"
#include "chrome/browser/platform_util.h"
#include "chrome/browser/printing/print_preview_dialog_controller.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/chrome_select_file_policy.h"
#include "chrome/browser/ui/webui/print_preview/sticky_settings.h"
#include "chrome/common/chrome_switches.h"
#include "chrome/grit/generated_resources.h"
#include "components/cloud_devices/common/printer_description.h"
#include "components/url_formatter/url_formatter.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/web_contents.h"
#include "net/base/filename_util.h"
#include "printing/print_job_constants.h"
#include "printing/printing_context.h"
#include "printing/units.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/gfx/geometry/size.h"

namespace {

constexpr base::FilePath::CharType kPdfExtension[] = FILE_PATH_LITERAL("pdf");

class PrintingContextDelegate : public printing::PrintingContext::Delegate {
 public:
  // PrintingContext::Delegate methods.
  gfx::NativeView GetParentView() override { return NULL; }
  std::string GetAppLocale() override {
    return g_browser_process->GetApplicationLocale();
  }
};

gfx::Size GetDefaultPdfMediaSizeMicrons() {
  PrintingContextDelegate delegate;
  std::unique_ptr<printing::PrintingContext> printing_context(
      printing::PrintingContext::Create(&delegate));
  if (printing::PrintingContext::OK != printing_context->UsePdfSettings() ||
      printing_context->settings().device_units_per_inch() <= 0) {
    return gfx::Size();
  }
  gfx::Size pdf_media_size = printing_context->GetPdfPaperSizeDeviceUnits();
  float deviceMicronsPerDeviceUnit =
      (printing::kHundrethsMMPerInch * 10.0f) /
      printing_context->settings().device_units_per_inch();
  return gfx::Size(pdf_media_size.width() * deviceMicronsPerDeviceUnit,
                   pdf_media_size.height() * deviceMicronsPerDeviceUnit);
}

std::unique_ptr<base::DictionaryValue> GetPdfCapabilities(
    const std::string& locale) {
  cloud_devices::CloudDeviceDescription description;
  using namespace cloud_devices::printer;

  OrientationCapability orientation;
  orientation.AddOption(cloud_devices::printer::PORTRAIT);
  orientation.AddOption(cloud_devices::printer::LANDSCAPE);
  orientation.AddDefaultOption(AUTO_ORIENTATION, true);
  orientation.SaveTo(&description);

  ColorCapability color;
  {
    Color standard_color(STANDARD_COLOR);
    standard_color.vendor_id = base::IntToString(printing::COLOR);
    color.AddDefaultOption(standard_color, true);
  }
  color.SaveTo(&description);

  static const cloud_devices::printer::MediaType kPdfMedia[] = {
      ISO_A0, ISO_A1,   ISO_A2,    ISO_A3,   ISO_A4,
      ISO_A5, NA_LEGAL, NA_LETTER, NA_LEDGER};
  const gfx::Size default_media_size = GetDefaultPdfMediaSizeMicrons();
  Media default_media("", "", default_media_size.width(),
                      default_media_size.height());
  if (!default_media.MatchBySize() ||
      std::find(kPdfMedia, kPdfMedia + arraysize(kPdfMedia),
                default_media.type) == kPdfMedia + arraysize(kPdfMedia)) {
    default_media = Media(locale == "en-US" ? NA_LETTER : ISO_A4);
  }
  MediaCapability media;
  for (const auto& pdf_media : kPdfMedia) {
    Media media_option(pdf_media);
    media.AddDefaultOption(media_option,
                           default_media.type == media_option.type);
  }
  media.SaveTo(&description);

  return std::unique_ptr<base::DictionaryValue>(description.root().DeepCopy());
}

// Callback that stores a PDF file on disk.
void PrintToPdfCallback(const scoped_refptr<base::RefCountedMemory>& data,
                        const base::FilePath& path,
                        const base::Closure& pdf_file_saved_closure) {
  base::File file(path,
                  base::File::FLAG_CREATE_ALWAYS | base::File::FLAG_WRITE);
  file.WriteAtCurrentPos(reinterpret_cast<const char*>(data->front()),
                         base::checked_cast<int>(data->size()));
  if (!pdf_file_saved_closure.is_null())
    pdf_file_saved_closure.Run();
}

// Returns a unique path for |path|, just like with downloads.
base::FilePath GetUniquePath(const base::FilePath& path) {
  base::FilePath unique_path = path;
  int uniquifier =
      base::GetUniquePathNumber(path, base::FilePath::StringType());
  if (uniquifier > 0) {
    unique_path = unique_path.InsertBeforeExtensionASCII(
        base::StringPrintf(" (%d)", uniquifier));
  }
  return unique_path;
}

base::FilePath SelectSaveDirectory(const base::FilePath& path,
                                   const base::FilePath& default_path) {
  if (base::DirectoryExists(path))
    return path;
  if (!base::DirectoryExists(default_path))
    base::CreateDirectory(default_path);
  return default_path;
}

}  // namespace

PdfPrinterHandler::PdfPrinterHandler(Profile* profile,
                                     content::WebContents* preview_web_contents,
                                     printing::StickySettings* sticky_settings)
    : preview_web_contents_(preview_web_contents),
      profile_(profile),
      sticky_settings_(sticky_settings),
      weak_ptr_factory_(this) {}

PdfPrinterHandler::~PdfPrinterHandler() {
  if (select_file_dialog_.get())
    select_file_dialog_->ListenerDestroyed();
}

void PdfPrinterHandler::Reset() {
  weak_ptr_factory_.InvalidateWeakPtrs();
}

void PdfPrinterHandler::StartGetPrinters(
    const AddedPrintersCallback& added_printers_callback,
    GetPrintersDoneCallback done_callback) {
  NOTREACHED();
}

void PdfPrinterHandler::StartGetCapability(const std::string& destination_id,
                                           GetCapabilityCallback callback) {
  auto printer_info = std::make_unique<base::DictionaryValue>();
  printer_info->SetString(printing::kSettingDeviceName, destination_id);
  printer_info->Set(
      printing::kSettingCapabilities,
      GetPdfCapabilities(g_browser_process->GetApplicationLocale()));
  std::move(callback).Run(std::move(printer_info));
}

void PdfPrinterHandler::StartPrint(
    const std::string& destination_id,
    const std::string& capability,
    const base::string16& job_title,
    const std::string& ticket_json,
    const gfx::Size& page_size,
    const scoped_refptr<base::RefCountedMemory>& print_data,
    PrintCallback callback) {
  print_data_ = print_data;
  if (!print_to_pdf_path_.empty()) {
    // User has already selected a path, no need to show the dialog again.
    PostPrintToPdfTask();
    return;
  }

  if (select_file_dialog_ &&
      select_file_dialog_->IsRunning(
          platform_util::GetTopLevel(preview_web_contents_->GetNativeView()))) {
    // Dialog is already showing.
    return;
  }

  DCHECK(!print_callback_);
  print_callback_ = std::move(callback);

  printing::PrintPreviewDialogController* dialog_controller =
      printing::PrintPreviewDialogController::GetInstance();
  content::WebContents* initiator =
      dialog_controller ? dialog_controller->GetInitiator(preview_web_contents_)
                        : nullptr;

  GURL initiator_url;
  bool is_savable = false;
  if (initiator) {
    initiator_url = initiator->GetLastCommittedURL();
    is_savable = initiator->IsSavable();
  }
  base::FilePath path = GetFileName(initiator_url, job_title, is_savable);

  base::CommandLine* cmdline = base::CommandLine::ForCurrentProcess();
  bool prompt_user = !cmdline->HasSwitch(switches::kKioskModePrinting);

  SelectFile(path, initiator, prompt_user);
}

void PdfPrinterHandler::FileSelected(const base::FilePath& path,
                                     int /* index */,
                                     void* /* params */) {
  // Update downloads location and save sticky settings.
  DownloadPrefs* download_prefs = DownloadPrefs::FromBrowserContext(profile_);
  download_prefs->SetSaveFilePath(path.DirName());
  sticky_settings_->SaveInPrefs(profile_->GetPrefs());
  print_to_pdf_path_ = path;
  PostPrintToPdfTask();
}

void PdfPrinterHandler::FileSelectionCanceled(void* params) {
  std::move(print_callback_).Run(base::Value("PDFPrintCanceled"));
}

void PdfPrinterHandler::SetPdfSavedClosureForTesting(
    const base::Closure& closure) {
  pdf_file_saved_closure_ = closure;
}

// static
base::FilePath PdfPrinterHandler::GetFileNameForPrintJobTitle(
    const base::string16& job_title) {
  DCHECK(!job_title.empty());
#if defined(OS_WIN)
  base::FilePath::StringType print_job_title(job_title);
#elif defined(OS_POSIX)
  base::FilePath::StringType print_job_title = base::UTF16ToUTF8(job_title);
#endif

  base::i18n::ReplaceIllegalCharactersInPath(&print_job_title, '_');
  base::FilePath default_filename(print_job_title);
  base::FilePath::StringType ext = default_filename.Extension();
  if (!ext.empty()) {
    ext = ext.substr(1);
    if (ext == kPdfExtension)
      return default_filename;
  }
  return default_filename.AddExtension(kPdfExtension);
}

// static
base::FilePath PdfPrinterHandler::GetFileNameForURL(const GURL& url) {
  DCHECK(url.is_valid());

  // TODO(thestig): This code is based on similar code in SavePackage in
  // content/ that is not exposed via the public content API. Consider looking
  // for a sane way to share the code.
  if (url.SchemeIs(url::kDataScheme)) {
    return base::FilePath::FromUTF8Unsafe("dataurl").ReplaceExtension(
        kPdfExtension);
  }

  base::FilePath name =
      net::GenerateFileName(url, std::string(), std::string(), std::string(),
                            std::string(), std::string());

  // If host is used as file name, try to decode punycode.
  if (name.AsUTF8Unsafe() == url.host()) {
    name = base::FilePath::FromUTF16Unsafe(
        url_formatter::IDNToUnicode(url.host()));
  }
  if (name.AsUTF8Unsafe() == url.host())
    return name.AddExtension(kPdfExtension);
  return name.ReplaceExtension(kPdfExtension);
}

// static
base::FilePath PdfPrinterHandler::GetFileName(const GURL& url,
                                              const base::string16& job_title,
                                              bool is_savable) {
  if (is_savable) {
    bool title_is_url =
        job_title.empty() || url_formatter::FormatUrl(url) == job_title;
    return title_is_url ? GetFileNameForURL(url)
                        : GetFileNameForPrintJobTitle(job_title);
  }
  base::FilePath name = net::GenerateFileName(
      url, std::string(), std::string(), std::string(), std::string(),
      l10n_util::GetStringUTF8(IDS_DEFAULT_DOWNLOAD_FILENAME));
  return name.ReplaceExtension(kPdfExtension);
}

void PdfPrinterHandler::SelectFile(const base::FilePath& default_filename,
                                   content::WebContents* initiator,
                                   bool prompt_user) {
  if (prompt_user) {
    ChromeSelectFilePolicy policy(initiator);
    if (!policy.CanOpenSelectFileDialog()) {
      policy.SelectFileDenied();
      std::move(print_callback_).Run(base::Value("PDFPrintCannotSelect"));
      return;
    }
  }

  // Get save location from Download Preferences.
  DownloadPrefs* download_prefs = DownloadPrefs::FromBrowserContext(profile_);
  base::FilePath path = download_prefs->SaveFilePath();
  sticky_settings_->SaveInPrefs(profile_->GetPrefs());

  // Handle the no prompting case. Like the dialog prompt, this function
  // returns and eventually FileSelected() gets called.
  if (!prompt_user) {
    base::PostTaskWithTraitsAndReplyWithResult(
        FROM_HERE, {base::MayBlock(), base::TaskPriority::BACKGROUND},
        base::Bind(&GetUniquePath, path.Append(default_filename)),
        base::Bind(&PdfPrinterHandler::OnGotUniqueFileName,
                   weak_ptr_factory_.GetWeakPtr()));
    return;
  }

  // If the directory is empty there is no reason to create it or use the
  // default location.
  if (path.empty()) {
    OnDirectorySelected(default_filename, path);
    return;
  }

  // Get default download directory. This will be used as a fallback if the
  // save directory does not exist.
  base::FilePath default_path = download_prefs->DownloadPath();
  base::PostTaskWithTraitsAndReplyWithResult(
      FROM_HERE, {base::MayBlock(), base::TaskPriority::BACKGROUND},
      base::BindOnce(&SelectSaveDirectory, path, default_path),
      base::BindOnce(&PdfPrinterHandler::OnDirectorySelected,
                     weak_ptr_factory_.GetWeakPtr(), default_filename));
}

void PdfPrinterHandler::PostPrintToPdfTask() {
  base::PostTaskWithTraits(
      FROM_HERE, {base::MayBlock(), base::TaskPriority::BACKGROUND},
      base::BindOnce(&PrintToPdfCallback, print_data_, print_to_pdf_path_,
                     pdf_file_saved_closure_));
  print_to_pdf_path_.clear();
  std::move(print_callback_).Run(base::Value());
}

void PdfPrinterHandler::OnGotUniqueFileName(const base::FilePath& path) {
  FileSelected(path, 0, nullptr);
}

void PdfPrinterHandler::OnDirectorySelected(const base::FilePath& filename,
                                            const base::FilePath& directory) {
  base::FilePath path = directory.Append(filename);

  // Prompts the user to select the file.
  ui::SelectFileDialog::FileTypeInfo file_type_info;
  file_type_info.extensions.resize(1);
  file_type_info.extensions[0].push_back(FILE_PATH_LITERAL("pdf"));
  file_type_info.include_all_files = true;
  // Print Preview requires native paths to write PDF files.
  // Note that Chrome OS save-as dialog has Google Drive as a saving location
  // even when a client requires native paths. In this case, Chrome OS save-as
  // dialog returns native paths to write files and uploads the saved files to
  // Google Drive later.
  file_type_info.allowed_paths =
      ui::SelectFileDialog::FileTypeInfo::NATIVE_PATH;

  select_file_dialog_ =
      ui::SelectFileDialog::Create(this, nullptr /*policy already checked*/);
  select_file_dialog_->SelectFile(
      ui::SelectFileDialog::SELECT_SAVEAS_FILE, base::string16(), path,
      &file_type_info, 0, base::FilePath::StringType(),
      platform_util::GetTopLevel(preview_web_contents_->GetNativeView()), NULL);
}
