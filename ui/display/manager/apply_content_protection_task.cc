// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/display/manager/apply_content_protection_task.h"

#include "ui/display/manager/display_layout_manager.h"
#include "ui/display/types/display_snapshot.h"
#include "ui/display/types/native_display_delegate.h"

namespace display {

namespace {

bool GetHDCPCapableDisplays(
    const DisplayLayoutManager& layout_manager,
    std::vector<DisplaySnapshot*>* hdcp_capable_displays) {
  for (DisplaySnapshot* display : layout_manager.GetDisplayStates()) {
    switch (display->type()) {
      case DISPLAY_CONNECTION_TYPE_UNKNOWN:
        return false;
      // DisplayPort, DVI, and HDMI all support HDCP.
      case DISPLAY_CONNECTION_TYPE_DISPLAYPORT:
      case DISPLAY_CONNECTION_TYPE_DVI:
      case DISPLAY_CONNECTION_TYPE_HDMI:
        hdcp_capable_displays->push_back(display);
        break;
      case DISPLAY_CONNECTION_TYPE_INTERNAL:
      case DISPLAY_CONNECTION_TYPE_VGA:
      case DISPLAY_CONNECTION_TYPE_NETWORK:
        // No protections for these types. Do nothing.
        break;
      case DISPLAY_CONNECTION_TYPE_NONE:
        NOTREACHED();
        break;
    }
  }

  return true;
}

}  // namespace

ApplyContentProtectionTask::ApplyContentProtectionTask(
    DisplayLayoutManager* layout_manager,
    NativeDisplayDelegate* native_display_delegate,
    const DisplayConfigurator::ContentProtections& requests,
    const ResponseCallback& callback)
    : layout_manager_(layout_manager),
      native_display_delegate_(native_display_delegate),
      requests_(requests),
      callback_(callback),
      query_status_(true),
      pending_requests_(0),
      weak_ptr_factory_(this) {}

ApplyContentProtectionTask::~ApplyContentProtectionTask() {}

void ApplyContentProtectionTask::Run() {
  std::vector<DisplaySnapshot*> hdcp_capable_displays;
  if (!GetHDCPCapableDisplays(*layout_manager_, &hdcp_capable_displays)) {
    callback_.Run(false);
    return;
  }

  pending_requests_ = hdcp_capable_displays.size();
  if (pending_requests_ == 0) {
    callback_.Run(true);
    return;
  }

  // Need to poll the driver for updates since other applications may have
  // updated the state.
  for (DisplaySnapshot* display : hdcp_capable_displays) {
    native_display_delegate_->GetHDCPState(
        *display,
        base::Bind(&ApplyContentProtectionTask::OnHDCPStateUpdate,
                   weak_ptr_factory_.GetWeakPtr(), display->display_id()));
  }
}

void ApplyContentProtectionTask::OnHDCPStateUpdate(int64_t display_id,
                                                   bool success,
                                                   HDCPState state) {
  query_status_ &= success;
  display_hdcp_state_map_[display_id] = state;
  pending_requests_--;

  // Wait for all the requests before continuing.
  if (pending_requests_ != 0)
    return;

  if (!query_status_) {
    callback_.Run(false);
    return;
  }

  ApplyProtections();
}

void ApplyContentProtectionTask::ApplyProtections() {
  std::vector<DisplaySnapshot*> hdcp_capable_displays;
  if (!GetHDCPCapableDisplays(*layout_manager_, &hdcp_capable_displays)) {
    callback_.Run(false);
    return;
  }

  std::vector<std::pair<DisplaySnapshot*, HDCPState>> hdcp_requests;
  // Figure out which displays need to have their HDCP state changed.
  for (DisplaySnapshot* display : hdcp_capable_displays) {
    uint32_t desired_mask = GetDesiredProtectionMask(display->display_id());

    auto it = display_hdcp_state_map_.find(display->display_id());
    // If the display can't be found, the display configuration changed.
    if (it == display_hdcp_state_map_.end()) {
      callback_.Run(false);
      return;
    }

    bool hdcp_enabled = it->second != HDCP_STATE_UNDESIRED;
    bool hdcp_requested = desired_mask & CONTENT_PROTECTION_METHOD_HDCP;
    if (hdcp_enabled != hdcp_requested) {
      hdcp_requests.push_back(std::make_pair(
          display, hdcp_requested ? HDCP_STATE_DESIRED : HDCP_STATE_UNDESIRED));
    }
  }

  pending_requests_ = hdcp_requests.size();
  // All the requested changes are the same as the current HDCP state. Nothing
  // to do anymore, just ack the content protection change.
  if (pending_requests_ == 0) {
    callback_.Run(true);
    return;
  }

  for (const auto& pair : hdcp_requests) {
    native_display_delegate_->SetHDCPState(
        *pair.first, pair.second,
        base::Bind(&ApplyContentProtectionTask::OnHDCPStateApplied,
                   weak_ptr_factory_.GetWeakPtr()));
  }
}

void ApplyContentProtectionTask::OnHDCPStateApplied(bool success) {
  query_status_ &= success;
  pending_requests_--;

  if (pending_requests_ == 0)
    callback_.Run(query_status_);
}

uint32_t ApplyContentProtectionTask::GetDesiredProtectionMask(
    int64_t display_id) const {
  uint32_t desired_mask = 0;
  // In mirror mode, protection request of all displays need to be fulfilled.
  // In non-mirror mode, only request of client's display needs to be
  // fulfilled.
  if (layout_manager_->IsMirroring()) {
    for (auto pair : requests_)
      desired_mask |= pair.second;
  } else {
    auto it = requests_.find(display_id);
    if (it != requests_.end())
      desired_mask = it->second;
  }

  return desired_mask;
}

}  // namespace display
