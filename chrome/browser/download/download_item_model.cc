// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/download/download_item_model.h"

#include "base/i18n/number_formatting.h"
#include "base/i18n/rtl.h"
#include "base/memory/ptr_util.h"
#include "base/metrics/field_trial.h"
#include "base/strings/string16.h"
#include "base/strings/sys_string_conversions.h"
#include "base/strings/utf_string_conversions.h"
#include "base/supports_user_data.h"
#include "base/time/time.h"
#include "chrome/browser/download/chrome_download_manager_delegate.h"
#include "chrome/browser/download/download_core_service.h"
#include "chrome/browser/download/download_core_service_factory.h"
#include "chrome/browser/download/download_crx_util.h"
#include "chrome/browser/download/download_history.h"
#include "chrome/browser/download/download_stats.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/safe_browsing/download_protection/download_feedback_service.h"
#include "chrome/common/safe_browsing/download_file_types.pb.h"
#include "chrome/grit/chromium_strings.h"
#include "chrome/grit/generated_resources.h"
#include "components/download/public/common/download_danger_type.h"
#include "components/download/public/common/download_interrupt_reasons.h"
#include "components/download/public/common/download_item.h"
#include "content/public/browser/download_item_utils.h"
#include "net/base/mime_util.h"
#include "third_party/blink/public/common/mime_util/mime_util.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/base/l10n/time_format.h"
#include "ui/base/text/bytes_formatting.h"
#include "ui/gfx/text_elider.h"

using base::TimeDelta;
using download::DownloadItem;
using safe_browsing::DownloadFileType;

namespace {

// Per DownloadItem data used by DownloadItemModel. The model doesn't keep any
// state since there could be multiple models associated with a single
// DownloadItem, and the lifetime of the model is shorter than the DownloadItem.
class DownloadItemModelData : public base::SupportsUserData::Data {
 public:
  ~DownloadItemModelData() override {}

  // Get the DownloadItemModelData object for |download|. Returns NULL if
  // there's no model data.
  static const DownloadItemModelData* Get(const DownloadItem* download);

  // Get the DownloadItemModelData object for |download|. Creates a model data
  // object if not found. Always returns a non-NULL pointer, unless OOM.
  static DownloadItemModelData* GetOrCreate(DownloadItem* download);

  // Whether the download should be displayed in the download shelf. True by
  // default.
  bool should_show_in_shelf_;

  // Whether the UI has been notified about this download.
  bool was_ui_notified_;

  // Whether the download should be opened in the browser vs. the system handler
  // for the file type.
  bool should_prefer_opening_in_browser_;

  // Danger level of the file determined based on the file type and whether
  // there was a user action associated with the download.
  DownloadFileType::DangerLevel danger_level_;

  // Whether the download is currently being revived.
  bool is_being_revived_;

 private:
  DownloadItemModelData();

