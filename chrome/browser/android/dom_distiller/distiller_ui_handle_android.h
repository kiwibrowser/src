// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_ANDROID_DOM_DISTILLER_DISTILLER_UI_HANDLE_ANDROID_H_
#define CHROME_BROWSER_ANDROID_DOM_DISTILLER_DISTILLER_UI_HANDLE_ANDROID_H_

#include "base/macros.h"
#include "components/dom_distiller/content/browser/distiller_ui_handle.h"
#include "content/public/browser/web_contents.h"

namespace dom_distiller {

namespace android {

class DistillerUIHandleAndroid : public DistillerUIHandle {
 public:
  DistillerUIHandleAndroid() {}
  ~DistillerUIHandleAndroid() override {}

  void OpenSettings(content::WebContents* web_contents) override;

 private:
  DISALLOW_COPY_AND_ASSIGN(DistillerUIHandleAndroid);
};

}  // namespace android

}  // namespace dom_distiller

#endif  // CHROME_BROWSER_ANDROID_DOM_DISTILLER_DISTILLER_UI_HANDLE_ANDROID_H_
