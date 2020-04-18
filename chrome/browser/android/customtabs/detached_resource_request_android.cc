// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/android/jni_android.h"
#include "base/android/jni_string.h"
#include "base/android/scoped_java_ref.h"
#include "chrome/browser/android/customtabs/detached_resource_request.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/profiles/profile_android.h"
#include "content/public/common/referrer.h"
#include "jni/CustomTabsConnection_jni.h"
#include "third_party/blink/public/platform/web_referrer_policy.h"
#include "url/gurl.h"

namespace customtabs {

static void JNI_CustomTabsConnection_CreateAndStartDetachedResourceRequest(
    JNIEnv* env,
    const base::android::JavaParamRef<jclass>& jcaller,
    const base::android::JavaParamRef<jobject>& profile,
    const base::android::JavaParamRef<jstring>& url,
    const base::android::JavaParamRef<jstring>& origin,
    jint referrer_policy) {
  DCHECK(profile && url && origin);

  Profile* native_profile = ProfileAndroid::FromProfileAndroid(profile);
  DCHECK(native_profile);

  GURL native_url(base::android::ConvertJavaStringToUTF8(env, url));
  GURL native_origin(base::android::ConvertJavaStringToUTF8(env, origin));
  DCHECK(native_url.is_valid());
  DCHECK(native_origin.is_valid());

  // Java only knows about the blink referrer policy.
  net::URLRequest::ReferrerPolicy url_request_referrer_policy =
      content::Referrer::ReferrerPolicyForUrlRequest(
          static_cast<blink::WebReferrerPolicy>(referrer_policy));
  DetachedResourceRequest::CreateAndStart(
      native_profile, native_url, native_origin, url_request_referrer_policy);
}

}  // namespace customtabs
