// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/exo/notification_surface.h"

#include "components/exo/notification_surface_manager.h"
#include "components/exo/shell_surface.h"
#include "components/exo/surface.h"
#include "ui/aura/window.h"
#include "ui/gfx/geometry/rect.h"

namespace exo {

NotificationSurface::NotificationSurface(NotificationSurfaceManager* manager,
                                         Surface* surface,
                                         const std::string& notification_key)
    : SurfaceTreeHost("ExoNotificationSurface"),
      manager_(manager),
      notification_key_(notification_key) {
  surface->AddSurfaceObserver(this);
  SetRootSurface(surface);
  host_window()->Show();
}

NotificationSurface::~NotificationSurface() {
  if (added_to_manager_)
    manager_->RemoveSurface(this);
  if (root_surface())
    root_surface()->RemoveSurfaceObserver(this);
}

const gfx::Size& NotificationSurface::GetContentSize() const {
  return root_surface()->content_size();
}

void NotificationSurface::SetApplicationId(const char* application_id) {
  exo::ShellSurface::SetApplicationId(host_window(),
                                      base::make_optional(application_id));
}

void NotificationSurface::OnSurfaceCommit() {
  SurfaceTreeHost::OnSurfaceCommit();

  // Defer AddSurface until there are contents to show.
  if (!added_to_manager_ && !host_window()->bounds().IsEmpty()) {
    added_to_manager_ = true;
    manager_->AddSurface(this);
  }

  SubmitCompositorFrame();
}

void NotificationSurface::OnSurfaceDestroying(Surface* surface) {
  DCHECK_EQ(surface, root_surface());
  surface->RemoveSurfaceObserver(this);
  SetRootSurface(nullptr);
}

}  // namespace exo
