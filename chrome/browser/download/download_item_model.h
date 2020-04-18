// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_DOWNLOAD_DOWNLOAD_ITEM_MODEL_H_
#define CHROME_BROWSER_DOWNLOAD_DOWNLOAD_ITEM_MODEL_H_

#include <stdint.h>

#include <string>

#include "base/compiler_specific.h"
#include "base/macros.h"
#include "base/strings/string16.h"
#include "chrome/browser/download/download_target_info.h"
#include "chrome/common/safe_browsing/download_file_types.pb.h"

namespace download {
class DownloadItem;
}

namespace gfx {
class FontList;
}

// This class is an abstraction for common UI tasks and properties associated
// with a DownloadItem.
//
// It is intended to be used as a thin wrapper around a |DownloadItem*|. As
// such, the caller is expected to ensure that the |download| passed into the
// constructor outlives this |DownloadItemModel|. In addition, multiple
// DownloadItemModel objects could be wrapping the same DownloadItem.
class DownloadItemModel {
 public:
  // Constructs a DownloadItemModel. The caller must ensure that |download|
  // outlives this object.
  explicit DownloadItemModel(download::DownloadItem* download);
  ~DownloadItemModel();

  // Returns a long descriptive string for a download that's in the INTERRUPTED
  // state. For other downloads, the returned string will be empty.
  base::string16 GetInterruptReasonText() const;

  // Returns a short one-line status string for the download.
  base::string16 GetStatusText() const;

  // Returns the localized status text for an in-progress download. This
  // is the progress status used in the WebUI interface.
  base::string16 GetTabProgressStatusText() const;

  // Returns a string suitable for use as a tooltip. For a regular download, the
  // tooltip is the filename. For an interrupted download, the string states the
  // filename and a short description of the reason for interruption. For
  // example:
  //    Report.pdf
  //    Network disconnected
  // |font_list| and |max_width| are used to elide the filename and/or interrupt
  // reason as necessary to keep the width of the tooltip text under
  // |max_width|. The tooltip will be at most 2 lines.
  base::string16 GetTooltipText(const gfx::FontList& font_list, int max_width) const;

  // Get the warning text to display for a dangerous download. The |base_width|
  // is the maximum width of an embedded filename (if there is one). The metrics
  // for the filename will be based on |font_list|. Should only be called if
  // IsDangerous() is true.
  base::string16 GetWarningText(const gfx::FontList& font_list, int base_width) const;

  // Get the caption text for a button for confirming a dangerous download
  // warning.
  base::string16 GetWarningConfirmButtonText() const;

  // Get the number of bytes that has completed so far. Virtual for testing.
  int64_t GetCompletedBytes() const;

  // Get the total number of bytes for this download. Should return 0 if the
  // total size of the download is not known. Virual for testing.
  int64_t GetTotalBytes() const;

  // Rough percent complete. Returns -1 if the progress is unknown.
  int PercentComplete() const;

  // Is this considered a dangerous download?
  bool IsDangerous() const;

  // Is this considered a malicious download? Implies IsDangerous().
  bool MightBeMalicious() const;

  // Is this considered a malicious download with very high confidence?
  // Implies IsDangerous() and MightBeMalicious().
  bool IsMalicious() const;

  // Does this download have a MIME type (either explicit or inferred from its
  // extension) suggesting that it is a supported image type?
  bool HasSupportedImageMimeType() const;

  // Is safe browsing download feedback feature available for this download?
  bool ShouldAllowDownloadFeedback() const;

  // Returns |true| if this download is expected to complete successfully and
  // thereafter be removed from the shelf.  Downloads that are opened
  // automatically or are temporary will be removed from the shelf on successful
  // completion.
  //
  // Returns |false| if the download is not expected to complete (interrupted,
  // cancelled, dangerous, malicious), or won't be removed on completion.
  //
  // Since the expectation of successful completion may change, the return value
  // of this function will change over the course of a download.
  bool ShouldRemoveFromShelfWhenComplete() const;

  // Returns |true| if the download started animation (big download arrow
  // animates down towards the shelf) should be displayed for this download.
  // Downloads that were initiated via "Save As" or are extension installs don't
  // show the animation.
  bool ShouldShowDownloadStartedAnimation() const;

  // Returns |true| if this download should be displayed in the downloads shelf.
  bool ShouldShowInShelf() const;

  // Change whether the download should be displayed on the downloads
  // shelf. Setting this is only effective if the download hasn't already been
  // displayed in the shelf.
  void SetShouldShowInShelf(bool should_show);

  // Returns |true| if the UI should be notified when the download is ready to
  // be presented in the UI. Note that this is indpendent of ShouldShowInShelf()
  // since there might be actions other than showing in the shelf that the UI
  // must perform.
  bool ShouldNotifyUI() const;

  // Returns |true| if the UI has been notified about this download. By default,
  // this value is |false| and should be changed explicitly using
  // SetWasUINotified().
  bool WasUINotified() const;

  // Change what's returned by WasUINotified().
  void SetWasUINotified(bool should_notify);

  // Returns |true| if opening in the browser is preferred for this download. If
  // |false|, the download should be opened with the system default application.
  bool ShouldPreferOpeningInBrowser() const;

  // Change what's returned by ShouldPreferOpeningInBrowser to |preference|.
  void SetShouldPreferOpeningInBrowser(bool preference);

  // Return the danger level determined during download target determination.
  // The value returned here is independent of the danger level as determined by
  // the Safe Browsing.
  safe_browsing::DownloadFileType::DangerLevel GetDangerLevel() const;

  // Change what's returned by GetDangerLevel().
  void SetDangerLevel(
      safe_browsing::DownloadFileType::DangerLevel danger_level);

  // Open the download using the platform handler for the download. The behavior
  // of this method will be different from DownloadItem::OpenDownload() if
  // ShouldPreferOpeningInBrowser().
  void OpenUsingPlatformHandler();

  // Whether the download was removed and this is currently being undone.
  bool IsBeingRevived() const;

  // Set whether the download is being revived.
  void SetIsBeingRevived(bool is_being_revived);

  download::DownloadItem* download() { return download_; }

  // Returns a string representations of the current download progress sizes. If
  // the total size of the download is known, this string looks like: "100/200
  // MB" where the numerator is the transferred size and the denominator is the
  // total size. If the total isn't known, returns the transferred size as a
  // string (e.g.: "100 MB").
  base::string16 GetProgressSizesString() const;

 private:
  // Returns a string indicating the status of an in-progress download.
  base::string16 GetInProgressStatusString() const;

  // The DownloadItem that this model represents. Note that DownloadItemModel
  // itself shouldn't maintain any state since there can be more than one
  // DownloadItemModel in use with the same DownloadItem.
  download::DownloadItem* download_;

  DISALLOW_COPY_AND_ASSIGN(DownloadItemModel);
};

#endif  // CHROME_BROWSER_DOWNLOAD_DOWNLOAD_ITEM_MODEL_H_
