// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/wm/core/transient_window_controller.h"

#include "ui/aura/client/transient_window_client_observer.h"
#include "ui/wm/core/transient_window_manager.h"

namespace wm {

// static
TransientWindowController* TransientWindowController::instance_ = nullptr;

TransientWindowController::TransientWindowController() {
  DCHECK(!instance_);
  instance_ = this;
}

TransientWindowController::~TransientWindowController() {
  DCHECK_EQ(instance_, this);
  instance_ = nullptr;
}

void TransientWindowController::AddTransientChild(aura::Window* parent,
                                                  aura::Window* child) {
  TransientWindowManager::GetOrCreate(parent)->AddTransientChild(child);
}

void TransientWindowController::RemoveTransientChild(aura::Window* parent,
                                                     aura::Window* child) {
  TransientWindowManager::GetOrCreate(parent)->RemoveTransientChild(child);
}

aura::Window* TransientWindowController::GetTransientParent(
    aura::Window* window) {
  return const_cast<aura::Window*>(GetTransientParent(
      const_cast<const aura::Window*>(window)));
}

const aura::Window* TransientWindowController::GetTransientParent(
    const aura::Window* window) {
  const TransientWindowManager* window_manager =
      TransientWindowManager::GetIfExists(window);
  return window_manager ? window_manager->transient_parent() : nullptr;
}

void TransientWindowController::AddObserver(
    aura::client::TransientWindowClientObserver* observer) {
  observers_.AddObserver(observer);
}

void TransientWindowController::RemoveObserver(
    aura::client::TransientWindowClientObserver* observer) {
  observers_.RemoveObserver(observer);
}

}  // namespace wm
