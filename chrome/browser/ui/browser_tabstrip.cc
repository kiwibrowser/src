// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/browser_tabstrip.h"

#include "base/command_line.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_navigator.h"
#include "chrome/browser/ui/browser_navigator_params.h"
#include "chrome/browser/ui/tab_contents/core_tab_helper.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/common/chrome_switches.h"
#include "chrome/common/url_constants.h"
#include "content/public/browser/navigation_entry.h"
#include "content/public/browser/render_view_host.h"
#include "content/public/browser/web_contents.h"

namespace chrome {

void AddTabAt(Browser* browser, const GURL& url, int idx, bool foreground) {
  // Time new tab page creation time.  We keep track of the timing data in
  // WebContents, but we want to include the time it takes to create the
  // WebContents object too.
  base::TimeTicks new_tab_start_time = base::TimeTicks::Now();
  NavigateParams params(browser,
                        url.is_empty() ? GURL(chrome::kChromeUINewTabURL) : url,
                        ui::PAGE_TRANSITION_TYPED);
  params.disposition = foreground ? WindowOpenDisposition::NEW_FOREGROUND_TAB
                                  : WindowOpenDisposition::NEW_BACKGROUND_TAB;
  params.tabstrip_index = idx;
  Navigate(&params);
  CoreTabHelper* core_tab_helper =
      CoreTabHelper::FromWebContents(params.navigated_or_inserted_contents);
  core_tab_helper->set_new_tab_start_time(new_tab_start_time);
}

content::WebContents* AddSelectedTabWithURL(
    Browser* browser,
    const GURL& url,
    ui::PageTransition transition) {
  NavigateParams params(browser, url, transition);
  params.disposition = WindowOpenDisposition::NEW_FOREGROUND_TAB;
  Navigate(&params);
  return params.navigated_or_inserted_contents;
}

void AddWebContents(Browser* browser,
                    content::WebContents* source_contents,
                    std::unique_ptr<content::WebContents> new_contents,
                    WindowOpenDisposition disposition,
                    const gfx::Rect& initial_rect) {
  // No code for this yet.
  DCHECK(disposition != WindowOpenDisposition::SAVE_TO_DISK);
  // Can't create a new contents for the current tab - invalid case.
  DCHECK(disposition != WindowOpenDisposition::CURRENT_TAB);

  NavigateParams params(browser, std::move(new_contents));
  params.source_contents = source_contents;
  params.disposition = disposition;
  params.window_bounds = initial_rect;
  params.window_action = NavigateParams::SHOW_WINDOW;
  // At this point, we're already beyond the popup blocker. Even if the popup
  // was created without a user gesture, we have to set |user_gesture| to true,
  // so it gets correctly focused.
  params.user_gesture = true;
  Navigate(&params);
}

void CloseWebContents(Browser* browser,
                      content::WebContents* contents,
                      bool add_to_history) {
  int index = browser->tab_strip_model()->GetIndexOfWebContents(contents);
  if (index == TabStripModel::kNoTab) {
    NOTREACHED() << "CloseWebContents called for tab not in our strip";
    return;
  }

  browser->tab_strip_model()->CloseWebContentsAt(
      index,
      add_to_history ? TabStripModel::CLOSE_CREATE_HISTORICAL_TAB
                     : TabStripModel::CLOSE_NONE);
}

}  // namespace chrome