  static const char kKey[];
};

// static
const char DownloadItemModelData::kKey[] = "DownloadItemModelData key";

// static
const DownloadItemModelData* DownloadItemModelData::Get(
    const DownloadItem* download) {
  return static_cast<const DownloadItemModelData*>(download->GetUserData(kKey));
}

// static
DownloadItemModelData* DownloadItemModelData::GetOrCreate(
    DownloadItem* download) {
  DownloadItemModelData* data =
      static_cast<DownloadItemModelData*>(download->GetUserData(kKey));
  if (data == NULL) {
    data = new DownloadItemModelData();
    data->should_show_in_shelf_ = !download->IsTransient();
    download->SetUserData(kKey, base::WrapUnique(data));
  }
  return data;
}

DownloadItemModelData::DownloadItemModelData()
    : should_show_in_shelf_(true),
      was_ui_notified_(false),
      should_prefer_opening_in_browser_(false),
      danger_level_(DownloadFileType::NOT_DANGEROUS),
      is_being_revived_(false) {}

base::string16 InterruptReasonStatusMessage(
    download::DownloadInterruptReason reason) {
  int string_id = IDS_DOWNLOAD_INTERRUPTED_STATUS;

  switch (reason) {
    case download::DOWNLOAD_INTERRUPT_REASON_FILE_ACCESS_DENIED:
      string_id = IDS_DOWNLOAD_INTERRUPTED_STATUS_ACCESS_DENIED;
      break;
    case download::DOWNLOAD_INTERRUPT_REASON_FILE_NO_SPACE:
      string_id = IDS_DOWNLOAD_INTERRUPTED_STATUS_DISK_FULL;
      break;
    case download::DOWNLOAD_INTERRUPT_REASON_FILE_NAME_TOO_LONG:
      string_id = IDS_DOWNLOAD_INTERRUPTED_STATUS_PATH_TOO_LONG;
      break;
    case download::DOWNLOAD_INTERRUPT_REASON_FILE_TOO_LARGE:
      string_id = IDS_DOWNLOAD_INTERRUPTED_STATUS_FILE_TOO_LARGE;
      break;
    case download::DOWNLOAD_INTERRUPT_REASON_FILE_VIRUS_INFECTED:
      string_id = IDS_DOWNLOAD_INTERRUPTED_STATUS_VIRUS;
      break;
    case download::DOWNLOAD_INTERRUPT_REASON_FILE_TRANSIENT_ERROR:
      string_id = IDS_DOWNLOAD_INTERRUPTED_STATUS_TEMPORARY_PROBLEM;
      break;
    case download::DOWNLOAD_INTERRUPT_REASON_FILE_BLOCKED:
      string_id = IDS_DOWNLOAD_INTERRUPTED_STATUS_BLOCKED;
      break;
    case download::DOWNLOAD_INTERRUPT_REASON_FILE_SECURITY_CHECK_FAILED:
      string_id = IDS_DOWNLOAD_INTERRUPTED_STATUS_SECURITY_CHECK_FAILED;
      break;
    case download::DOWNLOAD_INTERRUPT_REASON_FILE_TOO_SHORT:
      string_id = IDS_DOWNLOAD_INTERRUPTED_STATUS_FILE_TOO_SHORT;
      break;
    case download::DOWNLOAD_INTERRUPT_REASON_FILE_SAME_AS_SOURCE:
      string_id = IDS_DOWNLOAD_INTERRUPTED_STATUS_FILE_SAME_AS_SOURCE;
      break;
    case download::DOWNLOAD_INTERRUPT_REASON_NETWORK_INVALID_REQUEST:
    case download::DOWNLOAD_INTERRUPT_REASON_NETWORK_FAILED:
      string_id = IDS_DOWNLOAD_INTERRUPTED_STATUS_NETWORK_ERROR;
      break;
    case download::DOWNLOAD_INTERRUPT_REASON_NETWORK_TIMEOUT:
      string_id = IDS_DOWNLOAD_INTERRUPTED_STATUS_NETWORK_TIMEOUT;
      break;
    case download::DOWNLOAD_INTERRUPT_REASON_NETWORK_DISCONNECTED:
      string_id = IDS_DOWNLOAD_INTERRUPTED_STATUS_NETWORK_DISCONNECTED;
      break;
    case download::DOWNLOAD_INTERRUPT_REASON_NETWORK_SERVER_DOWN:
      string_id = IDS_DOWNLOAD_INTERRUPTED_STATUS_SERVER_DOWN;
      break;
    case download::DOWNLOAD_INTERRUPT_REASON_SERVER_FAILED:
      string_id = IDS_DOWNLOAD_INTERRUPTED_STATUS_SERVER_PROBLEM;
      break;
    case download::DOWNLOAD_INTERRUPT_REASON_SERVER_BAD_CONTENT:
      string_id = IDS_DOWNLOAD_INTERRUPTED_STATUS_NO_FILE;
      break;
    case download::DOWNLOAD_INTERRUPT_REASON_USER_CANCELED:
      string_id = IDS_DOWNLOAD_STATUS_CANCELLED;
      break;
    case download::DOWNLOAD_INTERRUPT_REASON_USER_SHUTDOWN:
      string_id = IDS_DOWNLOAD_INTERRUPTED_STATUS_SHUTDOWN;
      break;
    case download::DOWNLOAD_INTERRUPT_REASON_CRASH:
      string_id = IDS_DOWNLOAD_INTERRUPTED_STATUS_CRASH;
      break;
    case download::DOWNLOAD_INTERRUPT_REASON_SERVER_UNAUTHORIZED:
      string_id = IDS_DOWNLOAD_INTERRUPTED_STATUS_UNAUTHORIZED;
      break;
    case download::DOWNLOAD_INTERRUPT_REASON_SERVER_CERT_PROBLEM:
      string_id = IDS_DOWNLOAD_INTERRUPTED_STATUS_SERVER_CERT_PROBLEM;
      break;
    case download::DOWNLOAD_INTERRUPT_REASON_SERVER_FORBIDDEN:
      string_id = IDS_DOWNLOAD_INTERRUPTED_STATUS_FORBIDDEN;
      break;
    case download::DOWNLOAD_INTERRUPT_REASON_SERVER_UNREACHABLE:
      string_id = IDS_DOWNLOAD_INTERRUPTED_STATUS_UNREACHABLE;
      break;
    case download::DOWNLOAD_INTERRUPT_REASON_SERVER_CONTENT_LENGTH_MISMATCH:
      string_id = IDS_DOWNLOAD_INTERRUPTED_STATUS_CONTENT_LENGTH_MISMATCH;
      break;

    case download::DOWNLOAD_INTERRUPT_REASON_NONE:
      NOTREACHED();
      FALLTHROUGH;
    case download::DOWNLOAD_INTERRUPT_REASON_SERVER_NO_RANGE:
    case download::DOWNLOAD_INTERRUPT_REASON_FILE_FAILED:
    case download::DOWNLOAD_INTERRUPT_REASON_FILE_HASH_MISMATCH:
      string_id = IDS_DOWNLOAD_INTERRUPTED_STATUS;
  }

  return l10n_util::GetStringUTF16(string_id);
}

base::string16 InterruptReasonMessage(
    download::DownloadInterruptReason reason) {
  int string_id = IDS_DOWNLOAD_INTERRUPTED_STATUS;
  base::string16 status_text;

  switch (reason) {
    case download::DOWNLOAD_INTERRUPT_REASON_FILE_ACCESS_DENIED:
      string_id = IDS_DOWNLOAD_INTERRUPTED_DESCRIPTION_ACCESS_DENIED;
      break;
    case download::DOWNLOAD_INTERRUPT_REASON_FILE_NO_SPACE:
      string_id = IDS_DOWNLOAD_INTERRUPTED_DESCRIPTION_DISK_FULL;
      break;
    case download::DOWNLOAD_INTERRUPT_REASON_FILE_NAME_TOO_LONG:
      string_id = IDS_DOWNLOAD_INTERRUPTED_DESCRIPTION_PATH_TOO_LONG;
      break;
    case download::DOWNLOAD_INTERRUPT_REASON_FILE_TOO_LARGE:
      string_id = IDS_DOWNLOAD_INTERRUPTED_DESCRIPTION_FILE_TOO_LARGE;
      break;
    case download::DOWNLOAD_INTERRUPT_REASON_FILE_VIRUS_INFECTED:
      string_id = IDS_DOWNLOAD_INTERRUPTED_DESCRIPTION_VIRUS;
      break;
    case download::DOWNLOAD_INTERRUPT_REASON_FILE_TRANSIENT_ERROR:
      string_id = IDS_DOWNLOAD_INTERRUPTED_DESCRIPTION_TEMPORARY_PROBLEM;
      break;
    case download::DOWNLOAD_INTERRUPT_REASON_FILE_BLOCKED:
      string_id = IDS_DOWNLOAD_INTERRUPTED_DESCRIPTION_BLOCKED;
      break;
    case download::DOWNLOAD_INTERRUPT_REASON_FILE_SECURITY_CHECK_FAILED:
      string_id = IDS_DOWNLOAD_INTERRUPTED_DESCRIPTION_SECURITY_CHECK_FAILED;
      break;
    case download::DOWNLOAD_INTERRUPT_REASON_FILE_TOO_SHORT:
      string_id = IDS_DOWNLOAD_INTERRUPTED_DESCRIPTION_FILE_TOO_SHORT;
      break;
    case download::DOWNLOAD_INTERRUPT_REASON_FILE_SAME_AS_SOURCE:
      string_id = IDS_DOWNLOAD_INTERRUPTED_DESCRIPTION_FILE_SAME_AS_SOURCE;
      break;
    case download::DOWNLOAD_INTERRUPT_REASON_NETWORK_INVALID_REQUEST:
    case download::DOWNLOAD_INTERRUPT_REASON_NETWORK_FAILED:
      string_id = IDS_DOWNLOAD_INTERRUPTED_DESCRIPTION_NETWORK_ERROR;
      break;
    case download::DOWNLOAD_INTERRUPT_REASON_NETWORK_TIMEOUT:
      string_id = IDS_DOWNLOAD_INTERRUPTED_DESCRIPTION_NETWORK_TIMEOUT;
      break;
    case download::DOWNLOAD_INTERRUPT_REASON_NETWORK_DISCONNECTED:
      string_id = IDS_DOWNLOAD_INTERRUPTED_DESCRIPTION_NETWORK_DISCONNECTED;
      break;
    case download::DOWNLOAD_INTERRUPT_REASON_NETWORK_SERVER_DOWN:
      string_id = IDS_DOWNLOAD_INTERRUPTED_DESCRIPTION_SERVER_DOWN;
      break;
    case download::DOWNLOAD_INTERRUPT_REASON_SERVER_FAILED:
      string_id = IDS_DOWNLOAD_INTERRUPTED_DESCRIPTION_SERVER_PROBLEM;
      break;
    case download::DOWNLOAD_INTERRUPT_REASON_SERVER_BAD_CONTENT:
      string_id = IDS_DOWNLOAD_INTERRUPTED_DESCRIPTION_NO_FILE;
      break;
    case download::DOWNLOAD_INTERRUPT_REASON_USER_CANCELED:
      string_id = IDS_DOWNLOAD_STATUS_CANCELLED;
      break;
    case download::DOWNLOAD_INTERRUPT_REASON_USER_SHUTDOWN:
      string_id = IDS_DOWNLOAD_INTERRUPTED_DESCRIPTION_SHUTDOWN;
      break;
    case download::DOWNLOAD_INTERRUPT_REASON_CRASH:
      string_id = IDS_DOWNLOAD_INTERRUPTED_DESCRIPTION_CRASH;
      break;
    case download::DOWNLOAD_INTERRUPT_REASON_SERVER_UNAUTHORIZED:
      string_id = IDS_DOWNLOAD_INTERRUPTED_DESCRIPTION_UNAUTHORIZED;
      break;
    case download::DOWNLOAD_INTERRUPT_REASON_SERVER_CERT_PROBLEM:
      string_id = IDS_DOWNLOAD_INTERRUPTED_DESCRIPTION_SERVER_CERT_PROBLEM;
      break;
    case download::DOWNLOAD_INTERRUPT_REASON_SERVER_FORBIDDEN:
      string_id = IDS_DOWNLOAD_INTERRUPTED_DESCRIPTION_FORBIDDEN;
      break;
    case download::DOWNLOAD_INTERRUPT_REASON_SERVER_UNREACHABLE:
      string_id = IDS_DOWNLOAD_INTERRUPTED_DESCRIPTION_UNREACHABLE;
      break;
    case download::DOWNLOAD_INTERRUPT_REASON_SERVER_CONTENT_LENGTH_MISMATCH:
      string_id = IDS_DOWNLOAD_INTERRUPTED_DESCRIPTION_CONTENT_LENGTH_MISMATCH;
      break;
    case download::DOWNLOAD_INTERRUPT_REASON_NONE:
      NOTREACHED();
      FALLTHROUGH;
    // fallthrough
    case download::DOWNLOAD_INTERRUPT_REASON_SERVER_NO_RANGE:
    case download::DOWNLOAD_INTERRUPT_REASON_FILE_FAILED:
    case download::DOWNLOAD_INTERRUPT_REASON_FILE_HASH_MISMATCH:
      string_id = IDS_DOWNLOAD_INTERRUPTED_STATUS;
  }

  status_text = l10n_util::GetStringUTF16(string_id);

  return status_text;
}

} // namespace

