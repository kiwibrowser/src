// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_UI_WS2_CLIENT_CHANGE_TRACKER_H_
#define SERVICES_UI_WS2_CLIENT_CHANGE_TRACKER_H_

#include <stdint.h>

#include <memory>

#include "base/component_export.h"
#include "base/macros.h"

namespace aura {
class Window;
}

namespace ui {
namespace ws2 {

class ClientChange;

enum class ClientChangeType;

// Tracks the active change from the client. There is at most one change from a
// client at a time.
class COMPONENT_EXPORT(WINDOW_SERVICE) ClientChangeTracker {
 public:
  ClientChangeTracker();
  ~ClientChangeTracker();

  bool IsProcessingChangeForWindow(aura::Window* window, ClientChangeType type);

 private:
  friend class ClientChange;

  // Owned by the caller that created the ClientChange. This is set in
  // ClientChange's constructor and reset in the destructor.
  ClientChange* current_change_ = nullptr;

  DISALLOW_COPY_AND_ASSIGN(ClientChangeTracker);
};

}  // namespace ws2
}  // namespace ui

#endif  // SERVICES_UI_WS2_CLIENT_CHANGE_TRACKER_H_
