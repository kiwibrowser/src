// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_NET_PREDICTOR_TAB_HELPER_H_
#define CHROME_BROWSER_NET_PREDICTOR_TAB_HELPER_H_

#include "base/macros.h"
#include "content/public/browser/web_contents_observer.h"
#include "content/public/browser/web_contents_user_data.h"

namespace chrome_browser_net {

class PredictorTabHelper
    : public content::WebContentsObserver,
      public content::WebContentsUserData<PredictorTabHelper> {
 public:
  ~PredictorTabHelper() override;

  // content::WebContentsObserver:
  void DocumentOnLoadCompletedInMainFrame() override;

 private:
  explicit PredictorTabHelper(content::WebContents* web_contents);
  friend class content::WebContentsUserData<PredictorTabHelper>;

  DISALLOW_COPY_AND_ASSIGN(PredictorTabHelper);
};

}  // namespace chrome_browser_net

#endif  // CHROME_BROWSER_NET_PREDICTOR_TAB_HELPER_H_
