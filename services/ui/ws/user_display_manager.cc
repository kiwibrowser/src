// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/ui/ws/user_display_manager.h"

#include <utility>

#include "services/ui/display/screen_manager.h"
#include "services/ui/ws/user_display_manager_delegate.h"
#include "ui/display/display.h"
#include "ui/display/screen_base.h"
#include "ui/display/types/display_constants.h"

namespace ui {
namespace ws {

UserDisplayManager::UserDisplayManager(UserDisplayManagerDelegate* delegate)
    : delegate_(delegate),
      got_valid_frame_decorations_(delegate->GetFrameDecorations(nullptr)) {}

UserDisplayManager::~UserDisplayManager() {}

void UserDisplayManager::DisableAutomaticNotification() {
  DCHECK(notify_automatically_);
  notify_automatically_ = false;
}

void UserDisplayManager::CallOnDisplaysChanged() {
  observers_.ForAllPtrs([this](mojom::ScreenProviderObserver* observer) {
    CallOnDisplaysChanged(observer);
  });
}

void UserDisplayManager::OnFrameDecorationValuesChanged() {
  got_valid_frame_decorations_ = true;
  CallOnDisplaysChangedIfNecessary();
}

void UserDisplayManager::AddDisplayManagerBinding(
    mojo::InterfaceRequest<mojom::ScreenProvider> request) {
  bindings_.AddBinding(this, std::move(request));
}

void UserDisplayManager::OnDisplayUpdated(const display::Display& display) {
  CallOnDisplaysChangedIfNecessary();
}

void UserDisplayManager::OnDisplayDestroyed(int64_t display_id) {
  CallOnDisplaysChangedIfNecessary();
}

void UserDisplayManager::OnPrimaryDisplayChanged(int64_t primary_display_id) {
  CallOnDisplaysChangedIfNecessary();
}

void UserDisplayManager::AddObserver(
    mojom::ScreenProviderObserverPtr observer) {
  mojom::ScreenProviderObserver* observer_impl = observer.get();
  observers_.AddPtr(std::move(observer));
  OnObserverAdded(observer_impl);
}

void UserDisplayManager::OnObserverAdded(
    mojom::ScreenProviderObserver* observer) {
  // Many clients key off the frame decorations to size widgets. Wait for frame
  // decorations before notifying so that we don't have to worry about clients
  // resizing appropriately.
  if (!ShouldCallOnDisplaysChanged())
    return;

  CallOnDisplaysChanged(observer);
}

mojom::WsDisplayPtr UserDisplayManager::ToWsDisplayPtr(
    const display::Display& display) {
  mojom::WsDisplayPtr ws_display = mojom::WsDisplay::New();
  ws_display->display = display;
  delegate_->GetFrameDecorations(&ws_display->frame_decoration_values);
  return ws_display;
}

std::vector<mojom::WsDisplayPtr> UserDisplayManager::GetAllDisplays() {
  const auto& displays =
      display::ScreenManager::GetInstance()->GetScreen()->GetAllDisplays();

  std::vector<mojom::WsDisplayPtr> ws_display;
  ws_display.reserve(displays.size());

  for (const auto& display : displays)
    ws_display.push_back(ToWsDisplayPtr(display));

  return ws_display;
}

bool UserDisplayManager::ShouldCallOnDisplaysChanged() const {
  return got_valid_frame_decorations_ && !display::ScreenManager::GetInstance()
                                              ->GetScreen()
                                              ->GetAllDisplays()
                                              .empty();
}

void UserDisplayManager::CallOnDisplaysChangedIfNecessary() {
  if (!notify_automatically_ || !ShouldCallOnDisplaysChanged())
    return;

  CallOnDisplaysChanged();
}

void UserDisplayManager::CallOnDisplaysChanged(
    mojom::ScreenProviderObserver* observer) {
  observer->OnDisplaysChanged(GetAllDisplays(),
                              display::ScreenManager::GetInstance()
                                  ->GetScreen()
                                  ->GetPrimaryDisplay()
                                  .id(),
                              delegate_->GetInternalDisplayId());
}

}  // namespace ws
}  // namespace ui
