// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_SESSIONS_SESSION_RESTORE_DELEGATE_H_
#define CHROME_BROWSER_SESSIONS_SESSION_RESTORE_DELEGATE_H_

#include <vector>

#include "base/macros.h"
#include "base/time/time.h"

namespace content {
class WebContents;
}

// SessionRestoreDelegate is responsible for creating the tab loader and the
// stats collector.
class SessionRestoreDelegate {
 public:
  class RestoredTab {
   public:
    RestoredTab(content::WebContents* contents,
                bool is_active,
                bool is_app,
                bool is_pinned);

    bool operator<(const RestoredTab& right) const;

    content::WebContents* contents() const { return contents_; }
    bool is_active() const { return is_active_; }
    bool is_app() const { return is_app_; }
    bool is_internal_page() const { return is_internal_page_; }
    bool is_pinned() const { return is_pinned_; }

   private:
    content::WebContents* contents_;
    bool is_active_;
    bool is_app_;            // Browser window is an app.
    bool is_internal_page_;  // Internal web UI page, like NTP or Settings.
    bool is_pinned_;
  };

  static void RestoreTabs(const std::vector<RestoredTab>& tabs,
                          const base::TimeTicks& restore_started);

 private:
  DISALLOW_IMPLICIT_CONSTRUCTORS(SessionRestoreDelegate);
};

#endif  // CHROME_BROWSER_SESSIONS_SESSION_RESTORE_DELEGATE_H_
