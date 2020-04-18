// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_CHROMEOS_LOGIN_SCREENS_TERMS_OF_SERVICE_SCREEN_H_
#define CHROME_BROWSER_CHROMEOS_LOGIN_SCREENS_TERMS_OF_SERVICE_SCREEN_H_

#include <memory>

#include "base/compiler_specific.h"
#include "base/macros.h"
#include "base/timer/timer.h"
#include "chrome/browser/chromeos/login/screens/base_screen.h"
#include "chrome/browser/chromeos/login/screens/terms_of_service_screen_view.h"

namespace network {
class SimpleURLLoader;
}

namespace chromeos {

class BaseScreenDelegate;

// A screen that shows Terms of Service which have been configured through
// policy. The screen is shown during login and requires the user to accept the
// Terms of Service before proceeding. Currently, Terms of Service are available
// for public sessions only.
class TermsOfServiceScreen : public BaseScreen,
                             public TermsOfServiceScreenView::Delegate {
 public:
  TermsOfServiceScreen(BaseScreenDelegate* base_screen_delegate,
                       TermsOfServiceScreenView* view);
  ~TermsOfServiceScreen() override;

  // BaseScreen:
  void Show() override;
  void Hide() override;

  // TermsOfServiceScreenActor::Delegate:
  void OnDecline() override;
  void OnAccept() override;
  void OnViewDestroyed(TermsOfServiceScreenView* view) override;

 private:
  // Start downloading the Terms of Service.
  void StartDownload();

  // Abort the attempt to download the Terms of Service if it takes too long.
  void OnDownloadTimeout();

  // Callback function called when SimpleURLLoader completes.
  void OnDownloaded(std::unique_ptr<std::string> response_body);

  TermsOfServiceScreenView* view_;

  std::unique_ptr<network::SimpleURLLoader> terms_of_service_loader_;

  // Timer that enforces a custom (shorter) timeout on the attempt to download
  // the Terms of Service.
  base::OneShotTimer download_timer_;

  DISALLOW_COPY_AND_ASSIGN(TermsOfServiceScreen);
};

}  // namespace chromeos

#endif  // CHROME_BROWSER_CHROMEOS_LOGIN_SCREENS_TERMS_OF_SERVICE_SCREEN_H_
