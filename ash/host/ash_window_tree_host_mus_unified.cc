// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/host/ash_window_tree_host_mus_unified.h"

#include <memory>
#include <utility>

#include "ash/host/ash_window_tree_host_mirroring_delegate.h"
#include "ash/host/root_window_transformer.h"
#include "base/logging.h"
#include "base/stl_util.h"
#include "ui/aura/window.h"
#include "ui/aura/window_event_dispatcher.h"
#include "ui/aura/window_targeter.h"
#include "ui/compositor/compositor.h"
#include "ui/gfx/geometry/insets.h"
#include "ui/platform_window/stub/stub_window.h"

namespace ash {

// Performs event targeting and dispatching for events in unified mode.
class MusUnifiedEventTargeter : public aura::WindowTargeter {
 public:
  MusUnifiedEventTargeter(aura::Window* src_root,
                          aura::Window* dst_root,
                          AshWindowTreeHostMirroringDelegate* delegate)
      : src_root_(src_root), dst_root_(dst_root), delegate_(delegate) {
    DCHECK(delegate);
  }

  ui::EventTarget* FindTargetForEvent(ui::EventTarget* root,
                                      ui::Event* event) override {
    if (root == src_root_ && !event->target()) {
      delegate_->SetCurrentEventTargeterSourceHost(src_root_->GetHost());

      ignore_result(
          dst_root_->GetHost()->event_sink()->OnEventFromSource(event));

      // Reset the source host.
      delegate_->SetCurrentEventTargeterSourceHost(nullptr);

      return nullptr;
    }

    NOTREACHED();
    return aura::WindowTargeter::FindTargetForEvent(root, event);
  }

 private:
  aura::Window* src_root_;
  aura::Window* dst_root_;
  AshWindowTreeHostMirroringDelegate* delegate_;  // Not owned.

  DISALLOW_COPY_AND_ASSIGN(MusUnifiedEventTargeter);
};

AshWindowTreeHostMusUnified::AshWindowTreeHostMusUnified(
    aura::WindowTreeHostMusInitParams init_params,
    AshWindowTreeHostMirroringDelegate* delegate)
    : AshWindowTreeHostMus(std::move(init_params)), delegate_(delegate) {
  DCHECK(delegate);
}

AshWindowTreeHostMusUnified::~AshWindowTreeHostMusUnified() {
  for (auto* ash_host : mirroring_hosts_)
    ash_host->AsWindowTreeHost()->window()->RemoveObserver(this);
}

void AshWindowTreeHostMusUnified::PrepareForShutdown() {
  AshWindowTreeHostMus::PrepareForShutdown();

  for (auto* host : mirroring_hosts_)
    host->PrepareForShutdown();
}

void AshWindowTreeHostMusUnified::RegisterMirroringHost(
    AshWindowTreeHost* mirroring_ash_host) {
  aura::Window* src_root = mirroring_ash_host->AsWindowTreeHost()->window();
  src_root->SetEventTargeter(
      std::make_unique<MusUnifiedEventTargeter>(src_root, window(), delegate_));
  DCHECK(!base::ContainsValue(mirroring_hosts_, mirroring_ash_host));
  mirroring_hosts_.push_back(mirroring_ash_host);
  mirroring_ash_host->AsWindowTreeHost()->window()->AddObserver(this);
}

void AshWindowTreeHostMusUnified::SetBoundsInPixels(
    const gfx::Rect& bounds,
    const viz::LocalSurfaceId& local_surface_id) {
  AshWindowTreeHostMus::SetBoundsInPixels(bounds, local_surface_id);
  OnHostResizedInPixels(bounds.size());
}

void AshWindowTreeHostMusUnified::SetCursorNative(gfx::NativeCursor cursor) {
  for (auto* host : mirroring_hosts_)
    host->AsWindowTreeHost()->SetCursor(cursor);
}

void AshWindowTreeHostMusUnified::OnCursorVisibilityChangedNative(bool show) {
  for (auto* host : mirroring_hosts_)
    host->AsWindowTreeHost()->OnCursorVisibilityChanged(show);
}

void AshWindowTreeHostMusUnified::OnBoundsChanged(const gfx::Rect& bounds) {
  if (platform_window())
    OnHostResizedInPixels(bounds.size());
}

void AshWindowTreeHostMusUnified::OnWindowDestroying(aura::Window* window) {
  auto iter =
      std::find_if(mirroring_hosts_.begin(), mirroring_hosts_.end(),
                   [window](AshWindowTreeHost* ash_host) {
                     return ash_host->AsWindowTreeHost()->window() == window;
                   });
  DCHECK(iter != mirroring_hosts_.end());
  window->RemoveObserver(this);
  mirroring_hosts_.erase(iter);
}

}  // namespace ash