// -----------------------------------------------------------------------------
// DownloadItemModel

DownloadItemModel::DownloadItemModel(DownloadItem* download)
    : download_(download) {}

DownloadItemModel::~DownloadItemModel() {}

base::string16 DownloadItemModel::GetInterruptReasonText() const {
  if (download_->GetState() != DownloadItem::INTERRUPTED ||
      download_->GetLastReason() ==
          download::DOWNLOAD_INTERRUPT_REASON_USER_CANCELED) {
    return base::string16();
  }
  return InterruptReasonMessage(download_->GetLastReason());
}

base::string16 DownloadItemModel::GetStatusText() const {
  base::string16 status_text;
  switch (download_->GetState()) {
    case DownloadItem::IN_PROGRESS:
      status_text = GetInProgressStatusString();
      break;
    case DownloadItem::COMPLETE:
      if (download_->GetFileExternallyRemoved()) {
        status_text = l10n_util::GetStringUTF16(IDS_DOWNLOAD_STATUS_REMOVED);
      } else {
        status_text.clear();
      }
      break;
    case DownloadItem::CANCELLED:
      status_text = l10n_util::GetStringUTF16(IDS_DOWNLOAD_STATUS_CANCELLED);
      break;
    case DownloadItem::INTERRUPTED: {
      download::DownloadInterruptReason reason = download_->GetLastReason();
      if (reason != download::DOWNLOAD_INTERRUPT_REASON_USER_CANCELED) {
        base::string16 interrupt_reason = InterruptReasonStatusMessage(reason);
        status_text = l10n_util::GetStringFUTF16(
            IDS_DOWNLOAD_STATUS_INTERRUPTED, interrupt_reason);
      } else {
        // Same as DownloadItem::CANCELLED.
        status_text = l10n_util::GetStringUTF16(IDS_DOWNLOAD_STATUS_CANCELLED);
      }
      break;
    }
    default:
      NOTREACHED();
  }

  return status_text;
}

