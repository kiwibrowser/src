// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_SAD_TAB_HELPER_H_
#define CHROME_BROWSER_UI_SAD_TAB_HELPER_H_

#include <memory>

#include "base/compiler_specific.h"
#include "base/macros.h"
#include "content/public/browser/web_contents_observer.h"
#include "content/public/browser/web_contents_user_data.h"

class SadTab;

// Per-tab class to manage sad tab views.
class SadTabHelper : public content::WebContentsObserver,
                     public content::WebContentsUserData<SadTabHelper> {
 public:
  ~SadTabHelper() override;

  SadTab* sad_tab() { return sad_tab_.get(); }

  // Called when the sad tab needs to be reinstalled in the WebView,
  // for example because a tab was activated, or because a tab was
  // dragged to a new browser window.
  void ReinstallInWebView();

 private:
  friend class content::WebContentsUserData<SadTabHelper>;

  explicit SadTabHelper(content::WebContents* web_contents);

  void InstallSadTab(base::TerminationStatus status);

  // Overridden from content::WebContentsObserver:
  void RenderViewReady() override;
  void RenderProcessGone(base::TerminationStatus status) override;

  std::unique_ptr<SadTab> sad_tab_;

  DISALLOW_COPY_AND_ASSIGN(SadTabHelper);
};

#endif  // CHROME_BROWSER_UI_SAD_TAB_HELPER_H_
