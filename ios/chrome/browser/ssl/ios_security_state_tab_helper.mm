// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ios/chrome/browser/ssl/ios_security_state_tab_helper.h"

#include "base/bind.h"
#include "base/command_line.h"
#include "base/metrics/field_trial.h"
#include "base/metrics/histogram_macros.h"
#include "components/security_state/core/security_state.h"
#include "components/security_state/ios/ssl_status_input_event_data.h"
#include "ios/web/public/browser_state.h"
#include "ios/web/public/navigation_item.h"
#import "ios/web/public/navigation_manager.h"
#import "ios/web/public/origin_util.h"
#include "ios/web/public/security_style.h"
#include "ios/web/public/ssl_status.h"
#include "ios/web/public/web_state/web_state.h"
#include "net/cert/x509_certificate.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

DEFINE_WEB_STATE_USER_DATA_KEY(IOSSecurityStateTabHelper);

IOSSecurityStateTabHelper::IOSSecurityStateTabHelper(web::WebState* web_state)
    : web_state_(web_state) {}

IOSSecurityStateTabHelper::~IOSSecurityStateTabHelper() {}

void IOSSecurityStateTabHelper::GetSecurityInfo(
    security_state::SecurityInfo* result) const {
  security_state::GetSecurityInfo(GetVisibleSecurityState(),
                                  false /* used policy installed certificate */,
                                  base::Bind(&web::IsOriginSecure), result);
}

std::unique_ptr<security_state::VisibleSecurityState>
IOSSecurityStateTabHelper::GetVisibleSecurityState() const {
  auto state = std::make_unique<security_state::VisibleSecurityState>();

  web::NavigationItem* item =
      web_state_->GetNavigationManager()->GetVisibleItem();
  if (!item || item->GetSSL().security_style == web::SECURITY_STYLE_UNKNOWN)
    return state;

  state->connection_info_initialized = true;
  state->url = item->GetURL();
  const web::SSLStatus& ssl = item->GetSSL();
  state->certificate = ssl.certificate;
  state->cert_status = ssl.cert_status;
  state->displayed_mixed_content =
      (ssl.content_status & web::SSLStatus::DISPLAYED_INSECURE_CONTENT) ? true
                                                                        : false;
  state->is_incognito = web_state_->GetBrowserState()->IsOffTheRecord();

  security_state::SSLStatusInputEventData* input_events =
      static_cast<security_state::SSLStatusInputEventData*>(
          ssl.user_data.get());
  if (input_events)
    state->insecure_input_events = *input_events->input_events();

  return state;
}
