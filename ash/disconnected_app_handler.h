// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASH_DISCONNECTED_APP_HANDLER_H_
#define ASH_DISCONNECTED_APP_HANDLER_H_

#include "base/macros.h"
#include "ui/aura/window_observer.h"

namespace ash {

// DisconnectedAppHandler is associated with a single aura Window and deletes
// the window when the embedded app is disconnected. This is intended to be used
// for windows created at the request of client apps. This is only used in mash.
class DisconnectedAppHandler : public aura::WindowObserver {
 public:
  // Public for WindowProperty.
  ~DisconnectedAppHandler() override;

  // Creates a new DisconnectedAppHandler associated with |window|.
  // DisconnectedAppHandler is owned by the window.
  static void Create(aura::Window* window);

 private:
  explicit DisconnectedAppHandler(aura::Window* root_window);

  // aura::WindowObserver:
  void OnEmbeddedAppDisconnected(aura::Window* window) override;

  aura::Window* window_;

  DISALLOW_COPY_AND_ASSIGN(DisconnectedAppHandler);
};

}  // namespace ash

#endif  // ASH_DISCONNECTED_APP_HANDLER_H_
