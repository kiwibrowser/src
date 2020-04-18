// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASH_DBUS_ASH_DBUS_SERVICES_H_
#define ASH_DBUS_ASH_DBUS_SERVICES_H_

#include <memory>

#include "base/macros.h"

namespace chromeos {
class CrosDBusService;
}  // namespace chromeos

namespace ash {

// Handles starting/stopping the D-Bus thread for ash services and also
// manages the liftime of the ash D-Bus services.
class AshDBusServices {
 public:
  AshDBusServices();
  ~AshDBusServices();

  // Emit ash-initialized upstart signal to start Chrome OS tasks that expect
  // that ash is listening to D-Bus signals they emit.
  void EmitAshInitialized();

 private:
  bool initialized_dbus_thread_{false};
  std::unique_ptr<chromeos::CrosDBusService> url_handler_service_;
  std::unique_ptr<chromeos::CrosDBusService> display_service_;

  DISALLOW_COPY_AND_ASSIGN(AshDBusServices);
};

}  // namespace ash

#endif  // ASH_DBUS_ASH_DBUS_SERVICES_H_
