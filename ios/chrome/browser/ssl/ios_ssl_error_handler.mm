// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ios/chrome/browser/ssl/ios_ssl_error_handler.h"

#include "base/bind.h"
#include "base/callback.h"
#include "base/feature_list.h"
#include "base/mac/bind_objc_block.h"
#include "base/metrics/histogram_macros.h"
#include "components/captive_portal/captive_portal_detector.h"
#include "components/security_interstitials/core/ssl_error_ui.h"
#include "ios/chrome/browser/ssl/captive_portal_detector_tab_helper.h"
#include "ios/chrome/browser/ssl/captive_portal_features.h"
#include "ios/chrome/browser/ssl/captive_portal_metrics.h"
#include "ios/chrome/browser/ssl/ios_captive_portal_blocking_page.h"
#include "ios/chrome/browser/ssl/ios_ssl_blocking_page.h"
#include "ios/web/public/browser_state.h"
#import "ios/web/public/web_state/web_state.h"
#include "net/ssl/ssl_info.h"
#include "net/traffic_annotation/network_traffic_annotation.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

DEFINE_WEB_STATE_USER_DATA_KEY(IOSSSLErrorHandler);

const char kSessionDetectionResultHistogram[] =
    "CaptivePortal.Session.DetectionResult";

const int64_t kSSLInterstitialDelayInSeconds = 3;

using captive_portal::CaptivePortalDetector;

// static
void IOSSSLErrorHandler::HandleSSLError(
    web::WebState* web_state,
    int cert_error,
    const net::SSLInfo& info,
    const GURL& request_url,
    bool overridable,
    const base::Callback<void(bool)>& callback) {
  DCHECK(!web_state->IsShowingWebInterstitial());
  DCHECK(web_state);
  DCHECK(!FromWebState(web_state));
  // TODO(crbug.com/747405): If certificate error is only a name mismatch,
  // check if the cert is from a known captive portal.

  web_state->SetUserData(
      UserDataKey(),
      base::WrapUnique(new IOSSSLErrorHandler(
          web_state, cert_error, info, request_url, overridable, callback)));
  FromWebState(web_state)->StartHandlingError();
}

IOSSSLErrorHandler::~IOSSSLErrorHandler() = default;

IOSSSLErrorHandler::IOSSSLErrorHandler(
    web::WebState* web_state,
    int cert_error,
    const net::SSLInfo& info,
    const GURL& request_url,
    bool overridable,
    const base::Callback<void(bool)>& callback)
    : web_state_(web_state),
      cert_error_(cert_error),
      ssl_info_(info),
      request_url_(request_url),
      overridable_(overridable),
      callback_(callback),
      weak_factory_(this) {}

void IOSSSLErrorHandler::StartHandlingError() {
  if (!base::FeatureList::IsEnabled(kCaptivePortalFeature)) {
    IOSSSLErrorHandler::RecordCaptivePortalState(web_state_);

    // Display an SSL interstitial.
    ShowSSLInterstitial();
    return;
  }

  CaptivePortalDetectorTabHelper* tab_helper =
      CaptivePortalDetectorTabHelper::FromWebState(web_state_);
  // TODO(crbug.com/760873): replace test with DCHECK when this method is only
  // called on WebStates attached to tabs.
  if (tab_helper) {
    base::WeakPtr<IOSSSLErrorHandler> weak_error_handler =
        weak_factory_.GetWeakPtr();

    tab_helper->detector()->DetectCaptivePortal(
        GURL(CaptivePortalDetector::kDefaultURL),
        base::Bind(&IOSSSLErrorHandler::HandleCaptivePortalDetectionResult,
                   weak_error_handler),
        NO_TRAFFIC_ANNOTATION_YET);
  }

  // Default to presenting the SSL interstitial if Captive Portal detection
  // takes too long.
  timer_.Start(FROM_HERE,
               base::TimeDelta::FromSeconds(kSSLInterstitialDelayInSeconds),
               this, &IOSSSLErrorHandler::ShowSSLInterstitial);
}

void IOSSSLErrorHandler::HandleCaptivePortalDetectionResult(
    const CaptivePortalDetector::Results& results) {
  timer_.Stop();

  IOSSSLErrorHandler::LogCaptivePortalResult(results.result);
  if (results.result == captive_portal::RESULT_BEHIND_CAPTIVE_PORTAL) {
    ShowCaptivePortalInterstitial(results.landing_url);
  } else {
    ShowSSLInterstitial();
  }
}

void IOSSSLErrorHandler::ShowSSLInterstitial() {
  timer_.Stop();

  // Cancel the captive portal detection if it is still ongoing. This will be
  // the case if |timer_| triggered the call of this method.
  CaptivePortalDetectorTabHelper* tab_helper =
      CaptivePortalDetectorTabHelper::FromWebState(web_state_);
  // TODO(crbug.com/760873): replace test with DCHECK when this method is only
  // called on WebStates attached to tabs.
  if (tab_helper) {
    tab_helper->detector()->Cancel();
  }

  int options_mask =
      overridable_ ? security_interstitials::SSLErrorUI::SOFT_OVERRIDE_ENABLED
                   : security_interstitials::SSLErrorUI::STRICT_ENFORCEMENT;
  // SSLBlockingPage deletes itself when it's dismissed.
  IOSSSLBlockingPage* page = new IOSSSLBlockingPage(
      web_state_, cert_error_, ssl_info_, request_url_, options_mask,
      base::Time::NowFromSystemTime(), callback_);
  page->Show();
  // Once an interstitial is displayed, no need to keep the handler around.
  // This is the equivalent of "delete this".
  RemoveFromWebState(web_state_);
}

void IOSSSLErrorHandler::ShowCaptivePortalInterstitial(
    const GURL& landing_url) {
  // IOSCaptivePortalBlockingPage deletes itself when it's dismissed.
  IOSCaptivePortalBlockingPage* page = new IOSCaptivePortalBlockingPage(
      web_state_, request_url_, landing_url, callback_);
  page->Show();
  // Once an interstitial is displayed, no need to keep the handler around.
  // This is the equivalent of "delete this".
  RemoveFromWebState(web_state_);
}

// static
void IOSSSLErrorHandler::RecordCaptivePortalState(web::WebState* web_state) {
  CaptivePortalDetectorTabHelper* tab_helper =
      CaptivePortalDetectorTabHelper::FromWebState(web_state);

  // TODO(crbug.com/760873): replace test with DCHECK when this method is only
  // called on WebStates attached to tabs.
  if (!tab_helper) {
    return;
  }
  tab_helper->detector()->DetectCaptivePortal(
      GURL(CaptivePortalDetector::kDefaultURL),
      base::BindBlockArc(^(const CaptivePortalDetector::Results& results) {
        IOSSSLErrorHandler::LogCaptivePortalResult(results.result);
      }),
      NO_TRAFFIC_ANNOTATION_YET);
}

// static
void IOSSSLErrorHandler::LogCaptivePortalResult(
    captive_portal::CaptivePortalResult result) {
  CaptivePortalStatus status;
  switch (result) {
    case captive_portal::RESULT_INTERNET_CONNECTED:
      status = CaptivePortalStatus::ONLINE;
      break;
    case captive_portal::RESULT_BEHIND_CAPTIVE_PORTAL:
      status = CaptivePortalStatus::PORTAL;
      break;
    default:
      status = CaptivePortalStatus::UNKNOWN;
      break;
  }
  UMA_HISTOGRAM_ENUMERATION(kSessionDetectionResultHistogram,
                            static_cast<int>(status),
                            static_cast<int>(CaptivePortalStatus::COUNT));
}
