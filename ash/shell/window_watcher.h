// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASH_SHELL_WINDOW_WATCHER_H_
#define ASH_SHELL_WINDOW_WATCHER_H_

#include <stdint.h>

#include <map>
#include <memory>

#include "ash/public/cpp/shelf_types.h"
#include "ash/shell_observer.h"
#include "base/logging.h"
#include "base/macros.h"
#include "ui/aura/window_observer.h"

namespace ash {
namespace shell {

// WindowWatcher is responsible for listening for newly created windows and
// creating items on the Shelf for them.
class WindowWatcher : public aura::WindowObserver, public ShellObserver {
 public:
  WindowWatcher();
  ~WindowWatcher() override;

  aura::Window* GetWindowByID(const ShelfID& id);

  // aura::WindowObserver overrides:
  void OnWindowAdded(aura::Window* new_window) override;
  void OnWillRemoveWindow(aura::Window* window) override;

  // ShellObserver:
  void OnRootWindowAdded(aura::Window* root_window) override;

 private:
  class WorkspaceWindowWatcher;

  typedef std::map<ShelfID, aura::Window*> IDToWindow;

  // Maps from window to the id we gave it.
  IDToWindow id_to_window_;

  std::unique_ptr<WorkspaceWindowWatcher> workspace_window_watcher_;

  DISALLOW_COPY_AND_ASSIGN(WindowWatcher);
};

}  // namespace shell
}  // namespace ash

#endif  // ASH_SHELL_WINDOW_WATCHER_H_
