// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/aura/mus/in_flight_change.h"

#include "ui/aura/client/aura_constants.h"
#include "ui/aura/mus/capture_synchronizer.h"
#include "ui/aura/mus/focus_synchronizer.h"
#include "ui/aura/mus/window_mus.h"
#include "ui/aura/mus/window_port_mus.h"
#include "ui/aura/mus/window_tree_client.h"
#include "ui/aura/window.h"
#include "ui/base/ui_base_types.h"

namespace aura {

// InFlightChange -------------------------------------------------------------

InFlightChange::InFlightChange(WindowMus* window, ChangeType type)
    : window_(window), change_type_(type) {}

InFlightChange::~InFlightChange() {}

bool InFlightChange::Matches(const InFlightChange& change) const {
  DCHECK(change.window_ == window_ && change.change_type_ == change_type_);
  return true;
}

void InFlightChange::ChangeFailed() {}

// InFlightBoundsChange -------------------------------------------------------

InFlightBoundsChange::InFlightBoundsChange(
    WindowTreeClient* window_tree_client,
    WindowMus* window,
    const gfx::Rect& revert_bounds,
    const base::Optional<viz::LocalSurfaceId>& revert_local_surface_id)
    : InFlightChange(window, ChangeType::BOUNDS),
      window_tree_client_(window_tree_client),
      revert_bounds_(revert_bounds),
      revert_local_surface_id_(revert_local_surface_id) {}

InFlightBoundsChange::~InFlightBoundsChange() {}

void InFlightBoundsChange::SetRevertValueFrom(const InFlightChange& change) {
  revert_bounds_ =
      static_cast<const InFlightBoundsChange&>(change).revert_bounds_;
  revert_local_surface_id_ =
      static_cast<const InFlightBoundsChange&>(change).revert_local_surface_id_;
}

void InFlightBoundsChange::Revert() {
  window_tree_client_->SetWindowBoundsFromServer(window(), revert_bounds_,
                                                 revert_local_surface_id_);
}

// InFlightDragChange -----------------------------------------------------

InFlightDragChange::InFlightDragChange(WindowMus* window, ChangeType type)
    : InFlightChange(window, type) {
  DCHECK(type == ChangeType::MOVE_LOOP || type == ChangeType::DRAG_LOOP);
}

void InFlightDragChange::SetRevertValueFrom(const InFlightChange& change) {}

void InFlightDragChange::Revert() {}

// InFlightTransformChange -----------------------------------------------------

InFlightTransformChange::InFlightTransformChange(
    WindowTreeClient* window_tree_client,
    WindowMus* window,
    const gfx::Transform& revert_transform)
    : InFlightChange(window, ChangeType::TRANSFORM),
      window_tree_client_(window_tree_client),
      revert_transform_(revert_transform) {}

InFlightTransformChange::~InFlightTransformChange() {}

void InFlightTransformChange::SetRevertValueFrom(const InFlightChange& change) {
  revert_transform_ =
      static_cast<const InFlightTransformChange&>(change).revert_transform_;
}

void InFlightTransformChange::Revert() {
  window_tree_client_->SetWindowTransformFromServer(window(),
                                                    revert_transform_);
}

// CrashInFlightChange --------------------------------------------------------

CrashInFlightChange::CrashInFlightChange(WindowMus* window, ChangeType type)
    : InFlightChange(window, type) {}

CrashInFlightChange::~CrashInFlightChange() {}

void CrashInFlightChange::SetRevertValueFrom(const InFlightChange& change) {
  CHECK(false);
}

void CrashInFlightChange::ChangeFailed() {
  DLOG(ERROR) << "change failed, type=" << static_cast<int>(change_type());
  CHECK(false);
}

void CrashInFlightChange::Revert() {
  CHECK(false);
}

// InFlightWindowChange -------------------------------------------------------

InFlightWindowTreeClientChange::InFlightWindowTreeClientChange(
    WindowTreeClient* client,
    WindowMus* revert_value,
    ChangeType type)
    : InFlightChange(nullptr, type), client_(client), revert_window_(nullptr) {
  SetRevertWindow(revert_value);
}

InFlightWindowTreeClientChange::~InFlightWindowTreeClientChange() {
  SetRevertWindow(nullptr);
}

void InFlightWindowTreeClientChange::SetRevertValueFrom(
    const InFlightChange& change) {
  SetRevertWindow(static_cast<const InFlightWindowTreeClientChange&>(change)
                      .revert_window_);
}

void InFlightWindowTreeClientChange::SetRevertWindow(WindowMus* window) {
  if (revert_window_)
    revert_window_->GetWindow()->RemoveObserver(this);
  revert_window_ = window;
  if (revert_window_)
    revert_window_->GetWindow()->AddObserver(this);
}

void InFlightWindowTreeClientChange::OnWindowDestroyed(Window* window) {
  // NOTE: this has to be in OnWindowDestroyed() as FocusClients typically
  // change focus in OnWindowDestroying().
  SetRevertWindow(nullptr);
}

// InFlightCaptureChange ------------------------------------------------------

InFlightCaptureChange::InFlightCaptureChange(
    WindowTreeClient* client,
    CaptureSynchronizer* capture_synchronizer,
    WindowMus* revert_value)
    : InFlightWindowTreeClientChange(client, revert_value, ChangeType::CAPTURE),
      capture_synchronizer_(capture_synchronizer) {}

InFlightCaptureChange::~InFlightCaptureChange() {}

void InFlightCaptureChange::Revert() {
  capture_synchronizer_->SetCaptureFromServer(revert_window());
}

// InFlightFocusChange --------------------------------------------------------

InFlightFocusChange::InFlightFocusChange(WindowTreeClient* client,
                                         FocusSynchronizer* focus_synchronizer,
                                         WindowMus* revert_value)
    : InFlightWindowTreeClientChange(client, revert_value, ChangeType::FOCUS),
      focus_synchronizer_(focus_synchronizer) {}

InFlightFocusChange::~InFlightFocusChange() {}

void InFlightFocusChange::Revert() {
  focus_synchronizer_->SetFocusFromServer(revert_window());
}

// InFlightPropertyChange -----------------------------------------------------

InFlightPropertyChange::InFlightPropertyChange(
    WindowMus* window,
    const std::string& property_name,
    std::unique_ptr<std::vector<uint8_t>> revert_value)
    : InFlightChange(window, ChangeType::PROPERTY),
      property_name_(property_name),
      revert_value_(std::move(revert_value)) {}

InFlightPropertyChange::~InFlightPropertyChange() {}

bool InFlightPropertyChange::Matches(const InFlightChange& change) const {
  return static_cast<const InFlightPropertyChange&>(change).property_name_ ==
         property_name_;
}

void InFlightPropertyChange::SetRevertValueFrom(const InFlightChange& change) {
  const InFlightPropertyChange& property_change =
      static_cast<const InFlightPropertyChange&>(change);
  if (property_change.revert_value_) {
    revert_value_ =
        std::make_unique<std::vector<uint8_t>>(*property_change.revert_value_);
  } else {
    revert_value_.reset();
  }
}

void InFlightPropertyChange::Revert() {
  window()->SetPropertyFromServer(property_name_, revert_value_.get());
}

// InFlightCursorChange ----------------------------------------------------

InFlightCursorChange::InFlightCursorChange(WindowMus* window,
                                           const ui::CursorData& revert_value)
    : InFlightChange(window, ChangeType::CURSOR),
      revert_cursor_(revert_value) {}

InFlightCursorChange::~InFlightCursorChange() {}

void InFlightCursorChange::SetRevertValueFrom(const InFlightChange& change) {
  revert_cursor_ =
      static_cast<const InFlightCursorChange&>(change).revert_cursor_;
}

void InFlightCursorChange::Revert() {
  window()->SetCursorFromServer(revert_cursor_);
}

// InFlightVisibleChange -------------------------------------------------------

InFlightVisibleChange::InFlightVisibleChange(WindowTreeClient* client,
                                             WindowMus* window,
                                             bool revert_value)
    : InFlightChange(window, ChangeType::VISIBLE),
      window_tree_client_(client),
      revert_visible_(revert_value) {}

InFlightVisibleChange::~InFlightVisibleChange() {}

void InFlightVisibleChange::SetRevertValueFrom(const InFlightChange& change) {
  revert_visible_ =
      static_cast<const InFlightVisibleChange&>(change).revert_visible_;
}

void InFlightVisibleChange::Revert() {
  window_tree_client_->SetWindowVisibleFromServer(window(), revert_visible_);
}

// InFlightOpacityChange -------------------------------------------------------

InFlightOpacityChange::InFlightOpacityChange(WindowMus* window,
                                             float revert_value)
    : InFlightChange(window, ChangeType::OPACITY),
      revert_opacity_(revert_value) {}

InFlightOpacityChange::~InFlightOpacityChange() {}

void InFlightOpacityChange::SetRevertValueFrom(const InFlightChange& change) {
  revert_opacity_ =
      static_cast<const InFlightOpacityChange&>(change).revert_opacity_;
}

void InFlightOpacityChange::Revert() {
  window()->SetOpacityFromServer(revert_opacity_);
}

// InFlightSetModalTypeChange
// ------------------------------------------------------

InFlightSetModalTypeChange::InFlightSetModalTypeChange(
    WindowMus* window,
    ui::ModalType revert_value)
    : InFlightChange(window, ChangeType::SET_MODAL),
      revert_modal_type_(revert_value) {}

InFlightSetModalTypeChange::~InFlightSetModalTypeChange() {}

void InFlightSetModalTypeChange::SetRevertValueFrom(
    const InFlightChange& change) {
  revert_modal_type_ =
      static_cast<const InFlightSetModalTypeChange&>(change).revert_modal_type_;
}

void InFlightSetModalTypeChange::Revert() {
  window()->GetWindow()->SetProperty(client::kModalKey, revert_modal_type_);
}

}  // namespace aura
