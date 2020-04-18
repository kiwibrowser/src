// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_VR_VR_TAB_HELPER_H_
#define CHROME_BROWSER_VR_VR_TAB_HELPER_H_

#include "base/macros.h"
#include "chrome/browser/vr/ui_suppressed_element.h"
#include "content/public/browser/web_contents_user_data.h"

namespace vr {

class VrTabHelper : public content::WebContentsUserData<VrTabHelper> {
 public:
  ~VrTabHelper() override;

  bool is_in_vr() const { return is_in_vr_; }

  // Called by VrShell when we enter and exit vr mode. It finds us by looking us
  // up on the WebContents.
  void SetIsInVr(bool is_in_vr);

  static bool IsInVr(content::WebContents* contents);

  // If suppressed, this function will log a UMA stat.
  static bool IsUiSuppressedInVr(content::WebContents* contents,
                                 UiSuppressedElement element);

 private:
  explicit VrTabHelper(content::WebContents* contents);

  friend class content::WebContentsUserData<VrTabHelper>;

  content::WebContents* web_contents_;
  bool is_in_vr_ = false;

  DISALLOW_COPY_AND_ASSIGN(VrTabHelper);
};

}  // namespace vr

#endif  // CHROME_BROWSER_VR_VR_TAB_HELPER_H_
