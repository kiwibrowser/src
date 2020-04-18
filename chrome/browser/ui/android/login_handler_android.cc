// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/login/login_handler.h"

#include <memory>

#include "base/logging.h"
#include "base/optional.h"
#include "base/strings/string16.h"
#include "base/strings/utf_string_conversions.h"
#include "chrome/browser/ui/android/chrome_http_auth_handler.h"
#include "chrome/browser/ui/android/view_android_helper.h"
#include "chrome/browser/vr/vr_tab_helper.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/web_contents.h"
#include "net/base/auth.h"
#include "ui/android/view_android.h"
#include "ui/android/window_android.h"

using content::BrowserThread;
using net::AuthChallengeInfo;

class LoginHandlerAndroid : public LoginHandler {
 public:
  LoginHandlerAndroid(
      net::AuthChallengeInfo* auth_info,
      content::ResourceRequestInfo::WebContentsGetter web_contents_getter,
      LoginAuthRequiredCallback auth_required_callback)
      : LoginHandler(auth_info,
                     web_contents_getter,
                     std::move(auth_required_callback)) {}

  // LoginHandler methods:

  void OnAutofillDataAvailableInternal(
      const base::string16& username,
      const base::string16& password) override {
    DCHECK_CURRENTLY_ON(BrowserThread::UI);
    DCHECK(chrome_http_auth_handler_.get() != NULL);
    chrome_http_auth_handler_->OnAutofillDataAvailable(
        username, password);
  }
  void OnLoginModelDestroying() override {}

  void BuildViewImpl(const base::string16& authority,
                     const base::string16& explanation,
                     LoginModelData* login_model_data) override {
    DCHECK_CURRENTLY_ON(BrowserThread::UI);

    // Get pointer to TabAndroid
    content::WebContents* web_contents = GetWebContentsForLogin();
    CHECK(web_contents);
    ViewAndroidHelper* view_helper = ViewAndroidHelper::FromWebContents(
        web_contents);

    if (vr::VrTabHelper::IsUiSuppressedInVr(
            web_contents, vr::UiSuppressedElement::kHttpAuth)) {
      CancelAuth();
      return;
    }

    // Notify WindowAndroid that HTTP authentication is required.
    if (view_helper->GetViewAndroid() &&
        view_helper->GetViewAndroid()->GetWindowAndroid()) {
      chrome_http_auth_handler_.reset(
          new ChromeHttpAuthHandler(authority, explanation));
      chrome_http_auth_handler_->Init();
      chrome_http_auth_handler_->SetObserver(this);
      chrome_http_auth_handler_->ShowDialog(
          view_helper->GetViewAndroid()->GetWindowAndroid()->GetJavaObject());

      if (login_model_data)
        SetModel(*login_model_data);
      else
        ResetModel();

      NotifyAuthNeeded();
    } else {
      CancelAuth();
      LOG(WARNING) << "HTTP Authentication failed because TabAndroid is "
          "missing";
    }
  }

 protected:
  ~LoginHandlerAndroid() override {}

  void CloseDialog() override {}

 private:
  std::unique_ptr<ChromeHttpAuthHandler> chrome_http_auth_handler_;
};

// static
scoped_refptr<LoginHandler> LoginHandler::Create(
    net::AuthChallengeInfo* auth_info,
    content::ResourceRequestInfo::WebContentsGetter web_contents_getter,
    LoginAuthRequiredCallback auth_required_callback) {
  return base::MakeRefCounted<LoginHandlerAndroid>(
      auth_info, web_contents_getter, std::move(auth_required_callback));
}
