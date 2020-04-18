// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_ANDROID_CONTENT_VIEW_CORE_H_
#define CONTENT_BROWSER_ANDROID_CONTENT_VIEW_CORE_H_

#include "base/android/jni_android.h"
#include "base/android/jni_weak_ref.h"
#include "base/android/scoped_java_ref.h"
#include "base/macros.h"
#include "content/public/browser/web_contents_observer.h"

namespace ui {
class ViewAndroid;
}

namespace content {

class RenderWidgetHostViewAndroid;
class WebContentsImpl;

class ContentViewCore : public WebContentsObserver {
 public:
  ContentViewCore(JNIEnv* env,
                  const base::android::JavaRef<jobject>& obj,
                  WebContents* web_contents);

  ~ContentViewCore() override;

  // --------------------------------------------------------------------------
  // Methods called from Java via JNI
  // --------------------------------------------------------------------------

  void OnJavaContentViewCoreDestroyed(
      JNIEnv* env,
      const base::android::JavaParamRef<jobject>& obj);

  void SetFocus(JNIEnv* env,
                const base::android::JavaParamRef<jobject>& obj,
                jboolean focused);

 private:

  // WebContentsObserver implementation.
  void RenderViewReady() override;
  void RenderViewHostChanged(RenderViewHost* old_host,
                             RenderViewHost* new_host) override;
  void WebContentsDestroyed() override;

  // --------------------------------------------------------------------------
  // Other private methods and data
  // --------------------------------------------------------------------------

  ui::ViewAndroid* GetViewAndroid() const;

  RenderWidgetHostViewAndroid* GetRenderWidgetHostViewAndroid() const;

  // Update focus state of the RenderWidgetHostView.
  void SetFocusInternal(bool focused);

  // A weak reference to the Java ContentViewCore object.
  JavaObjectWeakGlobalRef java_ref_;

  // Reference to the current WebContents used to determine how and what to
  // display in the ContentViewCore.
  WebContentsImpl* web_contents_;

  DISALLOW_COPY_AND_ASSIGN(ContentViewCore);
};

}  // namespace content

#endif  // CONTENT_BROWSER_ANDROID_CONTENT_VIEW_CORE_H_
