// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_UI_DISPLAY_SCREEN_MANAGER_STUB_INTERNAL_H_
#define SERVICES_UI_DISPLAY_SCREEN_MANAGER_STUB_INTERNAL_H_

#include <stdint.h>

#include "base/memory/weak_ptr.h"
#include "services/ui/display/screen_manager.h"
#include "ui/display/display.h"

namespace display {

class ScreenBase;

// ScreenManagerStubInternal provides the necessary functionality to configure a
// fixed 1024x768 display for non-ozone platforms.
class ScreenManagerStubInternal : public ScreenManager {
 public:
  ScreenManagerStubInternal();
  ~ScreenManagerStubInternal() override;

 private:
  // Fake creation of a single 1024x768 display.
  void FixedSizeScreenConfiguration();

  // ScreenManager.
  void AddInterfaces(
      service_manager::BinderRegistryWithArgs<
          const service_manager::BindSourceInfo&>* registry) override;
  void Init(ScreenManagerDelegate* delegate) override;
  void RequestCloseDisplay(int64_t display_id) override;
  display::ScreenBase* GetScreen() override;

  // Sample display information.
  Display display_;

  ScreenManagerDelegate* delegate_ = nullptr;

  std::unique_ptr<ScreenBase> screen_;

  base::WeakPtrFactory<ScreenManagerStubInternal> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(ScreenManagerStubInternal);
};

}  // namespace display

#endif  // SERVICES_UI_DISPLAY_SCREEN_MANAGER_STUB_INTERNAL_H_
