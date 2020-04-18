// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_CHROMEOS_LOGIN_SCREENS_ARC_TERMS_OF_SERVICE_SCREEN_H_
#define CHROME_BROWSER_CHROMEOS_LOGIN_SCREENS_ARC_TERMS_OF_SERVICE_SCREEN_H_

#include <string>

#include "base/macros.h"
#include "chrome/browser/chromeos/login/screens/arc_terms_of_service_screen_view_observer.h"
#include "chrome/browser/chromeos/login/screens/base_screen.h"

namespace chromeos {

class ArcTermsOfServiceScreenView;
class BaseScreenDelegate;

class ArcTermsOfServiceScreen : public BaseScreen,
                                public ArcTermsOfServiceScreenViewObserver {
 public:
  ArcTermsOfServiceScreen(BaseScreenDelegate* base_screen_delegate,
                          ArcTermsOfServiceScreenView* view);
  ~ArcTermsOfServiceScreen() override;

  // BaseScreen:
  void Show() override;
  void Hide() override;

  // ArcTermsOfServiceScreenViewObserver:
  void OnSkip() override;
  void OnAccept() override;
  void OnViewDestroyed(ArcTermsOfServiceScreenView* view) override;

 private:
  ArcTermsOfServiceScreenView* view_;

  DISALLOW_COPY_AND_ASSIGN(ArcTermsOfServiceScreen);
};

}  // namespace chromeos

#endif  // CHROME_BROWSER_CHROMEOS_LOGIN_SCREENS_ARC_TERMS_OF_SERVICE_SCREEN_H_
