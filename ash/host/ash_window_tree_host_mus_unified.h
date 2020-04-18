// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASH_HOST_ASH_WINDOW_TREE_HOST_MUS_UNIFIED_H_
#define ASH_HOST_ASH_WINDOW_TREE_HOST_MUS_UNIFIED_H_

#include <vector>

#include "ash/host/ash_window_tree_host_mus.h"
#include "base/macros.h"
#include "ui/aura/mus/window_tree_host_mus_init_params.h"
#include "ui/aura/window_observer.h"

namespace ash {

class AshWindowTreeHostMirroringDelegate;

// A WTH for the unified desktop display in mus without viz.
// This is a near copy of AshWindowTreeHostUnified.
class AshWindowTreeHostMusUnified : public AshWindowTreeHostMus,
                                    public aura::WindowObserver {
 public:
  AshWindowTreeHostMusUnified(aura::WindowTreeHostMusInitParams init_params,
                              AshWindowTreeHostMirroringDelegate* delegate);
  ~AshWindowTreeHostMusUnified() override;

 private:
  // AshWindowTreeHost:
  void PrepareForShutdown() override;
  void RegisterMirroringHost(AshWindowTreeHost* mirroring_ash_host) override;

  // aura::WindowTreeHost:
  void SetBoundsInPixels(const gfx::Rect& bounds,
                         const viz::LocalSurfaceId& local_surface_id) override;
  void SetCursorNative(gfx::NativeCursor cursor) override;
  void OnCursorVisibilityChangedNative(bool show) override;

  // ui::PlatformWindowDelegate:
  void OnBoundsChanged(const gfx::Rect& bounds) override;

  // aura::WindowObserver:
  void OnWindowDestroying(aura::Window* window) override;

  AshWindowTreeHostMirroringDelegate* delegate_;  // Not owned.

  std::vector<AshWindowTreeHost*> mirroring_hosts_;

  DISALLOW_COPY_AND_ASSIGN(AshWindowTreeHostMusUnified);
};

}  // namespace ash

#endif  // ASH_HOST_ASH_WINDOW_TREE_HOST_MUS_UNIFIED_H_