base::string16 DownloadItemModel::GetTabProgressStatusText() const {
  int64_t total = GetTotalBytes();
  int64_t size = download_->GetReceivedBytes();
  base::string16 received_size = ui::FormatBytes(size);
  base::string16 amount = received_size;

  // Adjust both strings for the locale direction since we don't yet know which
  // string we'll end up using for constructing the final progress string.
  base::i18n::AdjustStringForLocaleDirection(&amount);

  if (total) {
    base::string16 total_text = ui::FormatBytes(total);
    base::i18n::AdjustStringForLocaleDirection(&total_text);

    base::i18n::AdjustStringForLocaleDirection(&received_size);
    amount = l10n_util::GetStringFUTF16(
        IDS_DOWNLOAD_TAB_PROGRESS_SIZE, received_size, total_text);
  } else {
    amount.assign(received_size);
  }
  int64_t current_speed = download_->CurrentSpeed();
  base::string16 speed_text = ui::FormatSpeed(current_speed);
  base::i18n::AdjustStringForLocaleDirection(&speed_text);

  base::TimeDelta remaining;
  base::string16 time_remaining;
  if (download_->IsPaused()) {
    time_remaining = l10n_util::GetStringUTF16(IDS_DOWNLOAD_PROGRESS_PAUSED);
  } else if (download_->TimeRemaining(&remaining)) {
    time_remaining = ui::TimeFormat::Simple(ui::TimeFormat::FORMAT_REMAINING,
                                            ui::TimeFormat::LENGTH_SHORT,
                                            remaining);
  }

  if (time_remaining.empty()) {
    base::i18n::AdjustStringForLocaleDirection(&amount);
    return l10n_util::GetStringFUTF16(
        IDS_DOWNLOAD_TAB_PROGRESS_STATUS_TIME_UNKNOWN, speed_text, amount);
  }
  return l10n_util::GetStringFUTF16(
      IDS_DOWNLOAD_TAB_PROGRESS_STATUS, speed_text, amount, time_remaining);
}

