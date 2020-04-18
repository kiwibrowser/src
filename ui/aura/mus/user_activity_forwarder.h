// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_AURA_MUS_USER_ACTIVITY_FORWARDER_H_
#define UI_AURA_MUS_USER_ACTIVITY_FORWARDER_H_

#include "base/compiler_specific.h"
#include "base/macros.h"
#include "mojo/public/cpp/bindings/binding.h"
#include "services/ui/public/interfaces/user_activity_monitor.mojom.h"
#include "ui/aura/aura_export.h"

namespace ui {
class UserActivityDetector;
}

namespace aura {

// Helper class that receives reports of user activity from the mojo
// UserActivityMonitor interface (in the window server) and forwards them to
// ui::UserActivityDetector (usually in ash).
// TODO(derat): Make current ui::UserActivityObserver implementations (i.e.
// downstream of ui::UserActivityDetector) instead observe UserActivityMonitor
// directly: http://crbug.com/626899
class AURA_EXPORT UserActivityForwarder
    : public ui::mojom::UserActivityObserver {
 public:
  UserActivityForwarder(ui::mojom::UserActivityMonitorPtr monitor,
                        ui::UserActivityDetector* detector);
  ~UserActivityForwarder() override;

 private:
  // ui::mojom::UserActivityObserver:
  void OnUserActivity() override;

  ui::mojom::UserActivityMonitorPtr monitor_;
  mojo::Binding<ui::mojom::UserActivityObserver> binding_;

  ui::UserActivityDetector* detector_;  // Not owned.

  DISALLOW_COPY_AND_ASSIGN(UserActivityForwarder);
};

}  // namespace aura

#endif  // UI_AURA_MUS_USER_ACTIVITY_FORWARDER_H_
