// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_CHROMEOS_LOGIN_SCREENS_ARC_TERMS_OF_SERVICE_SCREEN_VIEW_OBSERVER_H_
#define CHROME_BROWSER_CHROMEOS_LOGIN_SCREENS_ARC_TERMS_OF_SERVICE_SCREEN_VIEW_OBSERVER_H_

#include "base/macros.h"

namespace chromeos {

class ArcTermsOfServiceScreenView;

class ArcTermsOfServiceScreenViewObserver {
 public:
  virtual ~ArcTermsOfServiceScreenViewObserver() = default;

  // Called when the user skips the PlayStore Terms of Service.
  virtual void OnSkip() = 0;

  // Called when the user accepts the PlayStore Terms of Service.
  virtual void OnAccept() = 0;

  // Called when the view is destroyed so there is no dead reference to it.
  virtual void OnViewDestroyed(ArcTermsOfServiceScreenView* view) = 0;

 protected:
  ArcTermsOfServiceScreenViewObserver() = default;

 private:
  DISALLOW_COPY_AND_ASSIGN(ArcTermsOfServiceScreenViewObserver);
};

}  // namespace chromeos

#endif  // CHROME_BROWSER_CHROMEOS_LOGIN_SCREENS_ARC_TERMS_OF_SERVICE_SCREEN_VIEW_OBSERVER_H_
