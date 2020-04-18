// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_EXO_SHELL_SURFACE_H_
#define COMPONENTS_EXO_SHELL_SURFACE_H_

#include "ash/wm/window_state_observer.h"
#include "base/macros.h"
#include "components/exo/shell_surface_base.h"

namespace exo {
class Surface;

// This class implements toplevel surface for which position and state are
// managed by the shell.
class ShellSurface : public ShellSurfaceBase,
                     public ash::wm::WindowStateObserver {
 public:
  // The |origin| is the initial position in screen coordinates. The position
  // specified as part of the geometry is relative to the shell surface.
  ShellSurface(Surface* surface,
               const gfx::Point& origin,
               bool activatable,
               bool can_minimize,
               int container);
  explicit ShellSurface(Surface* surface);
  ~ShellSurface() override;

  // Set the "parent" of this surface. This window should be stacked above a
  // parent.
  void SetParent(ShellSurface* parent);

  // Maximizes the shell surface.
  void Maximize();

  // Minimize the shell surface.
  void Minimize();

  // Restore the shell surface.
  void Restore();

  // Set fullscreen state for shell surface.
  void SetFullscreen(bool fullscreen);

  // Start an interactive resize of surface. |component| is one of the windows
  // HT constants (see ui/base/hit_test.h) and describes in what direction the
  // surface should be resized.
  void Resize(int component);

  // Overridden from ShellSurfaceBase:
  void InitializeWindowState(ash::wm::WindowState* window_state) override;

  // Overridden from ash::wm::WindowStateObserver:
  void OnPreWindowStateTypeChange(
      ash::wm::WindowState* window_state,
      ash::mojom::WindowStateType old_type) override;
  void OnPostWindowStateTypeChange(
      ash::wm::WindowState* window_state,
      ash::mojom::WindowStateType old_type) override;

 private:
  class ScopedAnimationsDisabled;

  std::unique_ptr<ScopedAnimationsDisabled> scoped_animations_disabled_;

  DISALLOW_COPY_AND_ASSIGN(ShellSurface);
};

}  // namespace exo

#endif  // COMPONENTS_EXO_SHELL_SURFACE_H_
