// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_WEBUI_MD_DOWNLOADS_MD_DOWNLOADS_DOM_HANDLER_H_
#define CHROME_BROWSER_UI_WEBUI_MD_DOWNLOADS_MD_DOWNLOADS_DOM_HANDLER_H_

#include <stdint.h>

#include <set>
#include <vector>

#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "chrome/browser/download/download_danger_prompt.h"
#include "chrome/browser/ui/webui/md_downloads/downloads_list_tracker.h"
#include "content/public/browser/web_contents_observer.h"
#include "content/public/browser/web_ui_message_handler.h"

namespace base {
class ListValue;
}

namespace content {
class DownloadManager;
class WebContents;
class WebUI;
}

namespace download {
class DownloadItem;
}

// The handler for Javascript messages related to the "downloads" view,
// also observes changes to the download manager.
class MdDownloadsDOMHandler : public content::WebContentsObserver,
                              public content::WebUIMessageHandler {
 public:
  MdDownloadsDOMHandler(content::DownloadManager* download_manager,
                        content::WebUI* web_ui);
  ~MdDownloadsDOMHandler() override;

  // WebUIMessageHandler implementation.
  void RegisterMessages() override;
  void OnJavascriptDisallowed() override;

  // WebContentsObserver implementation.
  void RenderProcessGone(base::TerminationStatus status) override;

  // Callback for the "getDownloads" message.
  void HandleGetDownloads(const base::ListValue* args);

  // Callback for the "openFile" message - opens the file in the shell.
  void HandleOpenFile(const base::ListValue* args);

  // Callback for the "drag" message - initiates a file object drag.
  void HandleDrag(const base::ListValue* args);

  // Callback for the "saveDangerous" message - specifies that the user
  // wishes to save a dangerous file.
  void HandleSaveDangerous(const base::ListValue* args);

  // Callback for the "discardDangerous" message - specifies that the user
  // wishes to discard (remove) a dangerous file.
  void HandleDiscardDangerous(const base::ListValue* args);

  // Callback for the "show" message - shows the file in explorer.
  void HandleShow(const base::ListValue* args);

  // Callback for the "pause" message - pauses the file download.
  void HandlePause(const base::ListValue* args);

  // Callback for the "resume" message - resumes the file download.
  void HandleResume(const base::ListValue* args);

  // Callback for the "remove" message - removes the file download from shelf
  // and list.
  void HandleRemove(const base::ListValue* args);

  // Callback for the "undo" message. Currently only undoes removals.
  void HandleUndo(const base::ListValue* args);

  // Callback for the "cancel" message - cancels the download.
  void HandleCancel(const base::ListValue* args);

  // Callback for the "clearAll" message - clears all the downloads.
  void HandleClearAll(const base::ListValue* args);

  // Callback for the "openDownloadsFolder" message - opens the downloads
  // folder.
  void HandleOpenDownloadsFolder(const base::ListValue* args);

 protected:
  // These methods are for mocking so that most of this class does not actually
  // depend on WebUI. The other methods that depend on WebUI are
  // RegisterMessages() and HandleDrag().
  virtual content::WebContents* GetWebUIWebContents();

  // Actually remove downloads with an ID in |removals_|. This cannot be undone.
  void FinalizeRemovals();

  using DownloadVector = std::vector<download::DownloadItem*>;

  // Remove all downloads in |to_remove|. Safe downloads can be revived,
  // dangerous ones are immediately removed. Protected for testing.
  void RemoveDownloads(const DownloadVector& to_remove);

 private:
  using IdSet = std::set<uint32_t>;

  // Convenience method to call |main_notifier_->GetManager()| while
  // null-checking |main_notifier_|.
  content::DownloadManager* GetMainNotifierManager() const;

  // Convenience method to call |original_notifier_->GetManager()| while
  // null-checking |original_notifier_|.
  content::DownloadManager* GetOriginalNotifierManager() const;

  // Displays a native prompt asking the user for confirmation after accepting
  // the dangerous download specified by |dangerous|. The function returns
  // immediately, and will invoke DangerPromptAccepted() asynchronously if the
  // user accepts the dangerous download. The native prompt will observe
  // |dangerous| until either the dialog is dismissed or |dangerous| is no
  // longer an in-progress dangerous download.
  virtual void ShowDangerPrompt(download::DownloadItem* dangerous);

  // Conveys danger acceptance from the DownloadDangerPrompt to the
  // DownloadItem.
  void DangerPromptDone(int download_id, DownloadDangerPrompt::Action action);

  // Returns true if the records of any downloaded items are allowed (and able)
  // to be deleted.
  bool IsDeletingHistoryAllowed();

  // Returns the download that is referred to in a given value.
  download::DownloadItem* GetDownloadByValue(const base::ListValue* args);

  // Returns the download with |id| or NULL if it doesn't exist.
  download::DownloadItem* GetDownloadById(uint32_t id);

  // Removes the download specified by an ID from JavaScript in |args|.
  void RemoveDownloadInArgs(const base::ListValue* args);

  // Checks whether a download's file was removed from its original location.
  void CheckForRemovedFiles();

  DownloadsListTracker list_tracker_;

  // IDs of downloads to remove when this handler gets deleted.
  std::vector<IdSet> removals_;

  // Whether the render process has gone.
  bool render_process_gone_ = false;

  base::WeakPtrFactory<MdDownloadsDOMHandler> weak_ptr_factory_{this};

  DISALLOW_COPY_AND_ASSIGN(MdDownloadsDOMHandler);
};

#endif  // CHROME_BROWSER_UI_WEBUI_MD_DOWNLOADS_MD_DOWNLOADS_DOM_HANDLER_H_
