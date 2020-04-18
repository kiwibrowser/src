// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/display/mojo/display_constants_struct_traits.h"

namespace mojo {

display::mojom::DisplayConnectionType EnumTraits<
    display::mojom::DisplayConnectionType,
    display::DisplayConnectionType>::ToMojom(display::DisplayConnectionType
                                                 type) {
  switch (type) {
    case display::DisplayConnectionType::DISPLAY_CONNECTION_TYPE_NONE:
      return display::mojom::DisplayConnectionType::
          DISPLAY_CONNECTION_TYPE_NONE;
    case display::DisplayConnectionType::DISPLAY_CONNECTION_TYPE_UNKNOWN:
      return display::mojom::DisplayConnectionType::
          DISPLAY_CONNECTION_TYPE_UNKNOWN;
    case display::DisplayConnectionType::DISPLAY_CONNECTION_TYPE_INTERNAL:
      return display::mojom::DisplayConnectionType::
          DISPLAY_CONNECTION_TYPE_INTERNAL;
    case display::DisplayConnectionType::DISPLAY_CONNECTION_TYPE_VGA:
      return display::mojom::DisplayConnectionType::DISPLAY_CONNECTION_TYPE_VGA;
    case display::DisplayConnectionType::DISPLAY_CONNECTION_TYPE_HDMI:
      return display::mojom::DisplayConnectionType::
          DISPLAY_CONNECTION_TYPE_HDMI;
    case display::DisplayConnectionType::DISPLAY_CONNECTION_TYPE_DVI:
      return display::mojom::DisplayConnectionType::DISPLAY_CONNECTION_TYPE_DVI;
    case display::DisplayConnectionType::DISPLAY_CONNECTION_TYPE_DISPLAYPORT:
      return display::mojom::DisplayConnectionType::
          DISPLAY_CONNECTION_TYPE_DISPLAYPORT;
    case display::DisplayConnectionType::DISPLAY_CONNECTION_TYPE_NETWORK:
      return display::mojom::DisplayConnectionType::
          DISPLAY_CONNECTION_TYPE_NETWORK;
  }
  NOTREACHED();
  return display::mojom::DisplayConnectionType::DISPLAY_CONNECTION_TYPE_NONE;
}

bool EnumTraits<display::mojom::DisplayConnectionType,
                display::DisplayConnectionType>::
    FromMojom(display::mojom::DisplayConnectionType type,
              display::DisplayConnectionType* out) {
  switch (type) {
    case display::mojom::DisplayConnectionType::DISPLAY_CONNECTION_TYPE_NONE:
      *out = display::DisplayConnectionType::DISPLAY_CONNECTION_TYPE_NONE;
      return true;
    case display::mojom::DisplayConnectionType::DISPLAY_CONNECTION_TYPE_UNKNOWN:
      *out = display::DisplayConnectionType::DISPLAY_CONNECTION_TYPE_UNKNOWN;
      return true;
    case display::mojom::DisplayConnectionType::
        DISPLAY_CONNECTION_TYPE_INTERNAL:
      *out = display::DisplayConnectionType::DISPLAY_CONNECTION_TYPE_INTERNAL;
      return true;
    case display::mojom::DisplayConnectionType::DISPLAY_CONNECTION_TYPE_VGA:
      *out = display::DisplayConnectionType::DISPLAY_CONNECTION_TYPE_VGA;
      return true;
    case display::mojom::DisplayConnectionType::DISPLAY_CONNECTION_TYPE_HDMI:
      *out = display::DisplayConnectionType::DISPLAY_CONNECTION_TYPE_HDMI;
      return true;
    case display::mojom::DisplayConnectionType::DISPLAY_CONNECTION_TYPE_DVI:
      *out = display::DisplayConnectionType::DISPLAY_CONNECTION_TYPE_DVI;
      return true;
    case display::mojom::DisplayConnectionType::
        DISPLAY_CONNECTION_TYPE_DISPLAYPORT:
      *out =
          display::DisplayConnectionType::DISPLAY_CONNECTION_TYPE_DISPLAYPORT;
      return true;
    case display::mojom::DisplayConnectionType::DISPLAY_CONNECTION_TYPE_NETWORK:
      *out = display::DisplayConnectionType::DISPLAY_CONNECTION_TYPE_NETWORK;
      return true;
  }
  return false;
}

// static
display::mojom::HDCPState
EnumTraits<display::mojom::HDCPState, display::HDCPState>::ToMojom(
    display::HDCPState type) {
  switch (type) {
    case display::HDCPState::HDCP_STATE_UNDESIRED:
      return display::mojom::HDCPState::HDCP_STATE_UNDESIRED;
    case display::HDCPState::HDCP_STATE_DESIRED:
      return display::mojom::HDCPState::HDCP_STATE_DESIRED;
    case display::HDCPState::HDCP_STATE_ENABLED:
      return display::mojom::HDCPState::HDCP_STATE_ENABLED;
  }
  NOTREACHED();
  return display::mojom::HDCPState::HDCP_STATE_UNDESIRED;
}

// static
bool EnumTraits<display::mojom::HDCPState, display::HDCPState>::FromMojom(
    display::mojom::HDCPState type,
    display::HDCPState* out) {
  switch (type) {
    case display::mojom::HDCPState::HDCP_STATE_UNDESIRED:
      *out = display::HDCPState::HDCP_STATE_UNDESIRED;
      return true;
    case display::mojom::HDCPState::HDCP_STATE_DESIRED:
      *out = display::HDCPState::HDCP_STATE_DESIRED;
      return true;
    case display::mojom::HDCPState::HDCP_STATE_ENABLED:
      *out = display::HDCPState::HDCP_STATE_ENABLED;
      return true;
  }
  return false;
}

}  // namespace mojo
