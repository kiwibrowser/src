// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/ui/ws/window_finder.h"

#include "base/containers/adapters.h"
#include "services/ui/ws/server_window.h"
#include "ui/gfx/geometry/point.h"
#include "ui/gfx/geometry/point3_f.h"
#include "ui/gfx/geometry/point_f.h"
#include "ui/gfx/transform.h"

namespace ui {
namespace ws {
namespace {

bool IsLocationInNonclientArea(const ServerWindow* target,
                               const gfx::Point& location) {
  if (!target->parent() || target->bounds().size().IsEmpty())
    return false;

  gfx::Rect client_area(target->bounds().size());
  client_area.Inset(target->client_area());
  if (client_area.Contains(location))
    return false;

  for (const auto& rect : target->additional_client_areas()) {
    if (rect.Contains(location))
      return false;
  }

  return true;
}

bool ShouldUseExtendedHitRegion(const ServerWindow* window) {
  if (!window->parent())
    return false;

  const mojom::ShowState show_state = window->GetShowState();
  if (show_state == mojom::ShowState::MAXIMIZED ||
      show_state == mojom::ShowState::FULLSCREEN) {
    return false;
  }
  // This matches the logic of EasyResizeWindowTargeter.
  return !window->transient_parent() ||
         window->transient_parent() == window->parent();
}

// Returns true if |location_in_window| is in the extended hit region and not
// in the normal bounds of |window|.
bool IsLocationInExtendedHitRegion(EventSource event_source,
                                   const ServerWindow* window,
                                   const gfx::Point& location_in_window) {
  if (!ShouldUseExtendedHitRegion(window))
    return false;

  const gfx::Insets& extended_hit_insets =
      event_source == EventSource::MOUSE
          ? window->parent()->extended_mouse_hit_test_region()
          : window->parent()->extended_touch_hit_test_region();
  if (extended_hit_insets.IsEmpty())
    return false;

  gfx::Rect child_bounds(window->bounds().size());
  if (child_bounds.Contains(location_in_window))
    return false;

  child_bounds.Inset(extended_hit_insets);
  return child_bounds.Contains(location_in_window);
}

gfx::Transform TransformFromParent(const ServerWindow* window,
                                   const gfx::Transform& current_transform) {
  gfx::Transform result = current_transform;
  if (window->bounds().origin() != gfx::Point()) {
    gfx::Transform translation;
    translation.Translate(static_cast<float>(window->bounds().x()),
                          static_cast<float>(window->bounds().y()));
    result.PreconcatTransform(translation);
  }
  if (!window->transform().IsIdentity())
    result.PreconcatTransform(window->transform());
  return result;
}

bool FindDeepestVisibleWindowForLocationImpl(
    ServerWindow* window,
    bool is_root_window,
    EventSource event_source,
    const gfx::Point& location_in_root,
    const gfx::Point& location_in_window,
    const gfx::Transform& transform_from_parent,
    DeepestWindow* deepest_window) {
  // The non-client area takes precedence over descendants, as otherwise the
  // user would likely not be able to hit the non-client area as it's common
  // for descendants to go into the non-client area.
  //
  // Display roots aren't allowed to have non-client areas. This is important
  // as roots may have a transform, which causes problem in comparing sizes.
  if (!is_root_window &&
      IsLocationInNonclientArea(window, location_in_window)) {
    deepest_window->window = window;
    deepest_window->in_non_client_area = true;
    return true;
  }
  const mojom::EventTargetingPolicy event_targeting_policy =
      window->event_targeting_policy();

  if (event_targeting_policy == ui::mojom::EventTargetingPolicy::NONE)
    return false;

  if (event_targeting_policy ==
          mojom::EventTargetingPolicy::TARGET_AND_DESCENDANTS ||
      event_targeting_policy == mojom::EventTargetingPolicy::DESCENDANTS_ONLY) {
    const ServerWindow::Windows& children = window->children();
    for (ServerWindow* child : base::Reversed(children)) {
      if (!child->visible())
        continue;

      const gfx::Transform child_transform =
          TransformFromParent(child, transform_from_parent);
      gfx::Point3F location_in_child3(gfx::PointF{location_in_root});
      child_transform.TransformPointReverse(&location_in_child3);
      const gfx::Point location_in_child =
          gfx::ToFlooredPoint(location_in_child3.AsPointF());
      if (IsLocationInExtendedHitRegion(event_source, child,
                                        location_in_child)) {
        deepest_window->window = child;
        deepest_window->in_non_client_area = true;
        return true;
      }
      gfx::Rect child_bounds(child->bounds().size());
      if (!child_bounds.Contains(location_in_child) ||
          (child->hit_test_mask() &&
           !child->hit_test_mask()->Contains(location_in_child))) {
        continue;
      }

      const bool child_is_root = false;
      if (FindDeepestVisibleWindowForLocationImpl(
              child, child_is_root, event_source, location_in_root,
              location_in_child, child_transform, deepest_window)) {
        return true;
      }
    }
  }

  if (event_targeting_policy == mojom::EventTargetingPolicy::DESCENDANTS_ONLY)
    return false;

  deepest_window->window = window;
  deepest_window->in_non_client_area = false;
  return true;
}

}  // namespace

DeepestWindow FindDeepestVisibleWindowForLocation(ServerWindow* root_window,
                                                  EventSource event_source,
                                                  const gfx::Point& location) {
  gfx::Point initial_location = location;
  gfx::Transform root_transform = root_window->transform();
  if (!root_transform.IsIdentity()) {
    gfx::Point3F transformed_location(gfx::PointF{initial_location});
    root_transform.TransformPointReverse(&transformed_location);
    initial_location = gfx::ToFlooredPoint(transformed_location.AsPointF());
  }
  DeepestWindow result;
  // Allow the root to have a transform, which mirrors what happens with
  // WindowManagerDisplayRoot.
  const bool is_root_window = true;
  FindDeepestVisibleWindowForLocationImpl(
      root_window, is_root_window, event_source, location, initial_location,
      root_transform, &result);
  return result;
}

}  // namespace ws
}  // namespace ui
