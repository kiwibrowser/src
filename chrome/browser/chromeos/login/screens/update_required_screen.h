// Copyright (c) 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_CHROMEOS_LOGIN_SCREENS_UPDATE_REQUIRED_SCREEN_H_
#define CHROME_BROWSER_CHROMEOS_LOGIN_SCREENS_UPDATE_REQUIRED_SCREEN_H_

#include <set>

#include "base/callback.h"
#include "base/compiler_specific.h"
#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "chrome/browser/chromeos/login/screens/base_screen.h"
#include "chrome/browser/chromeos/login/screens/error_screen.h"

namespace chromeos {

class BaseScreenDelegate;
class UpdateRequiredView;

// Controller for the update required screen.
class UpdateRequiredScreen : public BaseScreen {
 public:
  constexpr static OobeScreen kScreenId = OobeScreen::SCREEN_UPDATE_REQUIRED;

  UpdateRequiredScreen(BaseScreenDelegate* base_screen_delegate,
                       UpdateRequiredView* view);
  ~UpdateRequiredScreen() override;

  // Called when the being destroyed. This should call Unbind() on the
  // associated View if this class is destroyed before it.
  void OnViewDestroyed(UpdateRequiredView* view);

 private:
  // BaseScreen:
  void Show() override;
  void Hide() override;

  UpdateRequiredView* view_ = nullptr;
  bool is_shown_;

  base::WeakPtrFactory<UpdateRequiredScreen> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(UpdateRequiredScreen);
};

}  // namespace chromeos

#endif  // CHROME_BROWSER_CHROMEOS_LOGIN_SCREENS_UPDATE_REQUIRED_SCREEN_H_
