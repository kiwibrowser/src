// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/views/frame/immersive_context_mus.h"

#include "ash/public/cpp/immersive/immersive_fullscreen_controller.h"
#include "ash/public/cpp/window_properties.h"
#include "ui/aura/client/aura_constants.h"
#include "ui/aura/window.h"
#include "ui/views/mus/mus_client.h"
#include "ui/views/mus/pointer_watcher_event_router.h"
#include "ui/views/pointer_watcher.h"
#include "ui/views/widget/desktop_aura/desktop_capture_client.h"

ImmersiveContextMus::ImmersiveContextMus() {}

ImmersiveContextMus::~ImmersiveContextMus() {}

void ImmersiveContextMus::InstallResizeHandleWindowTargeter(
    ash::ImmersiveFullscreenController* controller) {
  // There shouldn't be a need to do anything here, the windowmanager takes care
  // of this for us.
}

void ImmersiveContextMus::OnEnteringOrExitingImmersive(
    ash::ImmersiveFullscreenController* controller,
    bool entering) {
  aura::Window* window = controller->widget()->GetNativeWindow();
  // Auto hide the shelf in immersive fullscreen instead of hiding it.
  window->SetProperty(ash::kHideShelfWhenFullscreenKey, !entering);
  // Update the window's immersive mode state for the window manager.
  window->SetProperty(aura::client::kImmersiveFullscreenKey, entering);
}

gfx::Rect ImmersiveContextMus::GetDisplayBoundsInScreen(views::Widget* widget) {
  return widget->GetWindowBoundsInScreen();
}

void ImmersiveContextMus::AddPointerWatcher(
    views::PointerWatcher* watcher,
    views::PointerWatcherEventTypes events) {
  // TODO: http://crbug.com/641164
  views::MusClient::Get()->pointer_watcher_event_router()->AddPointerWatcher(
      watcher, events == views::PointerWatcherEventTypes::MOVES);
}

void ImmersiveContextMus::RemovePointerWatcher(views::PointerWatcher* watcher) {
  views::MusClient::Get()->pointer_watcher_event_router()->RemovePointerWatcher(
      watcher);
}

bool ImmersiveContextMus::DoesAnyWindowHaveCapture() {
  return views::DesktopCaptureClient::GetCaptureWindowGlobal() != nullptr;
}

bool ImmersiveContextMus::IsMouseEventsEnabled() {
  // TODO: http://crbug.com/640374.
  NOTIMPLEMENTED();
  return true;
}
