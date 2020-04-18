// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_TAB_CONTENTS_CORE_TAB_HELPER_H_
#define CHROME_BROWSER_UI_TAB_CONTENTS_CORE_TAB_HELPER_H_

#include <string>

#include "base/callback.h"
#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "base/time/time.h"
#include "chrome/common/chrome_render_frame.mojom.h"
#include "content/public/browser/web_contents_observer.h"
#include "content/public/browser/web_contents_user_data.h"
#include "ui/gfx/geometry/size.h"
#include "url/gurl.h"

class CoreTabHelperDelegate;

// Per-tab class to handle functionality that is core to the operation of tabs.
class CoreTabHelper : public content::WebContentsObserver,
                      public content::WebContentsUserData<CoreTabHelper> {
 public:
  ~CoreTabHelper() override;

  // Initial title assigned to NavigationEntries from Navigate.
  static base::string16 GetDefaultTitle();

  // Returns a human-readable description the tab's loading state.
  base::string16 GetStatusText() const;

  // Notification that tab closing has started.  This can be called multiple
  // times, subsequent calls are ignored.
  void OnCloseStarted();

  // Notification that tab closing was cancelled. This can happen when a user
  // cancels a window close via another tab's beforeunload dialog.
  void OnCloseCanceled();

  // Set the time during close when unload is started. Normally, this is set
  // after the beforeunload dialog. However, for a window close, it is set
  // after all the beforeunload dialogs have finished.
  void OnUnloadStarted();

  // Set the time during close when the tab is no longer visible.
  void OnUnloadDetachedStarted();

  void UpdateContentRestrictions(int content_restrictions);

  // Perform an image search for the image that triggered the context menu.  The
  // |src_url| is passed to the search request and is not used directly to fetch
  // the image resources.
  void SearchByImageInNewTab(content::RenderFrameHost* render_frame_host,
                             const GURL& src_url);

  CoreTabHelperDelegate* delegate() const { return delegate_; }
  void set_delegate(CoreTabHelperDelegate* d) { delegate_ = d; }

  void set_new_tab_start_time(const base::TimeTicks& time) {
    new_tab_start_time_ = time;
  }

  base::TimeTicks new_tab_start_time() const { return new_tab_start_time_; }
  int content_restrictions() const { return content_restrictions_; }

 private:
  explicit CoreTabHelper(content::WebContents* web_contents);
  friend class content::WebContentsUserData<CoreTabHelper>;

  static bool GetStatusTextForWebContents(base::string16* status_text,
                                          content::WebContents* source);

  // content::WebContentsObserver overrides:
  void DidStartLoading() override;
  void OnVisibilityChanged(content::Visibility visibility) override;
  void WebContentsDestroyed() override;
  void BeforeUnloadFired(const base::TimeTicks& proceed_time) override;
  void BeforeUnloadDialogCancelled() override;
  void NavigationEntriesDeleted() override;

  void DoSearchByImageInNewTab(
      chrome::mojom::ChromeRenderFrameAssociatedPtr chrome_render_frame,
      const GURL& src_url,
      const std::vector<uint8_t>& thumbnail_data,
      const gfx::Size& original_size);

  // Delegate for notifying our owner about stuff. Not owned by us.
  CoreTabHelperDelegate* delegate_;

  // The time when we started to create the new tab page.  This time is from
  // before we created this WebContents.
  base::TimeTicks new_tab_start_time_;

  // The time that we started to close this WebContents.
  base::TimeTicks close_start_time_;

  // The time when onbeforeunload ended.
  base::TimeTicks before_unload_end_time_;

  // The time when the tab was removed from view during close.
  base::TimeTicks unload_detached_start_time_;

  // Content restrictions, used to disable print/copy etc based on content's
  // (full-page plugins for now only) permissions.
  int content_restrictions_;

  base::WeakPtrFactory<CoreTabHelper> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(CoreTabHelper);
};

#endif  // CHROME_BROWSER_UI_TAB_CONTENTS_CORE_TAB_HELPER_H_
