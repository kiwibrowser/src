// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ios/chrome/browser/net/ios_chrome_http_user_agent_settings.h"

#include "components/prefs/pref_service.h"
#include "ios/chrome/browser/pref_names.h"
#include "ios/web/public/web_client.h"
#include "ios/web/public/web_thread.h"
#include "net/http/http_util.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

IOSChromeHttpUserAgentSettings::IOSChromeHttpUserAgentSettings(
    PrefService* prefs) {
  DCHECK_CURRENTLY_ON(web::WebThread::UI);
  pref_accept_language_.Init(prefs::kAcceptLanguages, prefs);
  last_pref_accept_language_ = *pref_accept_language_;
  last_http_accept_language_ =
      net::HttpUtil::GenerateAcceptLanguageHeader(last_pref_accept_language_);
  pref_accept_language_.MoveToThread(
      web::WebThread::GetTaskRunnerForThread(web::WebThread::IO));
}

IOSChromeHttpUserAgentSettings::~IOSChromeHttpUserAgentSettings() {
  DCHECK_CURRENTLY_ON(web::WebThread::IO);
}

void IOSChromeHttpUserAgentSettings::CleanupOnUIThread() {
  DCHECK_CURRENTLY_ON(web::WebThread::UI);
  pref_accept_language_.Destroy();
}

std::string IOSChromeHttpUserAgentSettings::GetAcceptLanguage() const {
  DCHECK_CURRENTLY_ON(web::WebThread::IO);
  std::string new_pref_accept_language = *pref_accept_language_;
  if (new_pref_accept_language != last_pref_accept_language_) {
    last_http_accept_language_ =
        net::HttpUtil::GenerateAcceptLanguageHeader(new_pref_accept_language);
    last_pref_accept_language_ = new_pref_accept_language;
  }
  return last_http_accept_language_;
}

std::string IOSChromeHttpUserAgentSettings::GetUserAgent() const {
  DCHECK_CURRENTLY_ON(web::WebThread::IO);
  return web::GetWebClient()->GetUserAgent(web::UserAgentType::MOBILE);
}
