// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASH_WM_ALWAYS_ON_TOP_CONTROLLER_H_
#define ASH_WM_ALWAYS_ON_TOP_CONTROLLER_H_

#include <memory>

#include "ash/ash_export.h"
#include "base/macros.h"
#include "ui/aura/window_observer.h"

namespace ash {

class WorkspaceLayoutManager;

// AlwaysOnTopController puts window into proper containers based on its
// 'AlwaysOnTop' property. That is, putting a window into the worskpace
// container if its "AlwaysOnTop" property is false. Otherwise, put it in
// |always_on_top_container_|.
class ASH_EXPORT AlwaysOnTopController : public aura::WindowObserver {
 public:
  explicit AlwaysOnTopController(aura::Window* viewport);
  ~AlwaysOnTopController() override;

  // Gets container for given |window| based on its "AlwaysOnTop" property.
  aura::Window* GetContainer(aura::Window* window) const;

  WorkspaceLayoutManager* GetLayoutManager() const;

  void SetLayoutManagerForTest(
      std::unique_ptr<WorkspaceLayoutManager> layout_manager);

 private:
  // Overridden from aura::WindowObserver:
  void OnWindowHierarchyChanged(const HierarchyChangeParams& params) override;
  void OnWindowPropertyChanged(aura::Window* window,
                               const void* key,
                               intptr_t old) override;
  void OnWindowDestroying(aura::Window* window) override;

  aura::Window* always_on_top_container_;

  DISALLOW_COPY_AND_ASSIGN(AlwaysOnTopController);
};

}  // namepsace ash

#endif  // ASH_WM_ALWAYS_ON_TOP_CONTROLLER_H_
