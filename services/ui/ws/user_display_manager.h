// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_UI_WS_USER_DISPLAY_MANAGER_H_
#define SERVICES_UI_WS_USER_DISPLAY_MANAGER_H_

#include <vector>

#include "base/macros.h"
#include "mojo/public/cpp/bindings/binding_set.h"
#include "mojo/public/cpp/bindings/interface_ptr_set.h"
#include "services/ui/public/interfaces/screen_provider.mojom.h"

namespace display {
class Display;
}

namespace ui {
namespace ws {

class UserDisplayManagerDelegate;

// Provides per user display state.
// TODO(sky): merge this with ui::ws::DisplayManager (and rename).
class UserDisplayManager : public mojom::ScreenProvider {
 public:
  UserDisplayManager(UserDisplayManagerDelegate* delegate);
  ~UserDisplayManager() override;

  void DisableAutomaticNotification();

  // Unconditionally calls OnDisplayChanged() on observers.
  void CallOnDisplaysChanged();

  // Called when the frame decorations for this user change.
  void OnFrameDecorationValuesChanged();

  void AddDisplayManagerBinding(
      mojo::InterfaceRequest<mojom::ScreenProvider> request);

  // Called when something about the display (e.g. pixel-ratio, size) changes.
  void OnDisplayUpdated(const display::Display& display);

  // Called when the display with |display_id| was removed.
  void OnDisplayDestroyed(int64_t display_id);

  // Called when the primary display changes.
  void OnPrimaryDisplayChanged(int64_t primary_display_id);

  // mojom::ScreenProvider:
  void AddObserver(mojom::ScreenProviderObserverPtr observer) override;

 private:
  // Called when a new observer is added. If frame decorations are available
  // notifies the observer immediately.
  void OnObserverAdded(mojom::ScreenProviderObserver* observer);

  // Fills in a WsDisplayPtr for |display|.
  mojom::WsDisplayPtr ToWsDisplayPtr(const display::Display& display);

  std::vector<mojom::WsDisplayPtr> GetAllDisplays();

  bool ShouldCallOnDisplaysChanged() const;

  // Calls OnDisplaysChanged() on all observers.
  void CallOnDisplaysChangedIfNecessary();

  // Calls OnDisplaysChanged() on |observer|.
  void CallOnDisplaysChanged(mojom::ScreenProviderObserver* observer);

  UserDisplayManagerDelegate* delegate_;

  // Set to true the first time at least one Display has valid frame values.
  bool got_valid_frame_decorations_;

  mojo::BindingSet<mojom::ScreenProvider> bindings_;

  // WARNING: only use these once |got_valid_frame_decorations_| is true.
  mojo::InterfacePtrSet<mojom::ScreenProviderObserver> observers_;

  // If true DisplayManagerObservers are notified any time there is a display
  // change. If false, observers are only notified when CallOnDisplaysChanged()
  // is explicitly called. This value is true in automatic display creation and
  // false when in manual mode.
  bool notify_automatically_ = true;

  DISALLOW_COPY_AND_ASSIGN(UserDisplayManager);
};

}  // namespace ws
}  // namespace ui

#endif  // SERVICES_UI_WS_USER_DISPLAY_MANAGER_H_
