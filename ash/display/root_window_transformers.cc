// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/display/root_window_transformers.h"

#include <cmath>

#include "ash/host/root_window_transformer.h"
#include "ash/magnifier/magnification_controller.h"
#include "ash/public/cpp/ash_switches.h"
#include "ash/shell.h"
#include "ash/utility/transformer_util.h"
#include "base/command_line.h"
#include "ui/compositor/dip_util.h"
#include "ui/display/display.h"
#include "ui/display/manager/display_layout_store.h"
#include "ui/display/manager/display_manager.h"
#include "ui/display/screen.h"
#include "ui/gfx/geometry/insets.h"
#include "ui/gfx/geometry/size_conversions.h"
#include "ui/gfx/transform.h"

namespace ash {
namespace {

// TODO(oshima): Transformers should be able to adjust itself
// when the device scale factor is changed, instead of
// precalculating the transform using fixed value.

// Creates rotation transform for |root_window| to |new_rotation|. This will
// call |CreateRotationTransform()|, the |old_rotation| will implicitly be
// |display::Display::ROTATE_0|.
gfx::Transform CreateRootWindowRotationTransform(
    aura::Window* root_window,
    const display::Display& display) {
  display::ManagedDisplayInfo info =
      Shell::Get()->display_manager()->GetDisplayInfo(display.id());
  return CreateRotationTransform(display::Display::ROTATE_0,
                                 info.GetActiveRotation(), display.bounds());
}

gfx::Transform CreateInsetsAndScaleTransform(const gfx::Insets& insets,
                                             float device_scale_factor,
                                             float ui_scale) {
  gfx::Transform transform;
  if (insets.top() != 0 || insets.left() != 0) {
    float x_offset = insets.left() / device_scale_factor;
    float y_offset = insets.top() / device_scale_factor;
    transform.Translate(x_offset, y_offset);
  }
  float inverted_scale = 1.0f / ui_scale;
  transform.Scale(inverted_scale, inverted_scale);
  return transform;
}

gfx::Transform CreateMirrorTransform(const display::Display& display) {
  gfx::Transform transform;
  transform.matrix().set3x3(-1, 0, 0, 0, 1, 0, 0, 0, 1);
  transform.Translate(-display.size().width(), 0);
  return transform;
}

// RootWindowTransformer for ash environment.
class AshRootWindowTransformer : public RootWindowTransformer {
 public:
  AshRootWindowTransformer(aura::Window* root, const display::Display& display)
      : root_window_(root) {
    display::DisplayManager* display_manager = Shell::Get()->display_manager();
    display::ManagedDisplayInfo info =
        display_manager->GetDisplayInfo(display.id());
    host_insets_ = info.GetOverscanInsetsInPixel();
    root_window_ui_scale_ = info.GetEffectiveUIScale();
    root_window_bounds_transform_ =
        CreateInsetsAndScaleTransform(host_insets_,
                                      display.device_scale_factor(),
                                      root_window_ui_scale_) *
        CreateRootWindowRotationTransform(root, display);
    if (base::CommandLine::ForCurrentProcess()->HasSwitch(
            switches::kAshEnableMirroredScreen)) {
      // Apply the transform that flips the screen image horizontally so that
      // the screen looks normal when reflected on a mirror.
      root_window_bounds_transform_ =
          root_window_bounds_transform_ * CreateMirrorTransform(display);
    }

    transform_ = root_window_bounds_transform_;
    MagnificationController* magnifier =
        Shell::Get()->magnification_controller();
    if (magnifier)
      transform_ *= magnifier->GetMagnifierTransform();

    CHECK(transform_.GetInverse(&invert_transform_));
  }