base::string16 DownloadItemModel::GetTooltipText(const gfx::FontList& font_list,
                                                 int max_width) const {
  base::string16 tooltip =
      gfx::ElideFilename(download_->GetFileNameToReportUser(), font_list,
                         max_width, gfx::Typesetter::NATIVE);
  download::DownloadInterruptReason reason = download_->GetLastReason();
  if (download_->GetState() == DownloadItem::INTERRUPTED &&
      reason != download::DOWNLOAD_INTERRUPT_REASON_USER_CANCELED) {
    tooltip += base::ASCIIToUTF16("\n");
    tooltip +=
        gfx::ElideText(InterruptReasonStatusMessage(reason), font_list,
                       max_width, gfx::ELIDE_TAIL, gfx::Typesetter::NATIVE);
  }
  return tooltip;
}

base::string16 DownloadItemModel::GetWarningText(const gfx::FontList& font_list,
                                                 int base_width) const {
  // Should only be called if IsDangerous().
  DCHECK(IsDangerous());
  base::string16 elided_filename =
      gfx::ElideFilename(download_->GetFileNameToReportUser(), font_list,
                         base_width, gfx::Typesetter::BROWSER);
  switch (download_->GetDangerType()) {
    case download::DOWNLOAD_DANGER_TYPE_DANGEROUS_URL: {
      return l10n_util::GetStringUTF16(IDS_PROMPT_MALICIOUS_DOWNLOAD_URL);
    }
    case download::DOWNLOAD_DANGER_TYPE_DANGEROUS_FILE: {
      if (download_crx_util::IsExtensionDownload(*download_)) {
        return l10n_util::GetStringUTF16(
            IDS_PROMPT_DANGEROUS_DOWNLOAD_EXTENSION);
      } else {
        return l10n_util::GetStringFUTF16(IDS_PROMPT_DANGEROUS_DOWNLOAD,
                                          elided_filename);
      }
    }
    case download::DOWNLOAD_DANGER_TYPE_DANGEROUS_CONTENT:
    case download::DOWNLOAD_DANGER_TYPE_DANGEROUS_HOST: {
      return l10n_util::GetStringFUTF16(IDS_PROMPT_MALICIOUS_DOWNLOAD_CONTENT,
                                        elided_filename);
    }
    case download::DOWNLOAD_DANGER_TYPE_UNCOMMON_CONTENT: {
      return l10n_util::GetStringFUTF16(IDS_PROMPT_UNCOMMON_DOWNLOAD_CONTENT,
                                        elided_filename);
    }
    case download::DOWNLOAD_DANGER_TYPE_POTENTIALLY_UNWANTED: {
      return l10n_util::GetStringFUTF16(
          IDS_PROMPT_DOWNLOAD_CHANGES_SETTINGS, elided_filename);
    }
    case download::DOWNLOAD_DANGER_TYPE_NOT_DANGEROUS:
    case download::DOWNLOAD_DANGER_TYPE_MAYBE_DANGEROUS_CONTENT:
    case download::DOWNLOAD_DANGER_TYPE_USER_VALIDATED:
    case download::DOWNLOAD_DANGER_TYPE_WHITELISTED_BY_POLICY:
    case download::DOWNLOAD_DANGER_TYPE_MAX: {
      break;
    }
  }
  NOTREACHED();
  return base::string16();
}

