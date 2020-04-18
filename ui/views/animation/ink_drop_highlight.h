// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_VIEWS_ANIMATION_INK_DROP_HIGHLIGHT_H_
#define UI_VIEWS_ANIMATION_INK_DROP_HIGHLIGHT_H_

#include <iosfwd>
#include <memory>

#include "base/macros.h"
#include "base/time/time.h"
#include "third_party/skia/include/core/SkColor.h"
#include "ui/gfx/geometry/point.h"
#include "ui/gfx/geometry/point_f.h"
#include "ui/gfx/geometry/size.h"
#include "ui/gfx/geometry/size_f.h"
#include "ui/gfx/transform.h"
#include "ui/views/views_export.h"

namespace ui {
class Layer;
class CallbackLayerAnimationObserver;
}  // namespace ui

namespace views {
namespace test {
class InkDropHighlightTestApi;
}  // namespace test

class BasePaintedLayerDelegate;
class InkDropHighlightObserver;

// Manages fade in/out animations for a painted Layer that is used to provide
// visual feedback on ui::Views for highlight states (e.g. mouse hover, keyboard
// focus).
class VIEWS_EXPORT InkDropHighlight {
 public:
  enum AnimationType { FADE_IN, FADE_OUT };

  // Creates a highlight with a specified painter.
  InkDropHighlight(const gfx::PointF& center_point,
                   std::unique_ptr<BasePaintedLayerDelegate> layer_delegate);

  // Creates a highlight that paints a partially transparent roundrect with
  // color |color|.
  InkDropHighlight(const gfx::SizeF& size,
                   int corner_radius,
                   const gfx::PointF& center_point,
                   SkColor color);

  // Deprecated version of the above that takes a Size instead of SizeF.
  // TODO(estade): remove. See crbug.com/706228
  InkDropHighlight(const gfx::Size& size,
                   int corner_radius,
                   const gfx::PointF& center_point,
                   SkColor color);

  virtual ~InkDropHighlight();

  void set_observer(InkDropHighlightObserver* observer) {
    observer_ = observer;
  }

  void set_explode_size(const gfx::SizeF& size) { explode_size_ = size; }

  void set_visible_opacity(float visible_opacity) {
    visible_opacity_ = visible_opacity;
  }

  // Returns true if the highlight animation is either in the process of fading
  // in or is fully visible.
  bool IsFadingInOrVisible() const;

  // Fades in the highlight visual over the given |duration|.
  void FadeIn(const base::TimeDelta& duration);

  // Fades out the highlight visual over the given |duration|. If |explode| is
  // true then the highlight will animate a size increase in addition to the
  // fade out.
  void FadeOut(const base::TimeDelta& duration, bool explode);

  // The root Layer that can be added in to a Layer tree.
  ui::Layer* layer() { return layer_.get(); }

  // Returns a test api to access internals of this. Default implmentations
  // should return nullptr and test specific subclasses can override to return
  // an instance.
  virtual test::InkDropHighlightTestApi* GetTestApi();

 private:
  friend class test::InkDropHighlightTestApi;

  // Animates a fade in/out as specified by |animation_type| combined with a
  // transformation from the |initial_size| to the |target_size| over the given
  // |duration|.
  void AnimateFade(AnimationType animation_type,
                   const base::TimeDelta& duration,
                   const gfx::SizeF& initial_size,
                   const gfx::SizeF& target_size);

  // Calculates the Transform to apply to |layer_| for the given |size|.
  gfx::Transform CalculateTransform(const gfx::SizeF& size) const;

  // The callback that will be invoked when a fade in/out animation is started.
  void AnimationStartedCallback(
      AnimationType animation_type,
      const ui::CallbackLayerAnimationObserver& observer);

  // The callback that will be invoked when a fade in/out animation is complete.
  bool AnimationEndedCallback(
      AnimationType animation_type,
      const ui::CallbackLayerAnimationObserver& observer);

  // The size of the highlight shape when fully faded in.
  gfx::SizeF size_;

  // The target size of the highlight shape when it expands during a fade out
  // animation.
  gfx::SizeF explode_size_;

  // The center point of the highlight shape in the parent Layer's coordinate
  // space.
  gfx::PointF center_point_;

  // The opacity for the fully visible state of the highlight.
  float visible_opacity_;

  // True if the last animation to be initiated was a FADE_IN, and false
  // otherwise.
  bool last_animation_initiated_was_fade_in_;

  // The LayerDelegate that paints the highlight |layer_|.
  std::unique_ptr<BasePaintedLayerDelegate> layer_delegate_;

  // The visual highlight layer that is painted by |layer_delegate_|.
  std::unique_ptr<ui::Layer> layer_;

  InkDropHighlightObserver* observer_;

  DISALLOW_COPY_AND_ASSIGN(InkDropHighlight);
};

// Returns a human readable string for |animation_type|.  Useful for logging.
VIEWS_EXPORT std::string ToString(
    InkDropHighlight::AnimationType animation_type);

// This is declared here for use in gtest-based unit tests but is defined in
// the views_test_support target. Depend on that to use this in your unit test.
// This should not be used in production code - call ToString() instead.
void PrintTo(InkDropHighlight::AnimationType animation_type,
             ::std::ostream* os);

}  // namespace views

#endif  // UI_VIEWS_ANIMATION_INK_DROP_HIGHLIGHT_H_