  // aura::RootWindowTransformer overrides:
  gfx::Transform GetTransform() const override { return transform_; }
  gfx::Transform GetInverseTransform() const override {
    return invert_transform_;
  }
  gfx::Rect GetRootWindowBounds(const gfx::Size& host_size) const override {
    gfx::Rect bounds(host_size);
    bounds.Inset(host_insets_);
    bounds = ui::ConvertRectToDIP(root_window_->layer(), bounds);
    gfx::RectF new_bounds(bounds);
    root_window_bounds_transform_.TransformRect(&new_bounds);
    // Apply |root_window_scale_| twice as the downscaling
    // is already applied once in |SetTransformInternal()|.
    // TODO(oshima): This is a bit ugly. Consider specifying
    // the pseudo host resolution instead.
    new_bounds.Scale(root_window_ui_scale_ * root_window_ui_scale_);
    // Ignore the origin because RootWindow's insets are handled by
    // the transform.
    // Floor the size because the bounds is no longer aligned to
    // backing pixel when |root_window_scale_| is specified
    // (850 height at 1.25 scale becomes 1062.5 for example.)
    return gfx::Rect(gfx::ToFlooredSize(new_bounds.size()));
  }

  gfx::Insets GetHostInsets() const override { return host_insets_; }

 private:
  ~AshRootWindowTransformer() override = default;

  aura::Window* root_window_;
  gfx::Transform transform_;

  // The accurate representation of the inverse of the |transform_|.
  // This is used to avoid computation error caused by
  // |gfx::Transform::GetInverse|.
  gfx::Transform invert_transform_;

  // The transform of the root window bounds. This is used to calculate
  // the size of root window.
  gfx::Transform root_window_bounds_transform_;

  // The scale of the root window. See |display_info::ui_scale_|
  // for more info.
  float root_window_ui_scale_;

  gfx::Insets host_insets_;

  DISALLOW_COPY_AND_ASSIGN(AshRootWindowTransformer);
};

// RootWindowTransformer for mirror root window. We simply copy the
// texture (bitmap) of the source display into the mirror window, so
// the root window bounds is the same as the source display's
// pixel size (excluding overscan insets).
class MirrorRootWindowTransformer : public RootWindowTransformer {
 public:
  MirrorRootWindowTransformer(
      const display::ManagedDisplayInfo& source_display_info,
      const display::ManagedDisplayInfo& mirror_display_info) {
    root_bounds_ = gfx::Rect(source_display_info.bounds_in_native().size());

    // The rotation of the source display (internal display) should be undone in
    // the destination display (external display) if mirror mode is enabled in
    // tablet mode.
    bool should_undo_rotation = Shell::Get()
                                    ->display_manager()
                                    ->layout_store()
                                    ->forced_mirror_mode_for_tablet();
    gfx::Transform rotation_transform;
    if (should_undo_rotation) {
      // Calculate the transform to undo the rotation and apply it to the
      // source display.
      rotation_transform =
          CreateRotationTransform(source_display_info.GetActiveRotation(),
                                  display::Display::ROTATE_0, root_bounds_);
      gfx::RectF rotated_bounds(root_bounds_);
      rotation_transform.TransformRect(&rotated_bounds);
      root_bounds_ = gfx::ToNearestRect(rotated_bounds);
    }

    gfx::Rect mirror_display_rect =
        gfx::Rect(mirror_display_info.bounds_in_native().size());

    bool letterbox = root_bounds_.width() * mirror_display_rect.height() >
                     root_bounds_.height() * mirror_display_rect.width();
    if (letterbox) {
      float mirror_scale_ratio =
          (static_cast<float>(root_bounds_.width()) /
           static_cast<float>(mirror_display_rect.width()));
      float inverted_scale = 1.0f / mirror_scale_ratio;
      int margin = static_cast<int>((mirror_display_rect.height() -
                                     root_bounds_.height() * inverted_scale) /
                                    2);
      insets_.Set(0, margin, 0, margin);

      transform_.Translate(0, margin);
      transform_.Scale(inverted_scale, inverted_scale);
    } else {
      float mirror_scale_ratio =
          (static_cast<float>(root_bounds_.height()) /
           static_cast<float>(mirror_display_rect.height()));
      float inverted_scale = 1.0f / mirror_scale_ratio;
      int margin = static_cast<int>((mirror_display_rect.width() -
                                     root_bounds_.width() * inverted_scale) /
                                    2);
      insets_.Set(margin, 0, margin, 0);

      transform_.Translate(margin, 0);
      transform_.Scale(inverted_scale, inverted_scale);
    }

    // Make sure the rotation transform is applied in the beginning.
    transform_.PreconcatTransform(rotation_transform);
  }