base::string16 DownloadItemModel::GetWarningConfirmButtonText() const {
  // Should only be called if IsDangerous()
  DCHECK(IsDangerous());
  if (download_->GetDangerType() ==
          download::DOWNLOAD_DANGER_TYPE_DANGEROUS_FILE &&
      download_crx_util::IsExtensionDownload(*download_)) {
    return l10n_util::GetStringUTF16(IDS_CONTINUE_EXTENSION_DOWNLOAD);
  } else {
    return l10n_util::GetStringUTF16(IDS_CONFIRM_DOWNLOAD);
  }
}

int64_t DownloadItemModel::GetCompletedBytes() const {
  return download_->GetReceivedBytes();
}

int64_t DownloadItemModel::GetTotalBytes() const {
  return download_->AllDataSaved() ? download_->GetReceivedBytes() :
                                     download_->GetTotalBytes();
}

// TODO(asanka,rdsmith): Once 'open' moves exclusively to the
//     ChromeDownloadManagerDelegate, we should calculate the percentage here
//     instead of calling into the DownloadItem.
int DownloadItemModel::PercentComplete() const {
  return download_->PercentComplete();
}

bool DownloadItemModel::IsDangerous() const {
  return download_->IsDangerous();
}

bool DownloadItemModel::MightBeMalicious() const {
  if (!IsDangerous())
    return false;
  switch (download_->GetDangerType()) {
    case download::DOWNLOAD_DANGER_TYPE_DANGEROUS_URL:
    case download::DOWNLOAD_DANGER_TYPE_DANGEROUS_CONTENT:
    case download::DOWNLOAD_DANGER_TYPE_UNCOMMON_CONTENT:
    case download::DOWNLOAD_DANGER_TYPE_DANGEROUS_HOST:
    case download::DOWNLOAD_DANGER_TYPE_POTENTIALLY_UNWANTED:
      return true;

    case download::DOWNLOAD_DANGER_TYPE_NOT_DANGEROUS:
    case download::DOWNLOAD_DANGER_TYPE_MAYBE_DANGEROUS_CONTENT:
    case download::DOWNLOAD_DANGER_TYPE_USER_VALIDATED:
    case download::DOWNLOAD_DANGER_TYPE_WHITELISTED_BY_POLICY:
    case download::DOWNLOAD_DANGER_TYPE_MAX:
      // We shouldn't get any of these due to the IsDangerous() test above.
      NOTREACHED();
      FALLTHROUGH;
    case download::DOWNLOAD_DANGER_TYPE_DANGEROUS_FILE:
      return false;
  }
  NOTREACHED();
  return false;
}

// If you change this definition of malicious, also update
// DownloadManagerImpl::NonMaliciousInProgressCount.
bool DownloadItemModel::IsMalicious() const {
  if (!MightBeMalicious())
    return false;
  switch (download_->GetDangerType()) {
    case download::DOWNLOAD_DANGER_TYPE_DANGEROUS_URL:
    case download::DOWNLOAD_DANGER_TYPE_DANGEROUS_CONTENT:
    case download::DOWNLOAD_DANGER_TYPE_DANGEROUS_HOST:
    case download::DOWNLOAD_DANGER_TYPE_POTENTIALLY_UNWANTED:
      return true;

    case download::DOWNLOAD_DANGER_TYPE_NOT_DANGEROUS:
    case download::DOWNLOAD_DANGER_TYPE_MAYBE_DANGEROUS_CONTENT:
    case download::DOWNLOAD_DANGER_TYPE_USER_VALIDATED:
    case download::DOWNLOAD_DANGER_TYPE_WHITELISTED_BY_POLICY:
    case download::DOWNLOAD_DANGER_TYPE_MAX:
    case download::DOWNLOAD_DANGER_TYPE_DANGEROUS_FILE:
      // We shouldn't get any of these due to the MightBeMalicious() test above.
      NOTREACHED();
      FALLTHROUGH;
    case download::DOWNLOAD_DANGER_TYPE_UNCOMMON_CONTENT:
      return false;
  }
  NOTREACHED();
  return false;
}

bool DownloadItemModel::HasSupportedImageMimeType() const {
  if (blink::IsSupportedImageMimeType(download_->GetMimeType())) {
    return true;
  }

  std::string mime;
  base::FilePath::StringType extension_with_dot =
      download_->GetTargetFilePath().FinalExtension();
  if (!extension_with_dot.empty() &&
      net::GetWellKnownMimeTypeFromExtension(extension_with_dot.substr(1),
                                             &mime) &&
      blink::IsSupportedImageMimeType(mime)) {
    return true;
  }

  return false;
}

