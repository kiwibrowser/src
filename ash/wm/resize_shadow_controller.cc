// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/wm/resize_shadow_controller.h"

#include <memory>
#include <utility>

#include "ash/wm/resize_shadow.h"
#include "ui/aura/window.h"

namespace ash {

ResizeShadowController::ResizeShadowController() = default;

ResizeShadowController::~ResizeShadowController() {
  for (const auto& shadow : window_shadows_)
    shadow.first->RemoveObserver(this);
}

void ResizeShadowController::ShowShadow(aura::Window* window, int hit_test) {
  ResizeShadow* shadow = GetShadowForWindow(window);
  if (!shadow)
    shadow = CreateShadow(window);
  shadow->ShowForHitTest(hit_test);
}

void ResizeShadowController::HideShadow(aura::Window* window) {
  ResizeShadow* shadow = GetShadowForWindow(window);
  if (shadow)
    shadow->Hide();
}

ResizeShadow* ResizeShadowController::GetShadowForWindowForTest(
    aura::Window* window) {
  return GetShadowForWindow(window);
}

void ResizeShadowController::OnWindowDestroying(aura::Window* window) {
  window_shadows_.erase(window);
}

void ResizeShadowController::OnWindowVisibilityChanging(aura::Window* window,
                                                        bool visible) {
  if (!visible)
    HideShadow(window);
}

ResizeShadow* ResizeShadowController::CreateShadow(aura::Window* window) {
  auto shadow = std::make_unique<ResizeShadow>(window);
  window->AddObserver(this);

  ResizeShadow* raw_shadow = shadow.get();
  window_shadows_.insert(std::make_pair(window, std::move(shadow)));
  return raw_shadow;
}

ResizeShadow* ResizeShadowController::GetShadowForWindow(aura::Window* window) {
  auto it = window_shadows_.find(window);
  return it != window_shadows_.end() ? it->second.get() : nullptr;
}

}  // namespace ash
