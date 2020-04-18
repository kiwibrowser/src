// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_ANDROID_INFOBARS_INFOBAR_ANDROID_H_
#define CHROME_BROWSER_UI_ANDROID_INFOBARS_INFOBAR_ANDROID_H_

#include <string>

#include "base/android/scoped_java_ref.h"
#include "base/macros.h"
#include "components/infobars/core/infobar.h"

namespace infobars {
class InfoBarDelegate;
}

class InfoBarAndroid : public infobars::InfoBar {
 public:

  // A Java counterpart will be generated for this enum.
  // GENERATED_JAVA_ENUM_PACKAGE: org.chromium.chrome.browser.infobar
  // GENERATED_JAVA_PREFIX_TO_STRIP: ACTION_
  enum ActionType {
    ACTION_NONE = 0,
    // Confirm infobar
    ACTION_OK = 1,
    ACTION_CANCEL = 2,
    // Translate infobar
    ACTION_TRANSLATE = 3,
    ACTION_TRANSLATE_SHOW_ORIGINAL = 4,
  };

  explicit InfoBarAndroid(std::unique_ptr<infobars::InfoBarDelegate> delegate);
  ~InfoBarAndroid() override;

  // InfoBar:
  virtual base::android::ScopedJavaLocalRef<jobject> CreateRenderInfoBar(
      JNIEnv* env) = 0;

  virtual void SetJavaInfoBar(
      const base::android::JavaRef<jobject>& java_info_bar);
  const base::android::JavaRef<jobject>& GetJavaInfoBar();
  bool HasSetJavaInfoBar() const;

  // Tells the Java-side counterpart of this InfoBar to point to the replacement
  // InfoBar instead of this one.
  void ReassignJavaInfoBar(InfoBarAndroid* replacement);

  int GetInfoBarIdentifier(JNIEnv* env,
                           const base::android::JavaParamRef<jobject>& obj);
  virtual void OnLinkClicked(JNIEnv* env,
                             const base::android::JavaParamRef<jobject>& obj) {}
  void OnButtonClicked(JNIEnv* env,
                       const base::android::JavaParamRef<jobject>& obj,
                       jint action);
  void OnCloseButtonClicked(JNIEnv* env,
                            const base::android::JavaParamRef<jobject>& obj);

  void CloseJavaInfoBar();

  // Maps from a Chromium ID (IDR_TRANSLATE) to a enum value that Java code can
  // translate into a Drawable ID using the ResourceId class.
  int GetEnumeratedIconId();

  // Acquire the java infobar from a different one.  This is used to do in-place
  // replacements.
  virtual void PassJavaInfoBar(InfoBarAndroid* source) {}

 protected:
  // Derived classes must implement this method to process the corresponding
  // action.
  virtual void ProcessButton(int action) = 0;

  void CloseInfoBar();
  InfoBarAndroid* infobar_android() { return this; }

 private:
  base::android::ScopedJavaGlobalRef<jobject> java_info_bar_;

  DISALLOW_COPY_AND_ASSIGN(InfoBarAndroid);
};

#endif  // CHROME_BROWSER_UI_ANDROID_INFOBARS_INFOBAR_ANDROID_H_
