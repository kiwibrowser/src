// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/system/palette/palette_ids.h"
#include "base/logging.h"

namespace ash {

std::string PaletteToolIdToString(PaletteToolId tool_id) {
  switch (tool_id) {
    case PaletteToolId::NONE:
      return "NONE";
    case PaletteToolId::CREATE_NOTE:
      return "CREATE_NOTE";
    case PaletteToolId::CAPTURE_REGION:
      return "CAPTURE_REGION";
    case PaletteToolId::CAPTURE_SCREEN:
      return "CAPTURE_SCREEN";
    case PaletteToolId::LASER_POINTER:
      return "LASER_POINTER";
    case PaletteToolId::MAGNIFY:
      return "MAGNIFY";
    case PaletteToolId::METALAYER:
      return "METALAYER";
  }

  NOTREACHED();
  return std::string();
}

std::string PaletteGroupToString(PaletteGroup group) {
  switch (group) {
    case PaletteGroup::ACTION:
      return "ACTION";
    case PaletteGroup::MODE:
      return "MODE";
  }

  NOTREACHED();
  return std::string();
}

PaletteTrayOptions PaletteToolIdToPaletteTrayOptions(PaletteToolId tool_id) {
  switch (tool_id) {
    case PaletteToolId::NONE:
      return PALETTE_OPTIONS_COUNT;
    case PaletteToolId::CREATE_NOTE:
      return PALETTE_NEW_NOTE;
    case PaletteToolId::CAPTURE_REGION:
      return PALETTE_CAPTURE_REGION;
    case PaletteToolId::CAPTURE_SCREEN:
      return PALETTE_CAPTURE_SCREEN;
    case PaletteToolId::LASER_POINTER:
      return PALETTE_LASER_POINTER;
    case PaletteToolId::MAGNIFY:
      return PALETTE_MAGNIFY;
    case PaletteToolId::METALAYER:
      return PALETTE_METALAYER;
  }

  NOTREACHED();
  return PALETTE_OPTIONS_COUNT;
}

PaletteModeCancelType PaletteToolIdToPaletteModeCancelType(
    PaletteToolId tool_id,
    bool is_switched) {
  PaletteModeCancelType type = PALETTE_MODE_CANCEL_TYPE_COUNT;
  if (tool_id == PaletteToolId::LASER_POINTER) {
    return is_switched ? PALETTE_MODE_LASER_POINTER_SWITCHED
                       : PALETTE_MODE_LASER_POINTER_CANCELLED;
  } else if (tool_id == PaletteToolId::MAGNIFY) {
    return is_switched ? PALETTE_MODE_MAGNIFY_SWITCHED
                       : PALETTE_MODE_MAGNIFY_CANCELLED;
  }
  return type;
}

}  // namespace ash
