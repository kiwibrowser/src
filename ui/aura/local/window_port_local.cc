// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/aura/local/window_port_local.h"

#include "components/viz/host/host_frame_sink_manager.h"
#include "ui/aura/client/cursor_client.h"
#include "ui/aura/env.h"
#include "ui/aura/local/layer_tree_frame_sink_local.h"
#include "ui/aura/window.h"
#include "ui/aura/window_delegate.h"
#include "ui/base/layout.h"
#include "ui/display/display.h"
#include "ui/display/screen.h"

namespace aura {
namespace {

class ScopedCursorHider {
 public:
  explicit ScopedCursorHider(Window* window)
      : window_(window), hid_cursor_(false) {
    if (!window_->IsRootWindow())
      return;
    const bool cursor_is_in_bounds = window_->GetBoundsInScreen().Contains(
        Env::GetInstance()->last_mouse_location());
    client::CursorClient* cursor_client = client::GetCursorClient(window_);
    if (cursor_is_in_bounds && cursor_client &&
        cursor_client->IsCursorVisible()) {
      cursor_client->HideCursor();
      hid_cursor_ = true;
    }
  }
  ~ScopedCursorHider() {
    if (!window_->IsRootWindow())
      return;

    // Update the device scale factor of the cursor client only when the last
    // mouse location is on this root window.
    if (hid_cursor_) {
      client::CursorClient* cursor_client = client::GetCursorClient(window_);
      if (cursor_client) {
        const display::Display& display =
            display::Screen::GetScreen()->GetDisplayNearestWindow(window_);
        cursor_client->SetDisplay(display);
        cursor_client->ShowCursor();
      }
    }
  }

 private:
  Window* window_;
  bool hid_cursor_;

