// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASH_SYSTEM_LOCALE_LOCALE_NOTIFICATION_CONTROLLER_H_
#define ASH_SYSTEM_LOCALE_LOCALE_NOTIFICATION_CONTROLLER_H_

#include <string>

#include "ash/public/interfaces/locale.mojom.h"
#include "base/macros.h"
#include "mojo/public/cpp/bindings/binding_set.h"

namespace ash {

// Observes the locale change and creates rich notification for the change.
class LocaleNotificationController
    : public mojom::LocaleNotificationController {
 public:
  LocaleNotificationController();
  ~LocaleNotificationController() override;

  // Binds the mojom::LocaleNotificationController interface request to this
  // object.
  void BindRequest(mojom::LocaleNotificationControllerRequest request);

 private:
  // Overridden from mojom::LocaleNotificationController:
  void OnLocaleChanged(const std::string& cur_locale,
                       const std::string& from_locale,
                       const std::string& to_locale,
                       OnLocaleChangedCallback callback) override;

  std::string cur_locale_;
  std::string from_locale_;
  std::string to_locale_;

  // Bindings for the LocaleNotificationController interface.
  mojo::BindingSet<mojom::LocaleNotificationController> bindings_;

  DISALLOW_COPY_AND_ASSIGN(LocaleNotificationController);
};

}  // namespace ash

#endif  // ASH_SYSTEM_LOCALE_LOCALE_NOTIFICATION_CONTROLLER_H_
