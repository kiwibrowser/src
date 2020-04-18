// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ANDROID_WEBVIEW_BROWSER_AW_HTTP_AUTH_HANDLER_H_
#define ANDROID_WEBVIEW_BROWSER_AW_HTTP_AUTH_HANDLER_H_

#include <string>

#include "base/android/jni_android.h"
#include "base/android/scoped_java_ref.h"
#include "base/memory/ref_counted.h"

namespace content {
class WebContents;
};

namespace net {
class AuthChallengeInfo;
};

namespace android_webview {

class AwLoginDelegate;

// Native class for Java class of same name and owns an instance
// of that Java object.
// One instance of this class is created per underlying AwLoginDelegate.
class AwHttpAuthHandler {
 public:
  AwHttpAuthHandler(AwLoginDelegate* login_delegate,
                    net::AuthChallengeInfo* auth_info,
                    bool first_auth_attempt);
  ~AwHttpAuthHandler();

  // from AwHttpAuthHandler
  bool HandleOnUIThread(content::WebContents* web_contents);

  void Proceed(JNIEnv* env,
               const base::android::JavaParamRef<jobject>& obj,
               const base::android::JavaParamRef<jstring>& username,
               const base::android::JavaParamRef<jstring>& password);
  void Cancel(JNIEnv* env, const base::android::JavaParamRef<jobject>& obj);

 private:
  scoped_refptr<AwLoginDelegate> login_delegate_;
  base::android::ScopedJavaGlobalRef<jobject> http_auth_handler_;
  std::string host_;
  std::string realm_;
};

}  // namespace android_webview

#endif  // ANDROID_WEBVIEW_BROWSER_AW_HTTP_AUTH_HANDLER_H_
