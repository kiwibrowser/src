// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_DISPLAY_MANAGER_DISPLAY_UTIL_H_
#define UI_DISPLAY_MANAGER_DISPLAY_UTIL_H_

#include <string>
#include <vector>

#include "third_party/cros_system_api/dbus/service_constants.h"
#include "ui/display/manager/display_manager_export.h"
#include "ui/display/types/display_constants.h"

namespace display {

class DisplaySnapshot;
class ManagedDisplayMode;

// Returns a string describing |state|.
std::string DisplayPowerStateToString(chromeos::DisplayPowerState state);

// Returns a string describing |state|.
std::string MultipleDisplayStateToString(MultipleDisplayState state);

// Returns the number of displays in |displays| that should be turned on, per
// |state|.  If |display_power| is non-NULL, it is updated to contain the
// on/off state of each corresponding entry in |displays|.
int DISPLAY_MANAGER_EXPORT
GetDisplayPower(const std::vector<DisplaySnapshot*>& displays,
                chromeos::DisplayPowerState state,
                std::vector<bool>* display_power);

// Returns whether the DisplayConnectionType |type| is a physically connected
// display. Currently only DISPLAY_CONNECTION_TYPE_NETWORK return false.
// All other types return true.
bool IsPhysicalDisplayType(DisplayConnectionType type);

// Returns a list of display zooms supported by the given |mode|.
std::vector<float> DISPLAY_MANAGER_EXPORT
GetDisplayZoomFactors(const ManagedDisplayMode& mode);

// Returns a list of display zooms based on the provided |dsf| of the display.
// This is useful for displays that have a non unity device scale factors
// applied to them.
std::vector<float> DISPLAY_MANAGER_EXPORT GetDisplayZoomFactorForDsf(float dsf);

// This function adds |dsf| to the vector of |zoom_values| by replacing
// the element it is closest to in the list. It also ensures that it never
// replaces the default zoom value of 1.0 from the list and that the size of the
// list never changes.
// TODO(malaykeshav): Remove this after a few milestones.
void DISPLAY_MANAGER_EXPORT InsertDsfIntoList(std::vector<float>* zoom_values,
                                              float dsf);

}  // namespace display

#endif  // UI_DISPLAY_MANAGER_DISPLAY_UTIL_H_
