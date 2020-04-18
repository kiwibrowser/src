// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/download/download_shelf_context_menu.h"

#include "build/build_config.h"
#include "chrome/browser/download/download_item_model.h"
#include "chrome/grit/generated_resources.h"
#include "content/public/common/content_features.h"
#include "extensions/common/extension.h"
#include "ui/base/l10n/l10n_util.h"

#if defined(OS_WIN)
#include "chrome/browser/ui/pdf/adobe_reader_info_win.h"
#endif

using download::DownloadItem;

bool DownloadShelfContextMenu::WantsContextMenu(
    const DownloadItemModel& download_model) {
  return !download_model.IsDangerous() || download_model.MightBeMalicious();
}

DownloadShelfContextMenu::~DownloadShelfContextMenu() {
  DetachFromDownloadItem();
}

DownloadShelfContextMenu::DownloadShelfContextMenu(DownloadItem* download_item)
    : download_item_(download_item),
      download_commands_(new DownloadCommands(download_item)) {
  DCHECK(download_item_);
  download_item_->AddObserver(this);
}

ui::SimpleMenuModel* DownloadShelfContextMenu::GetMenuModel() {
  ui::SimpleMenuModel* model = NULL;

  if (!download_item_)
    return NULL;

  DownloadItemModel download_model(download_item_);

  DCHECK(WantsContextMenu(download_model));

  if (download_model.IsMalicious())
    model = GetMaliciousMenuModel();
  else if (download_model.MightBeMalicious())
    model = GetMaybeMaliciousMenuModel();
  else if (download_item_->GetState() == DownloadItem::COMPLETE)
    model = GetFinishedMenuModel();
  else if (download_item_->GetState() == DownloadItem::INTERRUPTED)
    model = GetInterruptedMenuModel();
  else if (download_item_->IsPaused())
    model = GetInProgressPausedMenuModel();
  else
    model = GetInProgressMenuModel();
  return model;
}

bool DownloadShelfContextMenu::IsCommandIdEnabled(int command_id) const {
  if (!download_commands_)
    return false;

  return download_commands_->IsCommandEnabled(
      static_cast<DownloadCommands::Command>(command_id));
}

bool DownloadShelfContextMenu::IsCommandIdChecked(int command_id) const {
  if (!download_commands_)
    return false;

  return download_commands_->IsCommandChecked(
      static_cast<DownloadCommands::Command>(command_id));
}

bool DownloadShelfContextMenu::IsCommandIdVisible(int command_id) const {
  if (!download_commands_)
    return false;

  return download_commands_->IsCommandVisible(
      static_cast<DownloadCommands::Command>(command_id));
}

void DownloadShelfContextMenu::ExecuteCommand(int command_id, int event_flags) {
  if (!download_commands_)
    return;

  download_commands_->ExecuteCommand(
      static_cast<DownloadCommands::Command>(command_id));
}

bool DownloadShelfContextMenu::IsItemForCommandIdDynamic(int command_id) const {
  return false;
}

