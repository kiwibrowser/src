// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASH_HOST_ASH_WINDOW_TREE_HOST_MUS_H_
#define ASH_HOST_ASH_WINDOW_TREE_HOST_MUS_H_

#include "ash/host/ash_window_tree_host.h"
#include "ui/aura/mus/window_tree_host_mus.h"

namespace ash {

class TransformerHelper;

// Implementation of AshWindowTreeHost for mus/mash.
class AshWindowTreeHostMus : public AshWindowTreeHost,
                             public aura::WindowTreeHostMus {
 public:
  explicit AshWindowTreeHostMus(aura::WindowTreeHostMusInitParams init_params);
  ~AshWindowTreeHostMus() override;

  // AshWindowTreeHost:
  void ConfineCursorToRootWindow() override;
  void ConfineCursorToBoundsInRoot(const gfx::Rect& bounds_in_root) override;
  gfx::Rect GetLastCursorConfineBoundsInPixels() const override;
  void SetRootWindowTransformer(
      std::unique_ptr<RootWindowTransformer> transformer) override;
  gfx::Insets GetHostInsets() const override;
  aura::WindowTreeHost* AsWindowTreeHost() override;
  void PrepareForShutdown() override;
  void RegisterMirroringHost(AshWindowTreeHost* mirroring_ash_host) override;
  void SetCursorConfig(const display::Display& display,
                       display::Display::Rotation rotation) override;
  void ClearCursorConfig() override;

  // aura::WindowTreeHostMus:
  void SetRootTransform(const gfx::Transform& transform) override;
  gfx::Transform GetRootTransform() const override;
  gfx::Transform GetInverseRootTransform() const override;
  gfx::Rect GetTransformedRootWindowBoundsInPixels(
      const gfx::Size& host_size_in_pixels) const override;
  void OnCursorVisibilityChangedNative(bool show) override;

 private:
  std::unique_ptr<TransformerHelper> transformer_helper_;

  gfx::Rect last_cursor_confine_bounds_in_pixels_;

  DISALLOW_COPY_AND_ASSIGN(AshWindowTreeHostMus);
};

}  // namespace ash

#endif  // ASH_HOST_ASH_WINDOW_TREE_HOST_MUS_H_
