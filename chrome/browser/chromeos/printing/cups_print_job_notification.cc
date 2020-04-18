// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chromeos/printing/cups_print_job_notification.h"

#include <memory>
#include <string>
#include <vector>

#include "ash/public/cpp/vector_icons/vector_icons.h"
#include "base/strings/string16.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/utf_string_conversions.h"
#include "chrome/browser/chromeos/printing/cups_print_job.h"
#include "chrome/browser/chromeos/printing/cups_print_job_manager.h"
#include "chrome/browser/chromeos/printing/cups_print_job_manager_factory.h"
#include "chrome/browser/chromeos/printing/cups_print_job_notification_manager.h"
#include "chrome/browser/notifications/notification_display_service.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/grit/generated_resources.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/base/resource/resource_bundle.h"
#include "ui/chromeos/resources/grit/ui_chromeos_resources.h"
#include "ui/message_center/public/cpp/message_center_constants.h"
#include "ui/message_center/public/cpp/notification.h"
#include "ui/message_center/public/cpp/notification_delegate.h"

namespace chromeos {

namespace {

const char kCupsPrintJobNotificationId[] =
    "chrome://settings/printing/cups-print-job-notification";

}  // namespace

CupsPrintJobNotification::CupsPrintJobNotification(
    CupsPrintJobNotificationManager* manager,
    base::WeakPtr<CupsPrintJob> print_job,
    Profile* profile)
    : notification_manager_(manager),
      notification_id_(print_job->GetUniqueId()),
      print_job_(print_job),
      profile_(profile),
      weak_factory_(this) {
  // Create a notification for the print job. The title, body, icon and buttons
  // of the notification will be updated in UpdateNotification().
  notification_ = std::make_unique<message_center::Notification>(
      message_center::NOTIFICATION_TYPE_SIMPLE, notification_id_,
      base::string16(),  // title
      base::string16(),  // body
      gfx::Image(),      // icon
      l10n_util::GetStringUTF16(IDS_PRINT_JOB_NOTIFICATION_DISPLAY_SOURCE),
      GURL(kCupsPrintJobNotificationId),
      message_center::NotifierId(message_center::NotifierId::SYSTEM_COMPONENT,
                                 kCupsPrintJobNotificationId),
      message_center::RichNotificationData(),
      base::MakeRefCounted<message_center::ThunkNotificationDelegate>(
          weak_factory_.GetWeakPtr()));
  UpdateNotification();
}

CupsPrintJobNotification::~CupsPrintJobNotification() {}

void CupsPrintJobNotification::OnPrintJobStatusUpdated() {
  // After cancellation, ignore all updates.
  if (cancelled_by_user_)
    return;

  UpdateNotification();
}

void CupsPrintJobNotification::Close(bool by_user) {
  if (!by_user)
    return;

  closed_in_middle_ = true;
  if (!print_job_ ||
      print_job_->state() == CupsPrintJob::State::STATE_SUSPENDED) {
    notification_manager_->OnPrintJobNotificationRemoved(this);
  }
}

void CupsPrintJobNotification::Click(
    const base::Optional<int>& button_index,
    const base::Optional<base::string16>& reply) {
  if (!button_index)
    return;

  DCHECK(*button_index >= 0 &&
         static_cast<size_t>(*button_index) < button_commands_.size());

  CupsPrintJobManager* print_job_manager =
      CupsPrintJobManagerFactory::GetForBrowserContext(profile_);

  if (!print_job_)
    return;

  switch (button_commands_[*button_index]) {
    case ButtonCommand::CANCEL_PRINTING:
      print_job_manager->CancelPrintJob(print_job_.get());
      // print_job_ was deleted in CancelPrintJob.  Forget the pointer.
      print_job_ = nullptr;

      // Clean up the notification.
      NotificationDisplayService::GetForProfile(profile_)->Close(
          NotificationHandler::Type::TRANSIENT, notification_id_);
      cancelled_by_user_ = true;
      notification_manager_->OnPrintJobNotificationRemoved(this);
      break;
    case ButtonCommand::PAUSE_PRINTING:
      print_job_manager->SuspendPrintJob(print_job_.get());
      break;
    case ButtonCommand::RESUME_PRINTING:
      print_job_manager->ResumePrintJob(print_job_.get());
      break;
    case ButtonCommand::GET_HELP:
      break;
  }
}

void CupsPrintJobNotification::UpdateNotification() {
  if (!print_job_)
    return;
  UpdateNotificationTitle();
  UpdateNotificationIcon();
  UpdateNotificationBodyMessage();
  UpdateNotificationType();
  UpdateNotificationButtons();

  // |STATE_STARTED| and |STATE_PAGE_DONE| are special since if the user closes
  // the notification in the middle, which means they're not interested in the
  // printing progress, we should prevent showing the following printing
  // progress to the user.
  NotificationDisplayService* display_service =
      NotificationDisplayService::GetForProfile(profile_);
  if (print_job_->state() == CupsPrintJob::State::STATE_STARTED ||
      print_job_->state() == CupsPrintJob::State::STATE_PAGE_DONE) {
    // If the notification was closed during the printing, prevent showing the
    // following printing progress.
    if (!closed_in_middle_)
      display_service->Display(NotificationHandler::Type::TRANSIENT,
                               *notification_);
  } else {
    closed_in_middle_ = false;
    // In order to make sure it pop up, we should delete it before readding it.
    display_service->Close(NotificationHandler::Type::TRANSIENT,
                           notification_id_);
    display_service->Display(NotificationHandler::Type::TRANSIENT,
                             *notification_);
  }

  // |print_job_| will be deleted by CupsPrintJobManager if the job is finished
  // and we are not supposed to get any notification update after that.
  if (print_job_->IsJobFinished())
    print_job_ = nullptr;
}

void CupsPrintJobNotification::UpdateNotificationTitle() {
  if (!print_job_)
    return;
  base::string16 title;
  switch (print_job_->state()) {
    case CupsPrintJob::State::STATE_WAITING:
    case CupsPrintJob::State::STATE_STARTED:
    case CupsPrintJob::State::STATE_PAGE_DONE:
    case CupsPrintJob::State::STATE_SUSPENDED:
    case CupsPrintJob::State::STATE_RESUMED:
      title = l10n_util::GetStringFUTF16(
          IDS_PRINT_JOB_PRINTING_NOTIFICATION_TITLE,
          base::UTF8ToUTF16(print_job_->document_title()));
      break;
    case CupsPrintJob::State::STATE_DOCUMENT_DONE:
      title = l10n_util::GetStringFUTF16(
          IDS_PRINT_JOB_DONE_NOTIFICATION_TITLE,
          base::UTF8ToUTF16(print_job_->document_title()));
      break;
    case CupsPrintJob::State::STATE_CANCELLED:
    case CupsPrintJob::State::STATE_ERROR:
      title = l10n_util::GetStringFUTF16(
          IDS_PRINT_JOB_ERROR_NOTIFICATION_TITLE,
          base::UTF8ToUTF16(print_job_->document_title()));
      break;
    default:
      break;
  }
  notification_->set_title(title);
}

void CupsPrintJobNotification::UpdateNotificationIcon() {
  if (!print_job_)
    return;
  switch (print_job_->state()) {
    case CupsPrintJob::State::STATE_WAITING:
    case CupsPrintJob::State::STATE_STARTED:
    case CupsPrintJob::State::STATE_PAGE_DONE:
    case CupsPrintJob::State::STATE_SUSPENDED:
    case CupsPrintJob::State::STATE_RESUMED:
      notification_->set_accent_color(
          message_center::kSystemNotificationColorNormal);
      notification_->set_vector_small_image(ash::kNotificationPrintingIcon);
      break;
    case CupsPrintJob::State::STATE_DOCUMENT_DONE:
      notification_->set_accent_color(
          message_center::kSystemNotificationColorNormal);
      notification_->set_vector_small_image(ash::kNotificationPrintingDoneIcon);
      break;
    case CupsPrintJob::State::STATE_CANCELLED:
    case CupsPrintJob::State::STATE_ERROR:
      notification_->set_accent_color(
          message_center::kSystemNotificationColorWarning);
      notification_->set_vector_small_image(
          ash::kNotificationPrintingWarningIcon);
      break;
    case CupsPrintJob::State::STATE_NONE:
      break;
  }
}

void CupsPrintJobNotification::UpdateNotificationBodyMessage() {
  if (!print_job_)
    return;
  base::string16 message;
  if (print_job_->total_page_number() > 1) {
    message = l10n_util::GetStringFUTF16(
        IDS_PRINT_JOB_NOTIFICATION_MESSAGE,
        base::IntToString16(print_job_->total_page_number()),
        base::UTF8ToUTF16(print_job_->printer().display_name()));
  } else {
    message = l10n_util::GetStringFUTF16(
        IDS_PRINT_JOB_NOTIFICATION_SINGLE_PAGE_MESSAGE,
        base::UTF8ToUTF16(print_job_->printer().display_name()));
  }
  notification_->set_message(message);
}

void CupsPrintJobNotification::UpdateNotificationType() {
  if (!print_job_)
    return;
  switch (print_job_->state()) {
    case CupsPrintJob::State::STATE_WAITING:
    case CupsPrintJob::State::STATE_STARTED:
    case CupsPrintJob::State::STATE_PAGE_DONE:
    case CupsPrintJob::State::STATE_SUSPENDED:
    case CupsPrintJob::State::STATE_RESUMED:
      notification_->set_type(message_center::NOTIFICATION_TYPE_PROGRESS);
      notification_->set_progress(print_job_->printed_page_number() * 100 /
                                  print_job_->total_page_number());
      break;
    case CupsPrintJob::State::STATE_NONE:
    case CupsPrintJob::State::STATE_DOCUMENT_DONE:
    case CupsPrintJob::State::STATE_ERROR:
    case CupsPrintJob::State::STATE_CANCELLED:
      notification_->set_type(message_center::NOTIFICATION_TYPE_SIMPLE);
      break;
  }
}

void CupsPrintJobNotification::UpdateNotificationButtons() {
  std::vector<message_center::ButtonInfo> buttons;
  button_commands_ = GetButtonCommands();
  for (const auto& it : button_commands_) {
    message_center::ButtonInfo button_info =
        message_center::ButtonInfo(GetButtonLabel(it));
    button_info.icon = GetButtonIcon(it);
    buttons.push_back(button_info);
  }
  notification_->set_buttons(buttons);
}

std::vector<CupsPrintJobNotification::ButtonCommand>
CupsPrintJobNotification::GetButtonCommands() const {
  if (!print_job_)
    return {};
  std::vector<CupsPrintJobNotification::ButtonCommand> commands;
  switch (print_job_->state()) {
    case CupsPrintJob::State::STATE_WAITING:
      commands.push_back(ButtonCommand::CANCEL_PRINTING);
      break;
    case CupsPrintJob::State::STATE_STARTED:
    case CupsPrintJob::State::STATE_PAGE_DONE:
    case CupsPrintJob::State::STATE_RESUMED:
    case CupsPrintJob::State::STATE_SUSPENDED:
      // TODO(crbug.com/679927): Add PAUSE and RESUME buttons.
      commands.push_back(ButtonCommand::CANCEL_PRINTING);
      break;
    case CupsPrintJob::State::STATE_ERROR:
    case CupsPrintJob::State::STATE_CANCELLED:
      commands.push_back(ButtonCommand::GET_HELP);
      break;
    default:
      break;
  }
  return commands;
}

base::string16 CupsPrintJobNotification::GetButtonLabel(
    ButtonCommand button) const {
  switch (button) {
    case ButtonCommand::CANCEL_PRINTING:
      return l10n_util::GetStringUTF16(
          IDS_PRINT_JOB_NOTIFICATION_CANCEL_BUTTON);
    case ButtonCommand::PAUSE_PRINTING:
      return l10n_util::GetStringUTF16(IDS_PRINT_JOB_NOTIFICATION_PAUSE_BUTTON);
    case ButtonCommand::RESUME_PRINTING:
      return l10n_util::GetStringUTF16(
          IDS_PRINT_JOB_NOTIFICATION_RESUME_BUTTON);
    case ButtonCommand::GET_HELP:
      return l10n_util::GetStringUTF16(
          IDS_PRINT_JOB_NOTIFICATION_GET_HELP_BUTTON);
  }
  return base::string16();
}

gfx::Image CupsPrintJobNotification::GetButtonIcon(ButtonCommand button) const {
  ui::ResourceBundle& bundle = ui::ResourceBundle::GetSharedInstance();
  gfx::Image icon;
  switch (button) {
    case ButtonCommand::CANCEL_PRINTING:
      icon = bundle.GetImageNamed(IDR_PRINT_NOTIFICATION_CANCEL);
      break;
    case ButtonCommand::PAUSE_PRINTING:
      icon = bundle.GetImageNamed(IDR_PRINT_NOTIFICATION_PAUSE);
      break;
    case ButtonCommand::RESUME_PRINTING:
      icon = bundle.GetImageNamed(IDR_PRINT_NOTIFICATION_PLAY);
      break;
    case ButtonCommand::GET_HELP:
      icon = bundle.GetImageNamed(IDR_PRINT_NOTIFICATION_HELP);
      break;
  }
  return icon;
}

}  // namespace chromeos
