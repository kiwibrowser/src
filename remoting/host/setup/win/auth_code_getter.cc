// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "remoting/host/setup/win/auth_code_getter.h"

#include <objbase.h>

#include "base/strings/utf_string_conversions.h"
#include "base/time/time.h"
#include "base/win/scoped_bstr.h"
#include "base/win/scoped_variant.h"
#include "remoting/base/oauth_helper.h"

namespace {
const int kUrlPollIntervalMs = 100;
}  // namespace remoting

namespace remoting {

AuthCodeGetter::AuthCodeGetter() :
    browser_(nullptr),
    timer_interval_(base::TimeDelta::FromMilliseconds(kUrlPollIntervalMs)) {
}

AuthCodeGetter::~AuthCodeGetter() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  KillBrowser();
}

void AuthCodeGetter::GetAuthCode(
    base::Callback<void(const std::string&)> on_auth_code) {
  if (browser_.Get()) {
    on_auth_code.Run("");
    return;
  }
  on_auth_code_ = on_auth_code;
  HRESULT hr = ::CoCreateInstance(CLSID_InternetExplorer, nullptr,
                                  CLSCTX_LOCAL_SERVER, IID_PPV_ARGS(&browser_));
  if (FAILED(hr)) {
    on_auth_code_.Run("");
    return;
  }
  base::win::ScopedBstr url(base::UTF8ToWide(
      GetOauthStartUrl(GetDefaultOauthRedirectUrl())).c_str());
  base::win::ScopedVariant empty_variant;
  hr = browser_->Navigate(url, empty_variant.AsInput(), empty_variant.AsInput(),
                          empty_variant.AsInput(), empty_variant.AsInput());
  if (FAILED(hr)) {
    KillBrowser();
    on_auth_code_.Run("");
    return;
  }
  browser_->put_Visible(VARIANT_TRUE);
  StartTimer();
}

void AuthCodeGetter::StartTimer() {
  timer_.Start(FROM_HERE, timer_interval_, this, &AuthCodeGetter::OnTimer);
}

void AuthCodeGetter::OnTimer() {
  std::string auth_code;
  if (TestBrowserUrl(&auth_code)) {
    on_auth_code_.Run(auth_code);
  } else {
    StartTimer();
  }
}

bool AuthCodeGetter::TestBrowserUrl(std::string* auth_code) {
  *auth_code = "";
  if (!browser_.Get()) {
    return true;
  }
  base::win::ScopedBstr url;
  HRESULT hr = browser_->get_LocationName(url.Receive());
  if (!SUCCEEDED(hr)) {
    KillBrowser();
    return true;
  }
  *auth_code = GetOauthCodeInUrl(base::WideToUTF8(static_cast<BSTR>(url)),
                                 GetDefaultOauthRedirectUrl());
  if (!auth_code->empty()) {
    KillBrowser();
    return true;
  }
  return false;
}

void AuthCodeGetter::KillBrowser() {
  if (browser_.Get()) {
    browser_->Quit();
    browser_.Reset();
  }
}

}  // namespace remoting
