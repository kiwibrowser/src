// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_WEBUI_CHROMEOS_LOGIN_RECOMMEND_APPS_SCREEN_HANDLER_H_
#define CHROME_BROWSER_UI_WEBUI_CHROMEOS_LOGIN_RECOMMEND_APPS_SCREEN_HANDLER_H_

#include "base/macros.h"
#include "chrome/browser/chromeos/login/screens/recommend_apps_screen_view.h"
#include "chrome/browser/ui/webui/chromeos/login/base_screen_handler.h"

namespace chromeos {

class RecommendAppsScreen;

// The sole implementation of the RecommendAppsScreenView, using WebUI.
class RecommendAppsScreenHandler : public BaseScreenHandler,
                                   public RecommendAppsScreenView {
 public:
  RecommendAppsScreenHandler();
  ~RecommendAppsScreenHandler() override;

  // BaseScreenHandler:
  void DeclareLocalizedValues(
      ::login::LocalizedValuesBuilder* builder) override;
  void RegisterMessages() override;

  // RecommendAppsScreenView:
  void Bind(RecommendAppsScreen* screen) override;
  void Show() override;
  void Hide() override;

 private:
  // BaseScreenHandler:
  void Initialize() override;

  RecommendAppsScreen* screen_ = nullptr;

  DISALLOW_COPY_AND_ASSIGN(RecommendAppsScreenHandler);
};

}  // namespace chromeos

#endif  // CHROME_BROWSER_UI_WEBUI_CHROMEOS_LOGIN_RECOMMEND_APPS_SCREEN_HANDLER_H_
