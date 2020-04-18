// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_UI_WS2_FOCUS_HANDLER_H_
#define SERVICES_UI_WS2_FOCUS_HANDLER_H_

#include "base/macros.h"
#include "ui/aura/client/focus_change_observer.h"

namespace aura {
class Window;
}

namespace ui {
namespace ws2 {

class ClientWindow;
class WindowServiceClient;

// FocusHandler handles focus requests from the client, as well as notifying
// the client when focus changes.
class FocusHandler : public aura::client::FocusChangeObserver {
 public:
  explicit FocusHandler(WindowServiceClient* window_service_client);
  ~FocusHandler() override;

  // Sets focus to |window| (which may be null). Returns true on success.
  bool SetFocus(aura::Window* window);

  // Sets whether |window| can be focused.
  void SetCanFocus(aura::Window* window, bool can_focus);

 private:
  // Returns true if |window| can be focused.
  bool IsFocusableWindow(aura::Window* window) const;

  bool IsEmbeddedClient(ClientWindow* client_window) const;
  bool IsOwningClient(ClientWindow* client_window) const;

  // aura::client::FocusChangeObserver:
  void OnWindowFocused(aura::Window* gained_focus,
                       aura::Window* lost_focus) override;

  WindowServiceClient* window_service_client_;

  DISALLOW_COPY_AND_ASSIGN(FocusHandler);
};

}  // namespace ws2
}  // namespace ui

#endif  // SERVICES_UI_WS2_FOCUS_HANDLER_H_
