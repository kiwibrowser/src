// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/signin/dice_tab_helper.h"

#include "base/logging.h"
#include "base/metrics/user_metrics.h"
#include "chrome/browser/signin/dice_tab_helper.h"
#include "chrome/browser/signin/signin_util.h"
#include "chrome/browser/ui/browser_finder.h"
#include "components/signin/core/browser/profile_management_switches.h"
#include "content/public/browser/navigation_handle.h"
#include "google_apis/gaia/gaia_urls.h"

DEFINE_WEB_CONTENTS_USER_DATA_KEY(DiceTabHelper);

DiceTabHelper::DiceTabHelper(content::WebContents* web_contents)
    : content::WebContentsObserver(web_contents) {}

DiceTabHelper::~DiceTabHelper() = default;

void DiceTabHelper::InitializeSigninFlow(
    signin_metrics::AccessPoint access_point,
    signin_metrics::Reason reason,
    signin_metrics::PromoAction promo_action) {
  signin_access_point_ = access_point;
  signin_reason_ = reason;
  signin_promo_action_ = promo_action;
  did_finish_loading_signin_page_ = false;

  if (signin_reason_ == signin_metrics::Reason::REASON_SIGNIN_PRIMARY_ACCOUNT) {
    signin_metrics::LogSigninAccessPointStarted(access_point, promo_action);
    signin_metrics::RecordSigninUserActionForAccessPoint(access_point,
                                                         promo_action);
    base::RecordAction(base::UserMetricsAction("Signin_SigninPage_Loading"));
  }
}

void DiceTabHelper::DidFinishLoad(content::RenderFrameHost* render_frame_host,
                                  const GURL& validated_url) {
  if (did_finish_loading_signin_page_)
    return;

  if (validated_url.GetOrigin() == GaiaUrls::GetInstance()->gaia_url()) {
    VLOG(1) << "Finished loading sign-in page: " << validated_url.spec();
    did_finish_loading_signin_page_ = true;
    base::RecordAction(base::UserMetricsAction("Signin_SigninPage_Shown"));
  }
}
