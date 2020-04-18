// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_METRICS_TAB_REACTIVATION_TRACKER_H_
#define CHROME_BROWSER_METRICS_TAB_REACTIVATION_TRACKER_H_

#include <memory>
#include <unordered_map>

#include "base/macros.h"
#include "chrome/browser/ui/browser_tab_strip_tracker.h"
#include "chrome/browser/ui/tabs/tab_strip_model_observer.h"

namespace metrics {

// This class is used to track tab deactivation/reactivation cycle. A
// reactivation is defined as a tab that was hidden and then reshown.
//
// Note: A closing tab will not trigger a tab deactivation. Also worth noting
// that tab discarding disrupts the tracking and the discarded tab will be
// treated as a new tab.
class TabReactivationTracker : public TabStripModelObserver {
 public:
  class Delegate {
   public:
    virtual void OnTabDeactivated(content::WebContents* contents) = 0;
    virtual void OnTabReactivated(content::WebContents* contents) = 0;
  };
  explicit TabReactivationTracker(Delegate* delegate);
  ~TabReactivationTracker() override;

  // TabStripModelObserver:
  void TabInsertedAt(TabStripModel* tab_strip_model,
                     content::WebContents* contents,
                     int index,
                     bool foreground) override;
  void TabClosingAt(TabStripModel* tab_strip_model,
                    content::WebContents* contents,
                    int index) override;
  void ActiveTabChanged(content::WebContents* old_contents,
                        content::WebContents* new_contents,
                        int index,
                        int reason) override;

  void NotifyTabDeactivating(content::WebContents* contents);
  void NotifyTabReactivating(content::WebContents* contents);

 private:
  class WebContentsHelper;

  // Returns the helper for the |contents|, creating it if it doesn't exist.
  WebContentsHelper* GetHelper(content::WebContents* contents);

  // Called by the helper when |contents| is being destroyed.
  void OnWebContentsDestroyed(content::WebContents* contents);

  // The delegate must outlive this class.
  Delegate* delegate_;

  // A helper instance is managed per WebContents.
  std::unordered_map<content::WebContents*, std::unique_ptr<WebContentsHelper>>
      helper_map_;

  BrowserTabStripTracker browser_tab_strip_tracker_;

  DISALLOW_COPY_AND_ASSIGN(TabReactivationTracker);
};

}  // namespace metrics

#endif  // CHROME_BROWSER_METRICS_TAB_REACTIVATION_TRACKER_H_
