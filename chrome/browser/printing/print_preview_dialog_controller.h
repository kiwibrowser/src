// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_PRINTING_PRINT_PREVIEW_DIALOG_CONTROLLER_H_
#define CHROME_BROWSER_PRINTING_PRINT_PREVIEW_DIALOG_CONTROLLER_H_

#include <map>

#include "base/callback.h"
#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "components/sessions/core/session_id.h"
#include "content/public/browser/notification_observer.h"
#include "content/public/browser/notification_registrar.h"

class GURL;

namespace content {
struct LoadCommittedDetails;
class RenderProcessHost;
class WebContents;
}

namespace printing {

// For print preview, the WebContents that initiates the printing operation is
// the initiator, and the constrained dialog that shows the print preview is the
// print preview dialog.
// This class manages print preview dialog creation and destruction, and keeps
// track of the 1:1 relationship between initiator tabs and print preview
// dialogs.
class PrintPreviewDialogController
    : public base::RefCounted<PrintPreviewDialogController>,
      public content::NotificationObserver {
 public:
  PrintPreviewDialogController();

  static PrintPreviewDialogController* GetInstance();

  // Initiate print preview for |initiator|.
  // Call this instead of GetOrCreatePreviewDialog().
  static void PrintPreview(content::WebContents* initiator);

  // Returns true if |url| is a print preview url.
  static bool IsPrintPreviewURL(const GURL& url);

  // Get/Create the print preview dialog for |initiator|.
  // Exposed for unit tests.
  content::WebContents* GetOrCreatePreviewDialog(
      content::WebContents* initiator);

  // Returns the preview dialog for |contents|.
  // Returns |contents| if |contents| is a preview dialog.
  // Returns NULL if no preview dialog exists for |contents|.
  content::WebContents* GetPrintPreviewForContents(
      content::WebContents* contents) const;

  // Returns the initiator for |preview_dialog|.
  // Returns NULL if no initiator exists for |preview_dialog|.
  content::WebContents* GetInitiator(content::WebContents* preview_dialog);

  // Run |callback| on the dialog of each active print preview operation.
  void ForEachPreviewDialog(
      base::Callback<void(content::WebContents*)> callback);

  // content::NotificationObserver implementation.
  void Observe(int type,
               const content::NotificationSource& source,
               const content::NotificationDetails& details) override;

  // Erase the initiator info associated with |preview_dialog|.
  void EraseInitiatorInfo(content::WebContents* preview_dialog);

  bool is_creating_print_preview_dialog() const {
    return is_creating_print_preview_dialog_;
  }

 private:
  friend class base::RefCounted<PrintPreviewDialogController>;

  // 1:1 relationship between a print preview dialog and its initiator tab.
  // Key: Print preview dialog.
  // Value: Initiator.
  using PrintPreviewDialogMap =
      std::map<content::WebContents*, content::WebContents*>;

  ~PrintPreviewDialogController() override;

  // Handler for the RENDERER_PROCESS_CLOSED notification. This is observed when
  // the initiator renderer crashed.
  void OnRendererProcessClosed(content::RenderProcessHost* rph);

  // Handler for the WEB_CONTENTS_DESTROYED notification. This is observed when
  // either WebContents is closed.
  void OnWebContentsDestroyed(content::WebContents* contents);

  // Handler for the NAV_ENTRY_COMMITTED notification. This is observed when the
  // renderer is navigated to a different page.
  void OnNavEntryCommitted(content::WebContents* contents,
                           content::LoadCommittedDetails* details);

  // Creates a new print preview dialog.
  content::WebContents* CreatePrintPreviewDialog(
      content::WebContents* initiator);

  // Helper function to store the title of the initiator associated with
  // |preview_dialog| in |preview_dialog|'s PrintPreviewUI.
  void SaveInitiatorTitle(content::WebContents* preview_dialog);

  // Adds/Removes observers for notifications from |contents|.
  void AddObservers(content::WebContents* contents);
  void RemoveObservers(content::WebContents* contents);

  // Removes WebContents when they close/crash/navigate.
  void RemoveInitiator(content::WebContents* initiator);
  void RemovePreviewDialog(content::WebContents* preview_dialog);

  // Mapping between print preview dialog and the corresponding initiator.
  PrintPreviewDialogMap preview_dialog_map_;

  // A registrar for listening to notifications.
  content::NotificationRegistrar registrar_;

  // True if the controller is waiting for a new preview dialog via
  // content::NAVIGATION_TYPE_NEW_PAGE.
  bool waiting_for_new_preview_page_;

  // Whether the PrintPreviewDialogController is in the middle of creating a
  // print preview dialog.
  bool is_creating_print_preview_dialog_;

  // How many web contents (dialogs and initiators) are watching a given render
  // process host. Used to determine when a render process host's
  // NOTIFICATION_RENDERER_PROCESS_CLOSED notification should be removed from
  // the registrar.
  std::map<content::RenderProcessHost*, int> host_contents_count_map_;

  DISALLOW_COPY_AND_ASSIGN(PrintPreviewDialogController);
};

}  // namespace printing

#endif  // CHROME_BROWSER_PRINTING_PRINT_PREVIEW_DIALOG_CONTROLLER_H_
