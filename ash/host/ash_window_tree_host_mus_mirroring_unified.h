// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASH_HOST_ASH_WINDOW_TREE_HOST_MUS_MIRRORING_UNIFIED_H_
#define ASH_HOST_ASH_WINDOW_TREE_HOST_MUS_MIRRORING_UNIFIED_H_

#include "ash/host/ash_window_tree_host_mus.h"
#include "base/macros.h"
#include "ui/aura/mus/window_tree_host_mus_init_params.h"

namespace ash {

class AshWindowTreeHostMirroringDelegate;

// A WTH for the displays mirroring the unified desktop in mus without viz.
// This is a near copy of AshWindowTreeHostMirroringUnified.
class AshWindowTreeHostMusMirroringUnified : public AshWindowTreeHostMus {
 public:
  AshWindowTreeHostMusMirroringUnified(
      aura::WindowTreeHostMusInitParams init_params,
      int64_t mirroring_display_id,
      AshWindowTreeHostMirroringDelegate* delegate);
  ~AshWindowTreeHostMusMirroringUnified() override;

  // aura::WindowTreeHost:
  gfx::Transform GetRootTransformForLocalEventCoordinates() const override;
  void ConvertDIPToPixels(gfx::Point* point) const override;
  void ConvertPixelsToDIP(gfx::Point* point) const override;

  // ash::AshWindowTreeHostMus:
  void PrepareForShutdown() override;

 private:
  int64_t mirroring_display_id_;

  AshWindowTreeHostMirroringDelegate* delegate_;  // Not owned.

  bool is_shutting_down_ = false;

  DISALLOW_COPY_AND_ASSIGN(AshWindowTreeHostMusMirroringUnified);
};

}  // namespace ash

#endif  // ASH_HOST_ASH_WINDOW_TREE_HOST_MUS_MIRRORING_UNIFIED_H_
