// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/display/manager/query_content_protection_task.h"

#include "ui/display/manager/display_layout_manager.h"
#include "ui/display/types/display_snapshot.h"
#include "ui/display/types/native_display_delegate.h"

namespace display {

QueryContentProtectionTask::QueryContentProtectionTask(
    DisplayLayoutManager* layout_manager,
    NativeDisplayDelegate* native_display_delegate,
    int64_t display_id,
    const ResponseCallback& callback)
    : layout_manager_(layout_manager),
      native_display_delegate_(native_display_delegate),
      display_id_(display_id),
      callback_(callback),
      pending_requests_(0),
      weak_ptr_factory_(this) {}

QueryContentProtectionTask::~QueryContentProtectionTask() {}

void QueryContentProtectionTask::Run() {
  std::vector<DisplaySnapshot*> hdcp_capable_displays;
  for (DisplaySnapshot* display : layout_manager_->GetDisplayStates()) {
    // Query display if it is in mirror mode or client on the same display.
    if (!layout_manager_->IsMirroring() && display->display_id() != display_id_)
      continue;

    response_.link_mask |= display->type();

    switch (display->type()) {
      case DISPLAY_CONNECTION_TYPE_UNKNOWN:
        callback_.Run(response_);
        return;
      case DISPLAY_CONNECTION_TYPE_DISPLAYPORT:
      case DISPLAY_CONNECTION_TYPE_DVI:
      case DISPLAY_CONNECTION_TYPE_HDMI:
        hdcp_capable_displays.push_back(display);
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

  response_.success = true;
  pending_requests_ = hdcp_capable_displays.size();
  if (pending_requests_ != 0) {
    for (DisplaySnapshot* display : hdcp_capable_displays) {
      native_display_delegate_->GetHDCPState(
          *display, base::Bind(&QueryContentProtectionTask::OnHDCPStateUpdate,
                               weak_ptr_factory_.GetWeakPtr()));
    }
  } else {
    callback_.Run(response_);
  }
}

void QueryContentProtectionTask::OnHDCPStateUpdate(bool success,
                                                   HDCPState state) {
  response_.success &= success;
  if (state == HDCP_STATE_ENABLED)
    response_.enabled |= CONTENT_PROTECTION_METHOD_HDCP;
  else
    response_.unfulfilled |= CONTENT_PROTECTION_METHOD_HDCP;

  pending_requests_--;
  // Wait for all the requests to finish before invoking the callback.
  if (pending_requests_ != 0)
    return;

  callback_.Run(response_);
}

}  // namespace display
