// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_SCREEN_ORIENTATION_SCREEN_ORIENTATION_DELEGATE_ANDROID_H_
#define CONTENT_BROWSER_SCREEN_ORIENTATION_SCREEN_ORIENTATION_DELEGATE_ANDROID_H_

#include <jni.h>

#include "base/macros.h"
#include "content/public/browser/screen_orientation_delegate.h"
#include "third_party/blink/public/common/screen_orientation/web_screen_orientation_lock_type.h"

namespace content {

class WebContents;

// Android implementation of ScreenOrientationDelegate. The functionality of
// ScreenOrientationProvider is always supported.
class ScreenOrientationDelegateAndroid : public ScreenOrientationDelegate {
 public:
  ScreenOrientationDelegateAndroid();
  ~ScreenOrientationDelegateAndroid() override;

  // ScreenOrientationDelegate:
  bool FullScreenRequired(WebContents* web_contents) override;
  void Lock(WebContents* web_contents,
            blink::WebScreenOrientationLockType lock_orientation) override;
  bool ScreenOrientationProviderSupported() override;
  void Unlock(WebContents* web_contents) override;

 private:
  DISALLOW_COPY_AND_ASSIGN(ScreenOrientationDelegateAndroid);
};

} // namespace content

#endif  // CONTENT_BROWSER_SCREEN_ORIENTATION_SCREEN_ORIENTATION_DELEGATE_ANDROID_H_
