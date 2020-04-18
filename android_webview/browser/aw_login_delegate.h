// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ANDROID_WEBVIEW_BROWSER_AW_LOGIN_DELEGATE_H_
#define ANDROID_WEBVIEW_BROWSER_AW_LOGIN_DELEGATE_H_

#include <memory>

#include "android_webview/browser/aw_http_auth_handler.h"
#include "base/callback.h"
#include "base/memory/ref_counted.h"
#include "base/optional.h"
#include "base/strings/string16.h"
#include "content/public/browser/content_browser_client.h"
#include "content/public/browser/login_delegate.h"
#include "content/public/browser/resource_request_info.h"

namespace net {
class AuthChallengeInfo;
}

namespace android_webview {

class AwLoginDelegate : public content::LoginDelegate {
 public:
  AwLoginDelegate(
      net::AuthChallengeInfo* auth_info,
      content::ResourceRequestInfo::WebContentsGetter web_contents_getter,
      bool first_auth_attempt,
      LoginAuthRequiredCallback auth_required_callback);

  virtual void Proceed(const base::string16& user,
                       const base::string16& password);
  virtual void Cancel();

  // content::LoginDelegate:
  void OnRequestCancelled() override;

 private:
  ~AwLoginDelegate() override;
  void HandleHttpAuthRequestOnUIThread(
      bool first_auth_attempt,
      const content::ResourceRequestInfo::WebContentsGetter&
          web_contents_getter);
  void CancelOnIOThread();
  void ProceedOnIOThread(const base::string16& user,
                         const base::string16& password);
  void DeleteAuthHandlerSoon();

  std::unique_ptr<AwHttpAuthHandler> aw_http_auth_handler_;
  scoped_refptr<net::AuthChallengeInfo> auth_info_;
  LoginAuthRequiredCallback auth_required_callback_;
};

}  // namespace android_webview

#endif  // ANDROID_WEBVIEW_BROWSER_AW_LOGIN_DELEGATE_H_
