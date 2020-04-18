// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/cocoa/extensions/extension_keybinding_registry_cocoa.h"

#include "chrome/browser/chrome_notification_types.h"
#include "chrome/browser/extensions/api/commands/command_service.h"
#include "chrome/browser/extensions/extension_service.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/extensions/accelerator_priority.h"
#include "content/public/browser/notification_service.h"
#include "extensions/common/extension.h"
#include "extensions/common/manifest_constants.h"
#include "ui/content_accelerators/accelerator_util.h"

namespace values = extensions::manifest_values;

ExtensionKeybindingRegistryCocoa::ExtensionKeybindingRegistryCocoa(
    Profile* profile,
    gfx::NativeWindow window,
    ExtensionFilter extension_filter,
    Delegate* delegate)
    : ExtensionKeybindingRegistry(profile, extension_filter, delegate),
      profile_(profile),
      window_(window) {
  Init();
}

ExtensionKeybindingRegistryCocoa::~ExtensionKeybindingRegistryCocoa() {
}

bool ExtensionKeybindingRegistryCocoa::ProcessKeyEvent(
    const content::NativeWebKeyboardEvent& event,
    ui::AcceleratorManager::HandlerPriority priority) {
  if (shortcut_handling_suspended())
    return false;

  ui::Accelerator accelerator =
      ui::GetAcceleratorFromNativeWebKeyboardEvent(event);

  std::string extension_id;
  std::string command_name;
  if (!GetFirstTarget(accelerator, &extension_id, &command_name))
    return false;

  const ui::AcceleratorManager::HandlerPriority accelerator_priority =
      GetAcceleratorPriorityById(accelerator, extension_id, profile_);
  // Only handle the event if it has the right priority.
  if (priority != accelerator_priority)
    return false;

  int type = 0;
  if (command_name == values::kPageActionCommandEvent) {
    type = extensions::NOTIFICATION_EXTENSION_COMMAND_PAGE_ACTION_MAC;
  } else if (command_name == values::kBrowserActionCommandEvent) {
    type = extensions::NOTIFICATION_EXTENSION_COMMAND_BROWSER_ACTION_MAC;
  } else {
    // Not handled by using notifications. Route it through the Browser Event
    // Router using the base class (it will iterate through all targets).
    return ExtensionKeybindingRegistry::NotifyEventTargets(accelerator);
  }

  // Type != named command, so we need to dispatch this event directly.
  std::pair<const std::string, gfx::NativeWindow> details =
      std::make_pair(extension_id, window_);
  content::NotificationService::current()->Notify(
      type,
      content::Source<Profile>(profile_),
      content::Details<
          std::pair<const std::string, gfx::NativeWindow> >(&details));
  return true;
}

void ExtensionKeybindingRegistryCocoa::AddExtensionKeybindings(
    const extensions::Extension* extension,
    const std::string& command_name) {
  extensions::CommandService* command_service =
      extensions::CommandService::Get(profile_);
  extensions::CommandMap commands;
  command_service->GetNamedCommands(
          extension->id(),
          extensions::CommandService::ACTIVE,
          extensions::CommandService::REGULAR,
          &commands);

  for (extensions::CommandMap::const_iterator iter = commands.begin();
       iter != commands.end(); ++iter) {
    if (!command_name.empty() && (iter->second.command_name() != command_name))
      continue;

    AddEventTarget(iter->second.accelerator(),
                   extension->id(),
                   iter->second.command_name());
  }

  // The Mac implementation keeps track of browser and page actions in the
  // event_targets_ map.
  if (command_name.empty() ||
      command_name == extensions::manifest_values::kBrowserActionCommandEvent) {
    // Add the browser action (if any).
    extensions::Command browser_action;
    if (command_service->GetBrowserActionCommand(
            extension->id(),
            extensions::CommandService::ACTIVE,
            &browser_action,
            NULL)) {
      AddEventTarget(browser_action.accelerator(),
                     extension->id(),
                     browser_action.command_name());
    }
  }

  if (command_name.empty() ||
      command_name == extensions::manifest_values::kPageActionCommandEvent) {
    // Add the page action (if any).
    extensions::Command page_action;
    if (command_service->GetPageActionCommand(
            extension->id(),
            extensions::CommandService::ACTIVE,
            &page_action,
            NULL)) {
      AddEventTarget(page_action.accelerator(),
                     extension->id(),
                     page_action.command_name());
    }
  }
}

void ExtensionKeybindingRegistryCocoa::RemoveExtensionKeybindingImpl(
    const ui::Accelerator& accelerator,
    const std::string& command_name) {
}
