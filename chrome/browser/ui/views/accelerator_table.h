// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_VIEWS_ACCELERATOR_TABLE_H_
#define CHROME_BROWSER_UI_VIEWS_ACCELERATOR_TABLE_H_

#include <vector>

#include "chrome/browser/ui/views/chrome_views_export.h"
#include "ui/events/keycodes/keyboard_codes.h"

namespace ui {
class Accelerator;
}

// This contains the list of accelerators for the Aura implementation.
struct AcceleratorMapping {
  ui::KeyboardCode keycode;
  int modifiers;
  int command_id;
};

// Returns a list of accelerator mapping information for accelerators
// handled by Chrome but excluding accelerators handled by Ash.
CHROME_VIEWS_EXPORT std::vector<AcceleratorMapping> GetAcceleratorList();

// Returns true on Ash and if the command id has an associated accelerator which
// is handled by Ash. If the return is true the accelerator is returned via the
// second argument.
CHROME_VIEWS_EXPORT bool GetAshAcceleratorForCommandId(
    int command_id,
    ui::Accelerator* accelerator);

// Returns true if the command id has an associated standard
// accelerator like cut, copy and paste. If the return is true the
// accelerator is returned via the second argument.
CHROME_VIEWS_EXPORT bool GetStandardAcceleratorForCommandId(
    int command_id,
    ui::Accelerator* accelerator);

// Returns true if the command identified by |command_id| should be executed
// repeatedly while its accelerator keys are held down.
CHROME_VIEWS_EXPORT bool IsCommandRepeatable(int command_id);

#endif  // CHROME_BROWSER_UI_VIEWS_ACCELERATOR_TABLE_H_
