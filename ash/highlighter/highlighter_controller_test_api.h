// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASH_HIGHLIGHTER_HIGHLIGHTER_CONTROLLER_TEST_API_H_
#define ASH_HIGHLIGHTER_HIGHLIGHTER_CONTROLLER_TEST_API_H_

#include "ash/public/interfaces/highlighter_controller.mojom.h"
#include "base/macros.h"
#include "mojo/public/cpp/bindings/binding.h"
#include "ui/gfx/geometry/rect.h"

namespace fast_ink {
class FastInkPoints;
}

namespace ash {

class HighlighterController;

// An api for testing the HighlighterController class.
// Implements ash::mojom::HighlighterControllerClient and binds itself as the
// client to provide the tests with access to gesture recognition results.
class HighlighterControllerTestApi
    : public ash::mojom::HighlighterControllerClient {
 public:
  explicit HighlighterControllerTestApi(HighlighterController* instance);
  ~HighlighterControllerTestApi() override;

  // Attaches itself as the client to the controller. This method is called
  // automatically from the constructor, and should be explicitly called only
  // if DetachClient has been called previously.
  void AttachClient();

  // Detaches itself from the controller.
  void DetachClient();

  void SetEnabled(bool enabled);
  void DestroyPointerView();
  void SimulateInterruptedStrokeTimeout();
  bool IsShowingHighlighter() const;
  bool IsFadingAway() const;
  bool IsWaitingToResumeStroke() const;
  bool IsShowingSelectionResult() const;
  const fast_ink::FastInkPoints& points() const;
  const fast_ink::FastInkPoints& predicted_points() const;

  void ResetEnabledState() { handle_enabled_state_changed_called_ = false; }
  // Flushes the mojo connection, then checks whether HandleEnabledStateChange
  // has been called on the client since the last call to ResetEnabledState.
  bool HandleEnabledStateChangedCalled();
  bool enabled() const { return enabled_; }

  void ResetSelection() { handle_selection_called_ = false; }
  // Flushes the mojo connection, then checks whether HandleSelection
  // has been called on the client since the last call to ResetSelection.
  bool HandleSelectionCalled();
  const gfx::Rect& selection() const { return selection_; }

 private:
  // HighlighterSelectionObserver:
  void HandleSelection(const gfx::Rect& rect) override;
  void HandleEnabledStateChange(bool enabled) override;

  // Binds to the client interface.
  mojo::Binding<ash::mojom::HighlighterControllerClient> binding_;

  // HighlighterController interface.
  ash::mojom::HighlighterControllerPtr highlighter_controller_;

  HighlighterController* instance_;

  bool handle_selection_called_ = false;
  bool handle_enabled_state_changed_called_ = false;
  gfx::Rect selection_;
  bool enabled_ = false;

  DISALLOW_COPY_AND_ASSIGN(HighlighterControllerTestApi);
};

}  // namespace ash

#endif  // ASH_HIGHLIGHTER_HIGHLIGHTER_CONTROLLER_TEST_API_H_
