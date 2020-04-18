// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_UI_DISPLAY_SCREEN_MANAGER_H_
#define SERVICES_UI_DISPLAY_SCREEN_MANAGER_H_

#include <memory>

#include "base/macros.h"
#include "services/service_manager/public/cpp/bind_source_info.h"
#include "services/service_manager/public/cpp/binder_registry.h"
#include "services/ui/display/screen_manager_delegate.h"

namespace display {

class ScreenBase;

// ScreenManager provides the necessary functionality to configure all
// attached physical displays.
class ScreenManager {
 public:
  ScreenManager();
  virtual ~ScreenManager();

  // Creates a singleton ScreenManager instance.
  static std::unique_ptr<ScreenManager> Create();
  static ScreenManager* GetInstance();

  // Registers Mojo interfaces provided.
  virtual void AddInterfaces(
      service_manager::BinderRegistryWithArgs<
          const service_manager::BindSourceInfo&>* registry) = 0;

  // Triggers initial display configuration to start. On device this will
  // configuration the connected displays. Off device this will create one or
  // more fake displays and pretend to configure them. A non-null |delegate|
  // must be provided that will receive notifications when displays are added,
  // removed or modified.
  virtual void Init(ScreenManagerDelegate* delegate) = 0;

  // Handle requests from the platform to close a display.
  virtual void RequestCloseDisplay(int64_t display_id) = 0;

  virtual display::ScreenBase* GetScreen() = 0;

 private:
  static ScreenManager* instance_;  // Instance is not owned.

  DISALLOW_COPY_AND_ASSIGN(ScreenManager);
};

}  // namespace display

#endif  // SERVICES_UI_DISPLAY_SCREEN_MANAGER_H_
