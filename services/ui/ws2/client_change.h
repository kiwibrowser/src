// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_UI_WS2_CLIENT_CHANGE_H_
#define SERVICES_UI_WS2_CLIENT_CHANGE_H_

#include <stdint.h>

#include <memory>

#include "base/component_export.h"
#include "base/macros.h"
#include "ui/aura/window_tracker.h"

namespace aura {
class Window;
}

namespace ui {
namespace ws2 {

class ClientChangeTracker;

// Describes the type of the change. Maps to the incoming change from the
// client.
enum class ClientChangeType {
  // Used for WindowTree::SetWindowBounds().
  kBounds,
  // Used for WindowTree::SetCapture() and WindowTree::ReleaseCapture().
  kCapture,
  // Used for WindowTree::SetFocus().
  kFocus,
  // Used for WindowTree::SetWindowProperty().
  kProperty,
};

// ClientChange represents an incoming request from a WindowTreeClient. For
// example, SetWindowBounds() is a request to change the kBounds property of
// the window.
class COMPONENT_EXPORT(WINDOW_SERVICE) ClientChange {
 public:
  ClientChange(ClientChangeTracker* tracker,
               aura::Window* window,
               ClientChangeType type);
  ~ClientChange();

  // The window the changes associated with. Is null if the window has been
  // destroyed during processing.
  aura::Window* window() {
    return !window_tracker_.windows().empty() ? window_tracker_.windows()[0]
                                              : nullptr;
  }

  ClientChangeType type() const { return type_; }

 private:
  ClientChangeTracker* tracker_;
  aura::WindowTracker window_tracker_;
  const ClientChangeType type_;

  DISALLOW_COPY_AND_ASSIGN(ClientChange);
};

}  // namespace ws2
}  // namespace ui

#endif  // SERVICES_UI_WS2_CLIENT_CHANGE_H_
