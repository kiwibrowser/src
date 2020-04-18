// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_UI_DISPLAY_SCREEN_MANAGER_OZONE_EXTERNAL_H_
#define SERVICES_UI_DISPLAY_SCREEN_MANAGER_OZONE_EXTERNAL_H_

#include "services/ui/display/screen_manager.h"

namespace display {

// In external window mode, the purpose of having a ScreenManager
// does not apply: there is not a ScreenManagerDelegate manager
// responsible for creating Display instances.
// Basically, in this mode WindowTreeHost creates the display instance.
//
// ScreenManagerOzoneExternal provides the stub out implementation
// of ScreenManager for Ozone non-chromeos platforms.
class ScreenManagerOzoneExternal : public ScreenManager {
 public:
  ScreenManagerOzoneExternal();
  ~ScreenManagerOzoneExternal() override;

 private:
  // ScreenManager.
  void AddInterfaces(
      service_manager::BinderRegistryWithArgs<
          const service_manager::BindSourceInfo&>* registry) override;
  void Init(ScreenManagerDelegate* delegate) override;
  void RequestCloseDisplay(int64_t display_id) override;
  display::ScreenBase* GetScreen() override;

  std::unique_ptr<display::ScreenBase> screen_;

  DISALLOW_COPY_AND_ASSIGN(ScreenManagerOzoneExternal);
};

}  // namespace display

#endif  // SERVICES_UI_DISPLAY_SCREEN_MANAGER_OZONE_EXTERNAL_H_
