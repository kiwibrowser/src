// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "remoting/base/oauth_helper.h"

#include "base/strings/stringprintf.h"
#include "google_apis/google_api_keys.h"
#include "net/base/escape.h"
#include "url/third_party/mozilla/url_parse.h"

namespace {

std::string GetComponent(const std::string& url,
                         const url::Component component) {
  if (component.len < 0) {
    return std::string();
  }
  return url.substr(component.begin, component.len);
}

}  // namespace

namespace remoting {

std::string GetOauthScope() {
  return "https://www.googleapis.com/auth/chromoting "
         "https://www.googleapis.com/auth/googletalk "
         "https://www.googleapis.com/auth/userinfo.email ";
}

std::string GetDefaultOauthRedirectUrl() {
  return "https://chromoting-oauth.talkgadget.google.com/talkgadget/oauth/"
         "chrome-remote-desktop/rel/kgngmbheleoaphbjbaiobfdepmghbfah";
}

std::string GetOauthStartUrl(const std::string& redirect_url) {
  return base::StringPrintf(
      "https://accounts.google.com/o/oauth2/auth"
      "?scope=%s"
      "&redirect_uri=%s"
      "&response_type=code"
      "&client_id=%s"
      "&access_type=offline"
      "&approval_prompt=force",
      net::EscapeUrlEncodedData(GetOauthScope(), true).c_str(),
      redirect_url.c_str(),
      net::EscapeUrlEncodedData(
          google_apis::GetOAuth2ClientID(google_apis::CLIENT_REMOTING), true)
          .c_str());
}

std::string GetOauthCodeInUrl(const std::string& url,
                              const std::string& redirect_url) {
  url::Parsed url_parsed;
  ParseStandardURL(url.c_str(), url.length(), &url_parsed);
  url::Parsed redirect_url_parsed;
  ParseStandardURL(redirect_url.c_str(), redirect_url.length(),
                   &redirect_url_parsed);
  if (GetComponent(url, url_parsed.scheme) !=
      GetComponent(redirect_url, redirect_url_parsed.scheme)) {
    return std::string();
  }
  if (GetComponent(url, url_parsed.host) !=
      GetComponent(redirect_url, redirect_url_parsed.host)) {
    return std::string();
  }
  url::Component query = url_parsed.query;
  url::Component key;
  url::Component value;
  while (ExtractQueryKeyValue(url.c_str(), &query, &key, &value)) {
    if (GetComponent(url, key) == "code") {
      return GetComponent(url, value);
    }
  }
  return std::string();
}

}  // namespace remoting
