// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chromecast/browser/cast_http_user_agent_settings.h"

#include "base/i18n/rtl.h"
#include "base/logging.h"
#include "build/build_config.h"
#include "chromecast/app/grit/chromecast_settings.h"
#include "chromecast/common/cast_content_client.h"
#include "content/public/browser/browser_thread.h"
#include "net/http/http_util.h"
#include "ui/base/l10n/l10n_util.h"

#if defined(OS_ANDROID)
#include "base/android/locale_utils.h"
#endif  // defined(OS_ANDROID)

namespace chromecast {
namespace shell {

CastHttpUserAgentSettings::CastHttpUserAgentSettings() {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
}

CastHttpUserAgentSettings::~CastHttpUserAgentSettings() {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
}

std::string CastHttpUserAgentSettings::GetAcceptLanguage() const {
  DCHECK_CURRENTLY_ON(content::BrowserThread::IO);
  std::string new_locale(
#if defined(OS_ANDROID)
      // TODO(byungchul): Use transient locale set when new app starts.
      base::android::GetDefaultLocaleString()
#else
      base::i18n::GetConfiguredLocale()
#endif
      );
  if (new_locale != last_locale_ || accept_language_.empty()) {
    last_locale_ = new_locale;
    accept_language_ = net::HttpUtil::GenerateAcceptLanguageHeader(
#if defined(OS_ANDROID)
        last_locale_
#else
        l10n_util::GetStringUTF8(IDS_CHROMECAST_SETTINGS_ACCEPT_LANGUAGES)
#endif
        );
    LOG(INFO) << "Locale changed: accept_language=" << accept_language_;
  }
  return accept_language_;
}

std::string CastHttpUserAgentSettings::GetUserAgent() const {
  return chromecast::shell::GetUserAgent();
}

}  // namespace shell
}  // namespace chromecast