base::string16 DownloadShelfContextMenu::GetLabelForCommandId(
    int command_id) const {
  int id = -1;

  switch (static_cast<DownloadCommands::Command>(command_id)) {
    case DownloadCommands::OPEN_WHEN_COMPLETE:
      if (download_item_ && !download_item_->IsDone())
        id = IDS_DOWNLOAD_MENU_OPEN_WHEN_COMPLETE;
      else
        id = IDS_DOWNLOAD_MENU_OPEN;
      break;
    case DownloadCommands::PAUSE:
      id = IDS_DOWNLOAD_MENU_PAUSE_ITEM;
      break;
    case DownloadCommands::RESUME:
      id = IDS_DOWNLOAD_MENU_RESUME_ITEM;
      break;
    case DownloadCommands::SHOW_IN_FOLDER:
      id = IDS_DOWNLOAD_MENU_SHOW;
      break;
    case DownloadCommands::DISCARD:
      id = IDS_DOWNLOAD_MENU_DISCARD;
      break;
    case DownloadCommands::KEEP:
      id = IDS_DOWNLOAD_MENU_KEEP;
      break;
    case DownloadCommands::ALWAYS_OPEN_TYPE: {
      if (download_commands_) {
        bool can_open_pdf_in_system_viewer =
            download_commands_->CanOpenPdfInSystemViewer();
#if defined(OS_WIN)
        if (can_open_pdf_in_system_viewer) {
          id = IsAdobeReaderDefaultPDFViewer()
                   ? IDS_DOWNLOAD_MENU_ALWAYS_OPEN_PDF_IN_READER
                   : IDS_DOWNLOAD_MENU_PLATFORM_OPEN_ALWAYS;
          break;
        }
#elif defined(OS_MACOSX) || defined(OS_LINUX)
        if (can_open_pdf_in_system_viewer) {
          id = IDS_DOWNLOAD_MENU_PLATFORM_OPEN_ALWAYS;
          break;
        }
#endif
      }
      id = IDS_DOWNLOAD_MENU_ALWAYS_OPEN_TYPE;
      break;
    }
    case DownloadCommands::PLATFORM_OPEN:
      id = IDS_DOWNLOAD_MENU_PLATFORM_OPEN;
      break;
    case DownloadCommands::CANCEL:
      id = IDS_DOWNLOAD_MENU_CANCEL;
      break;
    case DownloadCommands::LEARN_MORE_SCANNING:
      id = IDS_DOWNLOAD_MENU_LEARN_MORE_SCANNING;
      break;
    case DownloadCommands::LEARN_MORE_INTERRUPTED:
      id = IDS_DOWNLOAD_MENU_LEARN_MORE_INTERRUPTED;
      break;
    case DownloadCommands::COPY_TO_CLIPBOARD:
    case DownloadCommands::ANNOTATE:
      // These commands are implemented only for the Download notification.
      NOTREACHED();
      break;
  }
  CHECK(id != -1);
  return l10n_util::GetStringUTF16(id);
}

void DownloadShelfContextMenu::DetachFromDownloadItem() {
  if (!download_item_)
    return;

  download_commands_.reset();
  download_item_->RemoveObserver(this);
  download_item_ = NULL;
}

void DownloadShelfContextMenu::OnDownloadDestroyed(DownloadItem* download) {
  DCHECK(download_item_ == download);
  DetachFromDownloadItem();
}

ui::SimpleMenuModel* DownloadShelfContextMenu::GetInProgressMenuModel() {
  if (in_progress_download_menu_model_)
    return in_progress_download_menu_model_.get();

  in_progress_download_menu_model_.reset(new ui::SimpleMenuModel(this));

  in_progress_download_menu_model_->AddCheckItem(
      DownloadCommands::OPEN_WHEN_COMPLETE,
      GetLabelForCommandId(DownloadCommands::OPEN_WHEN_COMPLETE));
  in_progress_download_menu_model_->AddCheckItem(
      DownloadCommands::ALWAYS_OPEN_TYPE,
      GetLabelForCommandId(DownloadCommands::ALWAYS_OPEN_TYPE));
  in_progress_download_menu_model_->AddSeparator(ui::NORMAL_SEPARATOR);
  in_progress_download_menu_model_->AddItem(
      DownloadCommands::PAUSE, GetLabelForCommandId(DownloadCommands::PAUSE));
  in_progress_download_menu_model_->AddItem(
      DownloadCommands::SHOW_IN_FOLDER,
      GetLabelForCommandId(DownloadCommands::SHOW_IN_FOLDER));
  in_progress_download_menu_model_->AddSeparator(ui::NORMAL_SEPARATOR);
  in_progress_download_menu_model_->AddItem(
      DownloadCommands::CANCEL, GetLabelForCommandId(DownloadCommands::CANCEL));

  return in_progress_download_menu_model_.get();
}

ui::SimpleMenuModel* DownloadShelfContextMenu::GetInProgressPausedMenuModel() {
  if (in_progress_download_paused_menu_model_)
    return in_progress_download_paused_menu_model_.get();

  in_progress_download_paused_menu_model_.reset(new ui::SimpleMenuModel(this));

  in_progress_download_paused_menu_model_->AddCheckItem(
      DownloadCommands::OPEN_WHEN_COMPLETE,
      GetLabelForCommandId(DownloadCommands::OPEN_WHEN_COMPLETE));
  in_progress_download_paused_menu_model_->AddCheckItem(
      DownloadCommands::ALWAYS_OPEN_TYPE,
      GetLabelForCommandId(DownloadCommands::ALWAYS_OPEN_TYPE));
  in_progress_download_paused_menu_model_->AddSeparator(ui::NORMAL_SEPARATOR);
  in_progress_download_paused_menu_model_->AddItem(
      DownloadCommands::RESUME, GetLabelForCommandId(DownloadCommands::RESUME));
  in_progress_download_paused_menu_model_->AddItem(
      DownloadCommands::SHOW_IN_FOLDER,
      GetLabelForCommandId(DownloadCommands::SHOW_IN_FOLDER));
  in_progress_download_paused_menu_model_->AddSeparator(ui::NORMAL_SEPARATOR);
  in_progress_download_paused_menu_model_->AddItem(
      DownloadCommands::CANCEL, GetLabelForCommandId(DownloadCommands::CANCEL));

  return in_progress_download_paused_menu_model_.get();
}

