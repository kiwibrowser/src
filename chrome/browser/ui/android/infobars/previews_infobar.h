// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_ANDROID_INFOBARS_PREVIEWS_INFOBAR_H_
#define CHROME_BROWSER_UI_ANDROID_INFOBARS_PREVIEWS_INFOBAR_H_

#include "base/android/scoped_java_ref.h"
#include "base/macros.h"
#include "chrome/browser/previews/previews_infobar_delegate.h"
#include "chrome/browser/ui/android/infobars/confirm_infobar.h"

class PreviewsInfoBar : public ConfirmInfoBar {
 public:
  explicit PreviewsInfoBar(std::unique_ptr<PreviewsInfoBarDelegate> delegate);

  ~PreviewsInfoBar() override;

  // Returns a Previews infobar that owns |delegate|.
  static std::unique_ptr<infobars::InfoBar> CreateInfoBar(
      infobars::InfoBarManager* infobar_manager,
      std::unique_ptr<PreviewsInfoBarDelegate> delegate);

 private:
  // ConfirmInfoBar:
  base::android::ScopedJavaLocalRef<jobject> CreateRenderInfoBar(
      JNIEnv* env) override;

  DISALLOW_COPY_AND_ASSIGN(PreviewsInfoBar);
};

#endif  // CHROME_BROWSER_UI_ANDROID_INFOBARS_PREVIEWS_INFOBAR_H_
