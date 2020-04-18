// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// The accelerator priority functions are intended to distinguish between
// accelerators that should preserve the built-in Chrome keybinding semantics
// (normal) and accelerators that should always override web page key handling
// (high). High priority is used for all accelerators assigned to extensions,
// except for the Ctrl+D bookmarking shortcut which is assigned normal priority
// when bound by an extension that requests the privilege to override the
// bookmark UI. This is currently the only case where we allow an extension to
// replace a built-in Chrome accelerator, and it should have the same behavior
// as the built-in accelerator.

#ifndef CHROME_BROWSER_UI_EXTENSIONS_ACCELERATOR_PRIORITY_H_
#define CHROME_BROWSER_UI_EXTENSIONS_ACCELERATOR_PRIORITY_H_

#include <string>

#include "ui/base/accelerators/accelerator_manager.h"

namespace content {
class BrowserContext;
}

namespace extensions {
class Extension;
}

namespace ui {
class Accelerator;
}

// Returns the registration priority to be used when registering |accelerator|
// for |extension|.
ui::AcceleratorManager::HandlerPriority GetAcceleratorPriority(
    const ui::Accelerator& accelerator,
    const extensions::Extension* extension);

// Returns the registration priority to be used when registering |accelerator|
// for the extension with ID |extension_id|.
ui::AcceleratorManager::HandlerPriority GetAcceleratorPriorityById(
    const ui::Accelerator& accelerator,
    const std::string& extension_id,
    content::BrowserContext* browser_context);

#endif  // CHROME_BROWSER_UI_EXTENSIONS_ACCELERATOR_PRIORITY_H_
