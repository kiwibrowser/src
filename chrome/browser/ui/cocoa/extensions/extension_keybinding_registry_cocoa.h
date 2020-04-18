// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_COCOA_EXTENSIONS_EXTENSION_KEYBINDING_REGISTRY_COCOA_H_
#define CHROME_BROWSER_UI_COCOA_EXTENSIONS_EXTENSION_KEYBINDING_REGISTRY_COCOA_H_

#include <string>
#include <utility>

#include "base/macros.h"
#include "chrome/browser/extensions/extension_keybinding_registry.h"
#include "ui/base/accelerators/accelerator.h"
#include "ui/base/accelerators/accelerator_manager.h"
#include "ui/gfx/native_widget_types.h"

class Profile;

namespace content {
  struct NativeWebKeyboardEvent;
}
namespace extensions {
  class Extension;
}

// The ExtensionKeybindingRegistryCocoa is the Cocoa specialization of the
// ExtensionKeybindingRegistry class that handles turning keyboard shortcuts
// into events that get sent to the extension.

// ExtensionKeybindingRegistryCocoa is a class that handles Cocoa-specific
// implemenation of the Extension Commands shortcuts (keyboard accelerators).
// It also routes the events to the intended recipient (ie. to the browser
// action button in case of browser action commands).
class ExtensionKeybindingRegistryCocoa
    : public extensions::ExtensionKeybindingRegistry {
 public:
  ExtensionKeybindingRegistryCocoa(Profile* profile,
                                   gfx::NativeWindow window,
                                   ExtensionFilter extension_filter,
                                   Delegate* delegate);
  ~ExtensionKeybindingRegistryCocoa() override;

  // For a given keyboard |event|, see if a known Extension Command registration
  // exists and route the event to it. Returns true if the event was handled,
  // false otherwise.
  bool ProcessKeyEvent(const content::NativeWebKeyboardEvent& event,
                       ui::AcceleratorManager::HandlerPriority priority);

 protected:
  // Overridden from ExtensionKeybindingRegistry:
  void AddExtensionKeybindings(const extensions::Extension* extension,
                               const std::string& command_name) override;
  void RemoveExtensionKeybindingImpl(const ui::Accelerator& accelerator,
                                     const std::string& command_name) override;

 private:
  // Weak pointer to the our profile. Not owned by us.
  Profile* profile_;

  // The window we are associated with.
  gfx::NativeWindow window_;

  // The content notification registrar for listening to extension events.
  content::NotificationRegistrar registrar_;

  DISALLOW_COPY_AND_ASSIGN(ExtensionKeybindingRegistryCocoa);
};

#endif  // CHROME_BROWSER_UI_COCOA_EXTENSIONS_EXTENSION_KEYBINDING_REGISTRY_COCOA_H_
