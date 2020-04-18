// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/net/predictor_tab_helper.h"

#include "chrome/browser/net/predictor.h"
#include "chrome/browser/profiles/profile.h"

#if defined(OS_CHROMEOS)
#include "chrome/browser/chromeos/profiles/profile_helper.h"
#endif  // defined(OS_CHROMEOS)

DEFINE_WEB_CONTENTS_USER_DATA_KEY(chrome_browser_net::PredictorTabHelper);

namespace chrome_browser_net {

PredictorTabHelper::PredictorTabHelper(content::WebContents* web_contents)
    : content::WebContentsObserver(web_contents) {}

PredictorTabHelper::~PredictorTabHelper() {}

void PredictorTabHelper::DocumentOnLoadCompletedInMainFrame() {
  Profile* profile =
      Profile::FromBrowserContext(web_contents()->GetBrowserContext());
  Predictor* predictor = profile->GetNetworkPredictor();
#if defined(OS_CHROMEOS)
  if (chromeos::ProfileHelper::IsSigninProfile(profile))
    return;
#endif
  if (predictor)
    predictor->SaveStateForNextStartup();
}

}  // namespace chrome_browser_net
