// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASH_COMPONENTS_SHORTCUT_VIEWER_SHORTCUT_VIEWER_APPLICATION_H_
#define ASH_COMPONENTS_SHORTCUT_VIEWER_SHORTCUT_VIEWER_APPLICATION_H_

#include "base/macros.h"
#include "services/service_manager/public/cpp/service.h"
#include "ui/events/devices/input_device_event_observer.h"

namespace views {
class AuraInit;
}  // namespace views

namespace keyboard_shortcut_viewer {

// A mojo application that shows the keyboard shortcut viewer window.
class ShortcutViewerApplication : public service_manager::Service,
                                  public ui::InputDeviceEventObserver {
 public:
  ShortcutViewerApplication();
  ~ShortcutViewerApplication() override;

 private:
  // service_manager::Service:
  void OnStart() override;

  // ui::InputDeviceEventObserver:
  void OnDeviceListsComplete() override;

  std::unique_ptr<views::AuraInit> aura_init_;

  DISALLOW_COPY_AND_ASSIGN(ShortcutViewerApplication);
};

}  // namespace keyboard_shortcut_viewer

#endif  // ASH_COMPONENTS_SHORTCUT_VIEWER_SHORTCUT_VIEWER_APPLICATION_H_
