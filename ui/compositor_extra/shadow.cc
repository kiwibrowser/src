// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/compositor_extra/shadow.h"

#include "ui/base/resource/resource_bundle.h"
#include "ui/compositor/layer.h"
#include "ui/compositor/scoped_layer_animation_settings.h"
#include "ui/gfx/geometry/insets.h"
#include "ui/gfx/shadow_util.h"

namespace ui {

namespace {

// Duration for opacity animation in milliseconds.
constexpr int kShadowAnimationDurationMs = 100;

}  // namespace

Shadow::Shadow() {}

Shadow::~Shadow() {}

void Shadow::Init(int elevation) {
  DCHECK_GE(elevation, 0);
  desired_elevation_ = elevation;
  layer_.reset(new ui::Layer(ui::LAYER_NOT_DRAWN));
  RecreateShadowLayer();
}

void Shadow::SetContentBounds(const gfx::Rect& content_bounds) {
  // When the window moves but doesn't change size, this is a no-op. (The
  // origin stays the same in this case.)
  if (content_bounds == content_bounds_)
    return;

  content_bounds_ = content_bounds;
  UpdateLayerBounds();
}

void Shadow::SetElevation(int elevation) {
  DCHECK_GE(elevation, 0);
  if (desired_elevation_ == elevation)
    return;

  desired_elevation_ = elevation;

  // Stop waiting for any as yet unfinished implicit animations.
  StopObservingImplicitAnimations();

  // The old shadow layer is the new fading out layer.
  DCHECK(shadow_layer_);
  fading_layer_ = std::move(shadow_layer_);
  RecreateShadowLayer();
  shadow_layer_->SetOpacity(0.f);

  {
    // Observe the fade out animation so we can clean up the layer when done.
    ui::ScopedLayerAnimationSettings settings(fading_layer_->GetAnimator());
    settings.AddObserver(this);
    settings.SetTransitionDuration(
        base::TimeDelta::FromMilliseconds(kShadowAnimationDurationMs));
    fading_layer_->SetOpacity(0.f);
  }

  {
    // We don't care to observe this one.
    ui::ScopedLayerAnimationSettings settings(shadow_layer_->GetAnimator());
    settings.SetTransitionDuration(
        base::TimeDelta::FromMilliseconds(kShadowAnimationDurationMs));
    shadow_layer_->SetOpacity(1.f);
  }
}

void Shadow::SetRoundedCornerRadius(int rounded_corner_radius) {
  DCHECK_GE(rounded_corner_radius, 0);
  if (rounded_corner_radius_ == rounded_corner_radius)
    return;

  rounded_corner_radius_ = rounded_corner_radius;
  UpdateLayerBounds();
}

void Shadow::OnImplicitAnimationsCompleted() {
  fading_layer_.reset();
  // The size needed for layer() may be smaller now that |fading_layer_| is
  // removed.
  UpdateLayerBounds();
}

void Shadow::RecreateShadowLayer() {
  shadow_layer_.reset(new ui::Layer(ui::LAYER_NINE_PATCH));
  shadow_layer_->set_name("Shadow");
  shadow_layer_->SetVisible(true);
  shadow_layer_->SetFillsBoundsOpaquely(false);
  layer()->Add(shadow_layer_.get());

  UpdateLayerBounds();
}

void Shadow::UpdateLayerBounds() {
  if (content_bounds_.IsEmpty())
    return;

  // The ninebox assumption breaks down when the window is too small for the
  // desired elevation. The height/width of |blur_region| will be 4 * elevation
  // (see ShadowDetails::Get), so cap elevation at the most we can handle.
  const int smaller_dimension =
      std::min(content_bounds_.width(), content_bounds_.height());
  const int size_adjusted_elevation =
      std::min((smaller_dimension - 2 * rounded_corner_radius_) / 4,
               static_cast<int>(desired_elevation_));
  const auto& details =
      gfx::ShadowDetails::Get(size_adjusted_elevation, rounded_corner_radius_);
  gfx::Insets blur_region = gfx::ShadowValue::GetBlurRegion(details.values) +
                            gfx::Insets(rounded_corner_radius_);
  // Update |shadow_layer_| if details changed and it has been updated in
  // the past (|details_| is set), or elevation is non-zero.
  if ((&details != details_) && (details_ || size_adjusted_elevation)) {
    shadow_layer_->UpdateNinePatchLayerImage(details.ninebox_image);
    // The ninebox grid is defined in terms of the image size. The shadow blurs
    // in both inward and outward directions from the edge of the contents, so
    // the aperture goes further inside the image than the shadow margins (which
    // represent exterior blur).
    gfx::Rect aperture(details.ninebox_image.size());
    aperture.Inset(blur_region);
    shadow_layer_->UpdateNinePatchLayerAperture(aperture);
    details_ = &details;
  }

  // Shadow margins are negative, so this expands outwards from
  // |content_bounds_|.
  const gfx::Insets margins = gfx::ShadowValue::GetMargin(details.values);
  gfx::Rect new_layer_bounds = content_bounds_;
  new_layer_bounds.Inset(margins);
  gfx::Rect shadow_layer_bounds(new_layer_bounds.size());

  // When there's an old shadow fading out, the bounds of layer() have to be
  // big enough to encompass both shadows.
  if (fading_layer_) {
    const gfx::Rect old_layer_bounds = layer()->bounds();
    gfx::Rect combined_layer_bounds = old_layer_bounds;
    combined_layer_bounds.Union(new_layer_bounds);
    layer()->SetBounds(combined_layer_bounds);

    // If this is reached via SetContentBounds, we might hypothetically need
    // to change the size of the fading layer, but the fade is so fast it's
    // not really an issue.
    gfx::Rect fading_layer_bounds(fading_layer_->bounds());
    fading_layer_bounds.Offset(old_layer_bounds.origin() -
                               combined_layer_bounds.origin());
    fading_layer_->SetBounds(fading_layer_bounds);

    shadow_layer_bounds.Offset(new_layer_bounds.origin() -
                               combined_layer_bounds.origin());
  } else {
    layer()->SetBounds(new_layer_bounds);
  }

  shadow_layer_->SetBounds(shadow_layer_bounds);

  // Occlude the region inside the bounding box. Occlusion uses shadow layer
  // space. See nine_patch_layer.h for more context on what's going on here.
  gfx::Rect occlusion_bounds(shadow_layer_bounds.size());
  occlusion_bounds.Inset(-margins + gfx::Insets(rounded_corner_radius_));
  shadow_layer_->UpdateNinePatchOcclusion(occlusion_bounds);

  // The border is the same inset as the aperture.
  shadow_layer_->UpdateNinePatchLayerBorder(
      gfx::Rect(blur_region.left(), blur_region.top(), blur_region.width(),
                blur_region.height()));
}

}  // namespace ui
