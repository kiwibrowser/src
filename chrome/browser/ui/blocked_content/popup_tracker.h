// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_BLOCKED_CONTENT_POPUP_TRACKER_H_
#define CHROME_BROWSER_UI_BLOCKED_CONTENT_POPUP_TRACKER_H_

#include "base/macros.h"
#include "base/optional.h"
#include "base/time/time.h"
#include "chrome/browser/ui/blocked_content/scoped_visibility_tracker.h"
#include "content/public/browser/web_contents_observer.h"
#include "content/public/browser/web_contents_user_data.h"
#include "services/metrics/public/cpp/ukm_source_id.h"

namespace content {
class WebContents;
}

class ScopedVisibilityTracker;

// This class tracks new popups, and is used to log metrics on the visibility
// time of the first document in the popup.
// TODO(csharrison): Consider adding more metrics like total visibility for the
// lifetime of the WebContents.
class PopupTracker : public content::WebContentsObserver,
                     public content::WebContentsUserData<PopupTracker> {
 public:
  static void CreateForWebContents(content::WebContents* contents,
                                   content::WebContents* opener);
  ~PopupTracker() override;

 private:
  friend class content::WebContentsUserData<PopupTracker>;

  PopupTracker(content::WebContents* contents, content::WebContents* opener);

  // content::WebContentsObserver:
  void WebContentsDestroyed() override;
  void DidFinishNavigation(
      content::NavigationHandle* navigation_handle) override;
  void OnVisibilityChanged(content::Visibility visibility) override;

  // Will be unset until the first navigation commits. Will be set to the total
  // time the contents was visible at commit time.
  base::Optional<base::TimeDelta> first_load_visible_time_start_;
  // Will be unset until the second navigation commits. Is the total time the
  // contents is visible while the first document is loading (after commit).
  base::Optional<base::TimeDelta> first_load_visible_time_;

  ScopedVisibilityTracker visibility_tracker_;

  // The id of the web contents that created the popup at the time of creation.
  // SourceIds are permanent so it's okay to use at any point so long as it's
  // not invalid.
  const ukm::SourceId opener_source_id_;
  DISALLOW_COPY_AND_ASSIGN(PopupTracker);
};

#endif  // CHROME_BROWSER_UI_BLOCKED_CONTENT_POPUP_TRACKER_H_