bool DownloadItemModel::ShouldAllowDownloadFeedback() const {
#if defined(FULL_SAFE_BROWSING)
  if (!IsDangerous())
    return false;
  return safe_browsing::DownloadFeedbackService::IsEnabledForDownload(
      *download_);
#else
  return false;
#endif
}

bool DownloadItemModel::ShouldRemoveFromShelfWhenComplete() const {
  switch (download_->GetState()) {
    case DownloadItem::IN_PROGRESS:
      // If the download is dangerous or malicious, we should display a warning
      // on the shelf until the user accepts the download.
      if (IsDangerous())
        return false;

      // If the download is an extension, temporary, or will be opened
      // automatically, then it should be removed from the shelf on completion.
      // TODO(asanka): The logic for deciding opening behavior should be in a
      //               central location. http://crbug.com/167702
      return (download_crx_util::IsExtensionDownload(*download_) ||
              download_->IsTemporary() ||
              download_->GetOpenWhenComplete() ||
              download_->ShouldOpenFileBasedOnExtension());

    case DownloadItem::COMPLETE:
      // If the download completed, then rely on GetAutoOpened() to check for
      // opening behavior. This should accurately reflect whether the download
      // was successfully opened.  Extensions, for example, may fail to open.
      return download_->GetAutoOpened() || download_->IsTemporary();

    case DownloadItem::CANCELLED:
    case DownloadItem::INTERRUPTED:
      // Interrupted or cancelled downloads should remain on the shelf.
      return false;

    case DownloadItem::MAX_DOWNLOAD_STATE:
      NOTREACHED();
  }

  NOTREACHED();
  return false;
}

bool DownloadItemModel::ShouldShowDownloadStartedAnimation() const {
  return !download_->IsSavePackageDownload() &&
      !download_crx_util::IsExtensionDownload(*download_);
}

bool DownloadItemModel::ShouldShowInShelf() const {
  const DownloadItemModelData* data = DownloadItemModelData::Get(download_);
  if (data)
    return data->should_show_in_shelf_;

  return !download_->IsTransient();
}

void DownloadItemModel::SetShouldShowInShelf(bool should_show) {
  DownloadItemModelData* data = DownloadItemModelData::GetOrCreate(download_);
  data->should_show_in_shelf_ = should_show;
}

bool DownloadItemModel::ShouldNotifyUI() const {
  if (download_->IsTransient())
    return false;

  Profile* profile = Profile::FromBrowserContext(
      content::DownloadItemUtils::GetBrowserContext(download_));
  DownloadCoreService* download_core_service =
      DownloadCoreServiceFactory::GetForBrowserContext(profile);
  DownloadHistory* download_history =
      (download_core_service ? download_core_service->GetDownloadHistory()
                             : nullptr);

  // The browser is only interested in new downloads. Ones that were restored
  // from history are not displayed on the shelf. The downloads page
  // independently listens for new downloads when it is active. Note that the UI
  // will be notified of downloads even if they are not meant to be displayed on
  // the shelf (i.e. ShouldShowInShelf() returns false). This is because:
  // *  The shelf isn't the only UI. E.g. on Android, the UI is the system
  //    DownloadManager.
  // *  There are other UI activities that need to be performed. E.g. if the
  //    download was initiated from a new tab, then that tab should be closed.
  //
  // TODO(asanka): If an interrupted download is restored from history and is
  // resumed, then ideally the UI should be notified.
  return !download_history ||
         !download_history->WasRestoredFromHistory(download_);
}

bool DownloadItemModel::WasUINotified() const {
  const DownloadItemModelData* data = DownloadItemModelData::Get(download_);
  return data && data->was_ui_notified_;
}

void DownloadItemModel::SetWasUINotified(bool was_ui_notified) {
  DownloadItemModelData* data = DownloadItemModelData::GetOrCreate(download_);
  data->was_ui_notified_ = was_ui_notified;
}

bool DownloadItemModel::ShouldPreferOpeningInBrowser() const {
  const DownloadItemModelData* data = DownloadItemModelData::Get(download_);
  return data && data->should_prefer_opening_in_browser_;
}

void DownloadItemModel::SetShouldPreferOpeningInBrowser(bool preference) {
  DownloadItemModelData* data = DownloadItemModelData::GetOrCreate(download_);
  data->should_prefer_opening_in_browser_ = preference;
}

DownloadFileType::DangerLevel DownloadItemModel::GetDangerLevel() const {
  const DownloadItemModelData* data = DownloadItemModelData::Get(download_);
  return data ? data->danger_level_ : DownloadFileType::NOT_DANGEROUS;
}

void DownloadItemModel::SetDangerLevel(
    DownloadFileType::DangerLevel danger_level) {
  DownloadItemModelData* data = DownloadItemModelData::GetOrCreate(download_);
  data->danger_level_ = danger_level;
}

