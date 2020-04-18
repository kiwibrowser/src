// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_WEB_CONTENTS_AURA_OVERSCROLL_WINDOW_ANIMATION_H_
#define CONTENT_BROWSER_WEB_CONTENTS_AURA_OVERSCROLL_WINDOW_ANIMATION_H_

#include <memory>

#include "base/macros.h"
#include "content/browser/renderer_host/overscroll_controller_delegate.h"
#include "content/common/content_export.h"
#include "ui/compositor/layer_animation_observer.h"

namespace aura {
class Window;
}

namespace ui {
class Layer;
}

namespace content {

class ShadowLayerDelegate;

// Manages the animation of a window sliding on top or behind another one. The
// main window, which is the one displayed before the animation starts, is not
// owned by OverscrollWindowAnimation, while the slide window, created at the
// start of the animation, is owned by us for its duration.
class CONTENT_EXPORT OverscrollWindowAnimation
    : public OverscrollControllerDelegate,
      ui::ImplicitAnimationObserver {
 public:
  // The direction of this animation. SLIDE_FRONT indicates that the slide
  // window moves on top of the main window, entering the screen from the right.
  // SLIDE_BACK means that the main window is animated to the right, revealing
  // the slide window in the back. SLIDE_NONE means we are not animating yet.
  // Both windows are animated at the same time but at different speeds,
  // creating a parallax scrolling effect. Left and right are reversed for RTL
  // languages, but stack order remains unchanged.
  enum Direction { SLIDE_FRONT, SLIDE_BACK, SLIDE_NONE };

  // Delegate class that interfaces with the window animation.
  class CONTENT_EXPORT Delegate {
   public:
    virtual ~Delegate() {}

    // Create a slide window with the given |bounds| relative to its parent.
    virtual std::unique_ptr<aura::Window> CreateFrontWindow(
        const gfx::Rect& bounds) = 0;
    virtual std::unique_ptr<aura::Window> CreateBackWindow(
        const gfx::Rect& bounds) = 0;

    // Returns the main window that participates in the animation. The delegate
    // does not own this window.
    virtual aura::Window* GetMainWindow() const = 0;

    // Called when we know the animation is going to complete successfully, but
    // before it actually completes.
    virtual void OnOverscrollCompleting() = 0;

    // Called when the animation has been completed. The slide window is
    // transferred to the delegate.
    virtual void OnOverscrollCompleted(
        std::unique_ptr<aura::Window> window) = 0;

    // Called when the overscroll gesture has been cancelled, after the cancel
    // animation finishes.
    virtual void OnOverscrollCancelled() = 0;
  };

  explicit OverscrollWindowAnimation(Delegate* delegate);

  ~OverscrollWindowAnimation() override;

  // Returns true if we are currently animating.
  bool is_active() const { return !!slide_window_; }

  OverscrollSource overscroll_source() { return overscroll_source_; }

  void SetOverscrollSourceForTesting(OverscrollSource source) {
    overscroll_source_ = source;
  }

  // OverscrollControllerDelegate:
  gfx::Size GetDisplaySize() const override;
  bool OnOverscrollUpdate(float delta_x, float delta_y) override;
  void OnOverscrollComplete(OverscrollMode overscroll_mode) override;
  void OnOverscrollModeChange(OverscrollMode old_mode,
                              OverscrollMode new_mode,
                              OverscrollSource source,
                              cc::OverscrollBehavior behavior) override;
  base::Optional<float> GetMaxOverscrollDelta() const override;

 private:
  // Cancels the slide, animating the front and back window to their original
  // positions.
  void CancelSlide();

  // Returns a translation on the x axis for the given overscroll.
  float GetTranslationForOverscroll(float delta_x);

  // Animates a translation of the given |layer|. If |listen_for_completion| is
  // true, adds |this| as observer of the animation.
  void AnimateTranslation(ui::Layer* layer,
                          float translate_x,
                          bool listen_for_completion);

  // Return the front/back layer that is involved in the animation. The caller
  // does not own it.
  ui::Layer* GetFrontLayer() const;
  ui::Layer* GetBackLayer() const;

  // Returns the size of the content window.
  gfx::Size GetContentSize() const;

  // ui::ImplicitAnimationObserver:
  void OnImplicitAnimationsCompleted() override;

  // We own the window created for the animation.
  std::unique_ptr<aura::Window> slide_window_;

  // Shadow shown under the animated layer.
  std::unique_ptr<ShadowLayerDelegate> shadow_;

  // Delegate that provides the animation target and is notified of the
  // animation state.
  Delegate* delegate_;

  // The current animation direction.
  Direction direction_;

  // OverscrollSource of the current overscroll gesture. Updated when the new
  // overscroll gesture starts, before CreateFront/BackWindow callback is called
  // on the delegate.
  OverscrollSource overscroll_source_ = OverscrollSource::NONE;

  // Indicates if the current slide has been cancelled. True while the cancel
  // animation is in progress.
  bool overscroll_cancelled_;

  DISALLOW_COPY_AND_ASSIGN(OverscrollWindowAnimation);
};

}  // namespace content

#endif  // CONTENT_BROWSER_WEB_CONTENTS_AURA_OVERSCROLL_WINDOW_ANIMATION_H_