  DISALLOW_COPY_AND_ASSIGN(ScopedCursorHider);
};

}  // namespace

WindowPortLocal::WindowPortLocal(Window* window)
    : window_(window), weak_factory_(this) {}

WindowPortLocal::~WindowPortLocal() {}

void WindowPortLocal::OnPreInit(Window* window) {}

void WindowPortLocal::OnDeviceScaleFactorChanged(
    float old_device_scale_factor,
    float new_device_scale_factor) {
  if (!window_->IsRootWindow() &&
      last_device_scale_factor_ != new_device_scale_factor &&
      IsEmbeddingExternalContent()) {
    last_device_scale_factor_ = new_device_scale_factor;
    parent_local_surface_id_allocator_->GenerateId();
    if (frame_sink_)
      frame_sink_->SetLocalSurfaceId(GetCurrentLocalSurfaceId());
  }

  ScopedCursorHider hider(window_);
  if (window_->delegate()) {
    window_->delegate()->OnDeviceScaleFactorChanged(old_device_scale_factor,
                                                    new_device_scale_factor);
  }
}

void WindowPortLocal::OnWillAddChild(Window* child) {}

void WindowPortLocal::OnWillRemoveChild(Window* child) {}

void WindowPortLocal::OnWillMoveChild(size_t current_index, size_t dest_index) {
}

void WindowPortLocal::OnVisibilityChanged(bool visible) {}

void WindowPortLocal::OnDidChangeBounds(const gfx::Rect& old_bounds,
                                        const gfx::Rect& new_bounds) {
  if (!window_->IsRootWindow() && last_size_ != new_bounds.size() &&
      IsEmbeddingExternalContent()) {
    last_size_ = new_bounds.size();
    parent_local_surface_id_allocator_->GenerateId();
    if (frame_sink_)
      frame_sink_->SetLocalSurfaceId(GetCurrentLocalSurfaceId());
  }
}

void WindowPortLocal::OnDidChangeTransform(
    const gfx::Transform& old_transform,
    const gfx::Transform& new_transform) {}

std::unique_ptr<ui::PropertyData> WindowPortLocal::OnWillChangeProperty(
    const void* key) {
  return nullptr;
}

void WindowPortLocal::OnPropertyChanged(
    const void* key,
    int64_t old_value,
    std::unique_ptr<ui::PropertyData> data) {}

std::unique_ptr<cc::LayerTreeFrameSink>
WindowPortLocal::CreateLayerTreeFrameSink() {
  auto* context_factory_private =
      aura::Env::GetInstance()->context_factory_private();
  auto frame_sink_id = context_factory_private->AllocateFrameSinkId();
  auto frame_sink = std::make_unique<LayerTreeFrameSinkLocal>(
      frame_sink_id, context_factory_private->GetHostFrameSinkManager(),
      window_->GetName());
  window_->SetEmbedFrameSinkId(frame_sink_id);
  frame_sink->SetSurfaceChangedCallback(base::Bind(
      &WindowPortLocal::OnSurfaceChanged, weak_factory_.GetWeakPtr()));
  frame_sink_ = frame_sink->GetWeakPtr();
  AllocateLocalSurfaceId();
  return std::move(frame_sink);
}

void WindowPortLocal::AllocateLocalSurfaceId() {
  if (!parent_local_surface_id_allocator_)
    parent_local_surface_id_allocator_ = viz::ParentLocalSurfaceIdAllocator();
  else
    parent_local_surface_id_allocator_->GenerateId();
  UpdateLocalSurfaceId();
}

bool WindowPortLocal::IsLocalSurfaceIdAllocationSuppressed() const {
  return parent_local_surface_id_allocator_ &&
         parent_local_surface_id_allocator_->is_allocation_suppressed();
}

viz::ScopedSurfaceIdAllocator WindowPortLocal::GetSurfaceIdAllocator(
    base::OnceCallback<void()> allocation_task) {
  return viz::ScopedSurfaceIdAllocator(
      &parent_local_surface_id_allocator_.value(), std::move(allocation_task));
}

void WindowPortLocal::UpdateLocalSurfaceIdFromEmbeddedClient(
    const viz::LocalSurfaceId& embedded_client_local_surface_id) {
  parent_local_surface_id_allocator_->UpdateFromChild(
      embedded_client_local_surface_id);
  UpdateLocalSurfaceId();
}

const viz::LocalSurfaceId& WindowPortLocal::GetLocalSurfaceId() {
  if (!parent_local_surface_id_allocator_)
    AllocateLocalSurfaceId();
  return GetCurrentLocalSurfaceId();
}

void WindowPortLocal::OnEventTargetingPolicyChanged() {}

void WindowPortLocal::OnSurfaceChanged(const viz::SurfaceInfo& surface_info) {
  DCHECK_EQ(surface_info.id().frame_sink_id(), window_->GetFrameSinkId());
  DCHECK_EQ(surface_info.id().local_surface_id(), GetCurrentLocalSurfaceId());
  window_->layer()->SetShowPrimarySurface(
      surface_info.id(), window_->bounds().size(), SK_ColorWHITE,
      cc::DeadlinePolicy::UseDefaultDeadline(),
      false /* stretch_content_to_fill_bounds */);
  window_->layer()->SetFallbackSurfaceId(surface_info.id());
}

bool WindowPortLocal::ShouldRestackTransientChildren() {
  return true;
}

void WindowPortLocal::UpdateLocalSurfaceId() {
  last_device_scale_factor_ = ui::GetScaleFactorForNativeView(window_);
  last_size_ = window_->bounds().size();
  if (frame_sink_)
    frame_sink_->SetLocalSurfaceId(GetCurrentLocalSurfaceId());
}

const viz::LocalSurfaceId& WindowPortLocal::GetCurrentLocalSurfaceId() const {
  return parent_local_surface_id_allocator_->GetCurrentLocalSurfaceId();
}

bool WindowPortLocal::IsEmbeddingExternalContent() const {
  return parent_local_surface_id_allocator_.has_value();
}

}  // namespace aura
