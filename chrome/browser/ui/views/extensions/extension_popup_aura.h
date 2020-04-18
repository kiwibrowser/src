// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_VIEWS_EXTENSIONS_EXTENSION_POPUP_AURA_H_
#define CHROME_BROWSER_UI_VIEWS_EXTENSIONS_EXTENSION_POPUP_AURA_H_

#include "base/macros.h"
#include "chrome/browser/ui/views/extensions/extension_popup.h"
#include "ui/wm/public/activation_change_observer.h"

class ExtensionPopupAura : public ExtensionPopup,
                           public wm::ActivationChangeObserver {
 public:
  ExtensionPopupAura(extensions::ExtensionViewHost* host,
                     views::View* anchor_view,
                     views::BubbleBorder::Arrow arrow,
                     ShowAction show_action);
  ~ExtensionPopupAura() override;

  // views::WidgetObserver overrides.
  void OnWidgetDestroying(views::Widget* widget) override;

  // wm::ActivationChangeObserver overrides.
  void OnWindowActivated(wm::ActivationChangeObserver::ActivationReason reason,
                         aura::Window* gained_active,
                         aura::Window* lost_active) override;

 private:
  DISALLOW_COPY_AND_ASSIGN(ExtensionPopupAura);
};

#endif  // CHROME_BROWSER_UI_VIEWS_EXTENSIONS_EXTENSION_POPUP_AURA_H_
