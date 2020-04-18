// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_UI_WS_IDS_H_
#define SERVICES_UI_WS_IDS_H_

#include <stdint.h>

#include "components/viz/common/surfaces/frame_sink_id.h"
#include "services/ui/common/types.h"

namespace ui {
namespace ws {

// See comments in README.md (services/ui/ws/README.md) for details on ids.

// A client id used to indicate no client. That is, no WindowTree ever gets this
// id.
const ClientSpecificId kInvalidClientId = 0;

const Id kInvalidTransportId = 0;

// Client id used to indicate the window was created by the WindowServer.
const ClientSpecificId kWindowServerClientId = 1;

// Used for ids assigned by the client.
using ClientWindowId = viz::FrameSinkId;

using ClientWindowIdHash = viz::FrameSinkIdHash;

}  // namespace ws
}  // namespace ui

#endif  // SERVICES_UI_WS_IDS_H_