  // aura::RootWindowTransformer overrides:
  gfx::Transform GetTransform() const override { return transform_; }
  gfx::Transform GetInverseTransform() const override {
    gfx::Transform invert;
    CHECK(transform_.GetInverse(&invert));
    return invert;
  }
  gfx::Rect GetRootWindowBounds(const gfx::Size& host_size) const override {
    return root_bounds_;
  }
  gfx::Insets GetHostInsets() const override { return insets_; }

 private:
  ~MirrorRootWindowTransformer() override = default;

  gfx::Transform transform_;
  gfx::Rect root_bounds_;
  gfx::Insets insets_;

  DISALLOW_COPY_AND_ASSIGN(MirrorRootWindowTransformer);
};

class PartialBoundsRootWindowTransformer : public RootWindowTransformer {
 public:
  PartialBoundsRootWindowTransformer(const gfx::Rect& screen_bounds,
                                     const display::Display& display) {
    const display::DisplayManager* display_manager =
        Shell::Get()->display_manager();
    display::ManagedDisplayInfo display_info =
        display_manager->GetDisplayInfo(display.id());
    // Physical root bounds.
    root_bounds_ = gfx::Rect(display_info.bounds_in_native().size());

    // |screen_bounds| is the unified desktop logical bounds.
    // Calculate the unified height scale value, and apply the same scale on the
    // row physical height to get the row logical height.
    display::Display unified_display =
        display::Screen::GetScreen()->GetPrimaryDisplay();
    const int unified_physical_height =
        unified_display.GetSizeInPixel().height();
    const int unified_logical_height = screen_bounds.height();
    const float unified_height_scale =
        static_cast<float>(unified_logical_height) / unified_physical_height;

    const int row_index =
        display_manager->GetMirroringDisplayRowIndexInUnifiedMatrix(
            display.id());
    const int row_physical_height =
        display_manager->GetUnifiedDesktopRowMaxHeight(row_index);
    const int row_logical_height = row_physical_height * unified_height_scale;
    const float dsf = unified_display.device_scale_factor();
    const float scale = root_bounds_.height() / (dsf * row_logical_height);

    transform_.Scale(scale, scale);
    transform_.Translate(-SkIntToMScalar(display.bounds().x()),
                         -SkIntToMScalar(display.bounds().y()));
  }

  // RootWindowTransformer:
  gfx::Transform GetTransform() const override { return transform_; }
  gfx::Transform GetInverseTransform() const override {
    gfx::Transform invert;
    CHECK(transform_.GetInverse(&invert));
    return invert;
  }
  gfx::Rect GetRootWindowBounds(const gfx::Size& host_size) const override {
    return root_bounds_;
  }
  gfx::Insets GetHostInsets() const override { return gfx::Insets(); }

 private:
  gfx::Transform transform_;
  gfx::Rect root_bounds_;

  DISALLOW_COPY_AND_ASSIGN(PartialBoundsRootWindowTransformer);
};

}  // namespace

RootWindowTransformer* CreateRootWindowTransformerForDisplay(
    aura::Window* root,
    const display::Display& display) {
  return new AshRootWindowTransformer(root, display);
}

RootWindowTransformer* CreateRootWindowTransformerForMirroredDisplay(
    const display::ManagedDisplayInfo& source_display_info,
    const display::ManagedDisplayInfo& mirror_display_info) {
  return new MirrorRootWindowTransformer(source_display_info,
                                         mirror_display_info);
}

RootWindowTransformer* CreateRootWindowTransformerForUnifiedDesktop(
    const gfx::Rect& screen_bounds,
    const display::Display& display) {
  return new PartialBoundsRootWindowTransformer(screen_bounds, display);
}

}  // namespace ash