bool DownloadItemModel::IsBeingRevived() const {
  const DownloadItemModelData* data = DownloadItemModelData::Get(download_);
  return data && data->is_being_revived_;
}

void DownloadItemModel::SetIsBeingRevived(bool is_being_revived) {
  DownloadItemModelData* data = DownloadItemModelData::GetOrCreate(download_);
  data->is_being_revived_ = is_being_revived;
}

base::string16 DownloadItemModel::GetProgressSizesString() const {
  base::string16 size_ratio;
  int64_t size = GetCompletedBytes();
  int64_t total = GetTotalBytes();
  if (total > 0) {
    ui::DataUnits amount_units = ui::GetByteDisplayUnits(total);
    base::string16 simple_size = ui::FormatBytesWithUnits(size, amount_units, false);

    // In RTL locales, we render the text "size/total" in an RTL context. This
    // is problematic since a string such as "123/456 MB" is displayed
    // as "MB 123/456" because it ends with an LTR run. In order to solve this,
    // we mark the total string as an LTR string if the UI layout is
    // right-to-left so that the string "456 MB" is treated as an LTR run.
    base::string16 simple_total = base::i18n::GetDisplayStringInLTRDirectionality(
        ui::FormatBytesWithUnits(total, amount_units, true));
    size_ratio = l10n_util::GetStringFUTF16(IDS_DOWNLOAD_STATUS_SIZES,
                                            simple_size, simple_total);
  } else {
    size_ratio = ui::FormatBytes(size);
  }
  return size_ratio;
}

base::string16 DownloadItemModel::GetInProgressStatusString() const {
  DCHECK_EQ(DownloadItem::IN_PROGRESS, download_->GetState());

  TimeDelta time_remaining;
  // time_remaining is only known if the download isn't paused.
  bool time_remaining_known = (!download_->IsPaused() &&
                               download_->TimeRemaining(&time_remaining));

  // Indication of progress. (E.g.:"100/200 MB" or "100MB")
  base::string16 size_ratio = GetProgressSizesString();

  // The download is a CRX (app, extension, theme, ...) and it is being unpacked
  // and validated.
  if (download_->AllDataSaved() &&
      download_crx_util::IsExtensionDownload(*download_)) {
    return l10n_util::GetStringUTF16(IDS_DOWNLOAD_STATUS_CRX_INSTALL_RUNNING);
  }

  // A paused download: "100/120 MB, Paused"
  if (download_->IsPaused()) {
    return l10n_util::GetStringFUTF16(
        IDS_DOWNLOAD_STATUS_IN_PROGRESS, size_ratio,
        l10n_util::GetStringUTF16(IDS_DOWNLOAD_PROGRESS_PAUSED));
  }

  // A download scheduled to be opened when complete: "Opening in 10 secs"
  if (download_->GetOpenWhenComplete()) {
    if (!time_remaining_known)
      return l10n_util::GetStringUTF16(IDS_DOWNLOAD_STATUS_OPEN_WHEN_COMPLETE);

    return l10n_util::GetStringFUTF16(
        IDS_DOWNLOAD_STATUS_OPEN_IN,
        ui::TimeFormat::Simple(ui::TimeFormat::FORMAT_DURATION,
                               ui::TimeFormat::LENGTH_SHORT, time_remaining));
  }

  // In progress download with known time left: "100/120 MB, 10 secs left"
  if (time_remaining_known) {
    return l10n_util::GetStringFUTF16(
        IDS_DOWNLOAD_STATUS_IN_PROGRESS, size_ratio,
        ui::TimeFormat::Simple(ui::TimeFormat::FORMAT_REMAINING,
                               ui::TimeFormat::LENGTH_SHORT, time_remaining));
  }

  // In progress download with no known time left and non-zero completed bytes:
  // "100/120 MB" or "100 MB"
  if (GetCompletedBytes() > 0)
    return size_ratio;

  // Instead of displaying "0 B" we say "Starting..."
  return l10n_util::GetStringUTF16(IDS_DOWNLOAD_STATUS_STARTING);
}

void DownloadItemModel::OpenUsingPlatformHandler() {
  DownloadCoreService* download_core_service =
      DownloadCoreServiceFactory::GetForBrowserContext(
          content::DownloadItemUtils::GetBrowserContext(download_));
  if (!download_core_service)
    return;

  ChromeDownloadManagerDelegate* delegate =
      download_core_service->GetDownloadManagerDelegate();
  if (!delegate)
    return;
  delegate->OpenDownloadUsingPlatformHandler(download_);
  RecordDownloadOpenMethod(DOWNLOAD_OPEN_METHOD_USER_PLATFORM);
}
