// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/cocoa/download/download_item_mac.h"

#include "base/bind.h"
#include "base/callback.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/download/download_item_model.h"
#import "chrome/browser/ui/cocoa/download/download_item_controller.h"
#include "components/download/public/common/download_item.h"
#include "ui/gfx/image/image.h"

using download::DownloadItem;

// DownloadItemMac -------------------------------------------------------------

DownloadItemMac::DownloadItemMac(DownloadItem* download,
                                 DownloadItemController* controller)
    : download_model_(download),
      item_controller_(controller) {
  download_model_.download()->AddObserver(this);
}

DownloadItemMac::~DownloadItemMac() {
  download_model_.download()->RemoveObserver(this);
}

void DownloadItemMac::OnDownloadUpdated(DownloadItem* download) {
  DCHECK_EQ(download, download_model_.download());

  if (!download_model_.ShouldShowInShelf()) {
    [item_controller_ remove];  // We're deleted now!
    return;
  }

  if ([item_controller_ isDangerousMode] && !download_model_.IsDangerous()) {
    // We have been approved.
    [item_controller_ clearDangerousMode];
  }

  if (download->GetTargetFilePath() != lastFilePath_) {
    LoadIcon();
    lastFilePath_ = download->GetTargetFilePath();

    [item_controller_ updateToolTip];
  }

  switch (download->GetState()) {
    case DownloadItem::COMPLETE:
      if (download_model_.ShouldRemoveFromShelfWhenComplete()) {
        [item_controller_ remove];  // We're deleted now!
        return;
      }
      FALLTHROUGH;
    case DownloadItem::IN_PROGRESS:
    case DownloadItem::CANCELLED:
      [item_controller_ setStateFromDownload:&download_model_];
      break;
    case DownloadItem::INTERRUPTED:
      [item_controller_ updateToolTip];
      [item_controller_ setStateFromDownload:&download_model_];
      break;
    default:
      NOTREACHED();
  }
}

void DownloadItemMac::OnDownloadDestroyed(DownloadItem* download) {
  [item_controller_ remove];  // We're deleted now!
}

void DownloadItemMac::OnDownloadOpened(DownloadItem* download) {
  DCHECK_EQ(download, download_model_.download());
  [item_controller_ downloadWasOpened];
}

void DownloadItemMac::LoadIcon() {
  IconManager* icon_manager = g_browser_process->icon_manager();
  if (!icon_manager)
    return;

  // We may already have this particular image cached.
  base::FilePath file = download_model_.download()->GetTargetFilePath();
  gfx::Image* icon = icon_manager->LookupIconFromFilepath(
      file, IconLoader::ALL);
  if (icon) {
    [item_controller_ setIcon:icon->ToNSImage()];
    return;
  }

  // The icon isn't cached, load it asynchronously.
  icon_manager->LoadIcon(file,
                         IconLoader::ALL,
                         base::Bind(&DownloadItemMac::OnExtractIconComplete,
                                    base::Unretained(this)),
                         &cancelable_task_tracker_);
}

void DownloadItemMac::OnExtractIconComplete(gfx::Image* icon) {
  if (!icon)
    return;
  [item_controller_ setIcon:icon->ToNSImage()];
}