ui::SimpleMenuModel* DownloadShelfContextMenu::GetFinishedMenuModel() {
  if (finished_download_menu_model_)
    return finished_download_menu_model_.get();

  finished_download_menu_model_.reset(new ui::SimpleMenuModel(this));

  finished_download_menu_model_->AddItem(
      DownloadCommands::OPEN_WHEN_COMPLETE,
      GetLabelForCommandId(DownloadCommands::OPEN_WHEN_COMPLETE));
  finished_download_menu_model_->AddCheckItem(
      DownloadCommands::ALWAYS_OPEN_TYPE,
      GetLabelForCommandId(DownloadCommands::ALWAYS_OPEN_TYPE));
  finished_download_menu_model_->AddItem(
      DownloadCommands::PLATFORM_OPEN,
      GetLabelForCommandId(DownloadCommands::PLATFORM_OPEN));
  finished_download_menu_model_->AddSeparator(ui::NORMAL_SEPARATOR);
  finished_download_menu_model_->AddItem(
      DownloadCommands::SHOW_IN_FOLDER,
      GetLabelForCommandId(DownloadCommands::SHOW_IN_FOLDER));
  finished_download_menu_model_->AddSeparator(ui::NORMAL_SEPARATOR);
  finished_download_menu_model_->AddItem(
      DownloadCommands::CANCEL, GetLabelForCommandId(DownloadCommands::CANCEL));

  return finished_download_menu_model_.get();
}

ui::SimpleMenuModel* DownloadShelfContextMenu::GetInterruptedMenuModel() {
  if (interrupted_download_menu_model_)
    return interrupted_download_menu_model_.get();

  interrupted_download_menu_model_.reset(new ui::SimpleMenuModel(this));

  interrupted_download_menu_model_->AddItem(
      DownloadCommands::RESUME, GetLabelForCommandId(DownloadCommands::RESUME));
#if defined(OS_WIN)
  // The Help Center article is currently Windows specific.
  // TODO(asanka): Enable this for other platforms when the article is expanded
  // for other platforms.
  interrupted_download_menu_model_->AddItem(
      DownloadCommands::LEARN_MORE_INTERRUPTED,
      GetLabelForCommandId(DownloadCommands::LEARN_MORE_INTERRUPTED));
#endif
  interrupted_download_menu_model_->AddSeparator(ui::NORMAL_SEPARATOR);
  interrupted_download_menu_model_->AddItem(
      DownloadCommands::CANCEL, GetLabelForCommandId(DownloadCommands::CANCEL));

  return interrupted_download_menu_model_.get();
}

ui::SimpleMenuModel* DownloadShelfContextMenu::GetMaybeMaliciousMenuModel() {
  if (maybe_malicious_download_menu_model_)
    return maybe_malicious_download_menu_model_.get();

  maybe_malicious_download_menu_model_.reset(new ui::SimpleMenuModel(this));

  maybe_malicious_download_menu_model_->AddItem(
      DownloadCommands::KEEP, GetLabelForCommandId(DownloadCommands::KEEP));
  maybe_malicious_download_menu_model_->AddSeparator(ui::NORMAL_SEPARATOR);
  maybe_malicious_download_menu_model_->AddItem(
      DownloadCommands::LEARN_MORE_SCANNING,
      GetLabelForCommandId(DownloadCommands::LEARN_MORE_SCANNING));
  return maybe_malicious_download_menu_model_.get();
}

ui::SimpleMenuModel* DownloadShelfContextMenu::GetMaliciousMenuModel() {
  if (malicious_download_menu_model_)
    return malicious_download_menu_model_.get();

  malicious_download_menu_model_.reset(new ui::SimpleMenuModel(this));
  malicious_download_menu_model_->AddItem(
      DownloadCommands::LEARN_MORE_SCANNING,
      GetLabelForCommandId(DownloadCommands::LEARN_MORE_SCANNING));

  return malicious_download_menu_model_.get();
}
