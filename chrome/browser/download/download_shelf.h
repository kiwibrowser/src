// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_DOWNLOAD_DOWNLOAD_SHELF_H_
#define CHROME_BROWSER_DOWNLOAD_DOWNLOAD_SHELF_H_

#include <stdint.h>

#include "base/memory/weak_ptr.h"
#include "base/time/time.h"
#include "build/build_config.h"

class Browser;

namespace gfx {
class Canvas;
}

namespace content {
class DownloadManager;
}

namespace download {
class DownloadItem;
}

namespace ui {
class ThemeProvider;
}

// This is an abstract base class for platform specific download shelf
// implementations.
class DownloadShelf {
 public:
  // Reason for closing download shelf.
  enum CloseReason {
    // Closing the shelf automatically. E.g.: all remaining downloads in the
    // shelf have been opened, last download in shelf was removed, or the
    // browser is switching to full-screen mode.
    AUTOMATIC,

    // Closing shelf due to a user selection. E.g.: the user clicked on the
    // 'close' button on the download shelf, or the shelf is being closed as a
    // side-effect of the user opening the downloads page.
    USER_ACTION
  };

  // Download progress animations ----------------------------------------------

  enum {
    // Progress animation timer period, in milliseconds.
    kProgressRateMs = 30,

    // Size of the space used for the progress indicator.
    kProgressIndicatorSize = 25,

    // x/y offset for the file type icon.
    kFiletypeIconOffset = (kProgressIndicatorSize - 16) / 2
  };

  DownloadShelf();
  virtual ~DownloadShelf();

  // Paint the common download animation progress foreground and background,
  // clipping the foreground to 'percent' full. If percent is -1, then we don't
  // know the total size, so we just draw a rotating segment until we're done.
  // |progress_time| is only used for these unknown size downloads.
  static void PaintDownloadProgress(gfx::Canvas* canvas,
                                    const ui::ThemeProvider& theme_provider,
                                    const base::TimeDelta& progress_time,
                                    int percent);

  static void PaintDownloadComplete(gfx::Canvas* canvas,
                                    const ui::ThemeProvider& theme_provider,
                                    double animation_progress);

  static void PaintDownloadInterrupted(gfx::Canvas* canvas,
                                       const ui::ThemeProvider& theme_provider,
                                       double animation_progress);

  // A new download has started. Add it to our shelf and show the download
  // started animation.
  //
  // Some downloads are removed from the shelf on completion (See
  // DownloadItemModel::ShouldRemoveFromShelfWhenComplete()). These transient
  // downloads are added to the shelf after a delay. If the download completes
  // before the delay duration, it will not be added to the shelf at all.
  void AddDownload(download::DownloadItem* download);

  // The browser view needs to know when we are going away to properly return
  // the resize corner size to WebKit so that we don't draw on top of it.
  // This returns the showing state of our animation which is set to true at
  // the beginning Show and false at the beginning of a Hide.
  virtual bool IsShowing() const = 0;

  // Returns whether the download shelf is showing the close animation.
  virtual bool IsClosing() const = 0;

  // Opens the shelf.
  void Open();

  // Closes the shelf.
  void Close(CloseReason reason);

  // Hides the shelf. This closes the shelf if it is currently showing.
  void Hide();

  // Unhides the shelf. This will cause the shelf to be opened if it was open
  // when it was hidden, or was shown while it was hidden.
  void Unhide();

  virtual Browser* browser() const = 0;

  // Returns whether the download shelf is hidden.
  bool is_hidden() { return is_hidden_; }

 protected:
  virtual void DoAddDownload(download::DownloadItem* download) = 0;
  virtual void DoOpen() = 0;
  virtual void DoClose(CloseReason reason) = 0;
  virtual void DoHide() = 0;
  virtual void DoUnhide() = 0;

  // Time delay to wait before adding a transient download to the shelf.
  // Protected virtual for testing.
  virtual base::TimeDelta GetTransientDownloadShowDelay();

  // Returns the DownloadManager associated with this DownloadShelf. All
  // downloads that are shown on this shelf is expected to belong to this
  // DownloadManager. Protected virtual for testing.
  virtual content::DownloadManager* GetDownloadManager();

 private:
  // Show the download on the shelf immediately. Also displayes the download
  // started animation if necessary.
  void ShowDownload(download::DownloadItem* download);

  // Similar to ShowDownload() but refers to the download using an ID. This
  // download should belong to the DownloadManager returned by
  // GetDownloadManager().
  void ShowDownloadById(int32_t download_id);

  bool should_show_on_unhide_;
  bool is_hidden_;
  base::WeakPtrFactory<DownloadShelf> weak_ptr_factory_;
};

#endif  // CHROME_BROWSER_DOWNLOAD_DOWNLOAD_SHELF_H_
