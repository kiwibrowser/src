// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASH_WM_PANELS_PANEL_WINDOW_RESIZER_H_
#define ASH_WM_PANELS_PANEL_WINDOW_RESIZER_H_

#include <memory>

#include "ash/ash_export.h"
#include "ash/wm/window_resizer.h"
#include "base/macros.h"
#include "base/memory/weak_ptr.h"

namespace gfx {
class Rect;
class Point;
}

namespace ash {

class PanelLayoutManager;

// PanelWindowResizer is used by ToplevelWindowEventFilter to handle dragging,
// moving or resizing panel window. These can be attached and detached from the
// launcher.
class ASH_EXPORT PanelWindowResizer : public WindowResizer {
 public:
  ~PanelWindowResizer() override;

  // Creates a new PanelWindowResizer. The caller takes ownership of the
  // returned object. The ownership of |next_window_resizer| is taken by the
  // returned object. Returns NULL if not resizable.
  static PanelWindowResizer* Create(WindowResizer* next_window_resizer,
                                    wm::WindowState* window_state);

  // WindowResizer:
  void Drag(const gfx::Point& location, int event_flags) override;
  void CompleteDrag() override;
  void RevertDrag() override;

 private:
  // Creates PanelWindowResizer that adds the ability to attach / detach panel
  // windows as well as reparenting them to the panel layer while dragging to
  // |next_window_resizer|. This object takes ownership of
  // |next_window_resizer|.
  PanelWindowResizer(WindowResizer* next_window_resizer,
                     wm::WindowState* window_state);

  // Checks if the provided window bounds should attach to the launcher. If true
  // the offset gives the necessary adjustment to snap to the launcher.
  bool AttachToLauncher(const gfx::Rect& bounds, gfx::Point* offset);

  // Tracks the panel's initial position and attachment at the start of a drag
  // and informs the PanelLayoutManager that a drag has started if necessary.
  void StartedDragging();

  // Informs the PanelLayoutManager that the drag is complete if it was informed
  // of the drag start.
  void FinishDragging();

  // Updates the dragged panel's index in the launcher.
  void UpdateLauncherPosition();

  // Returns the PanelLayoutManager associated with |panel_container_|.
  PanelLayoutManager* GetPanelLayoutManager();

  // Last pointer location in screen coordinates.
  gfx::Point last_location_;

  // Wraps a window resizer and adds panel detaching / reattaching and snapping
  // to launcher behavior during drags.
  std::unique_ptr<WindowResizer> next_window_resizer_;

  // Panel container window.
  aura::Window* panel_container_;
  aura::Window* initial_panel_container_;

  // Set to true once Drag() is invoked and the bounds of the window change.
  bool did_move_or_resize_;

  // True if the window started attached to the launcher.
  const bool was_attached_;

  base::WeakPtrFactory<PanelWindowResizer> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(PanelWindowResizer);
};

}  // namespace ash

#endif  // ASH_WM_PANELS_PANEL_WINDOW_RESIZER_H_
