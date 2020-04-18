// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/extensions/api/commands/commands.h"

#include <memory>
#include <utility>

#include "chrome/browser/extensions/api/commands/command_service.h"
#include "chrome/browser/profiles/profile.h"

namespace {

std::unique_ptr<base::DictionaryValue> CreateCommandValue(
    const extensions::Command& command,
    bool active) {
  std::unique_ptr<base::DictionaryValue> result(new base::DictionaryValue());
  result->SetString("name", command.command_name());
  result->SetString("description", command.description());
  result->SetString("shortcut",
                    active ? command.accelerator().GetShortcutText() :
                             base::string16());
  return result;
}

}  // namespace

ExtensionFunction::ResponseAction GetAllCommandsFunction::Run() {
  std::unique_ptr<base::ListValue> command_list(new base::ListValue());

  extensions::CommandService* command_service =
      extensions::CommandService::Get(browser_context());

  extensions::Command browser_action;
  bool active = false;
  if (command_service->GetBrowserActionCommand(extension_->id(),
          extensions::CommandService::ALL,
          &browser_action,
          &active)) {
    command_list->Append(CreateCommandValue(browser_action, active));
  }

  extensions::Command page_action;
  if (command_service->GetPageActionCommand(extension_->id(),
          extensions::CommandService::ALL,
          &page_action,
          &active)) {
    command_list->Append(CreateCommandValue(page_action, active));
  }

  extensions::CommandMap named_commands;
  command_service->GetNamedCommands(extension_->id(),
                                    extensions::CommandService::ALL,
                                    extensions::CommandService::ANY_SCOPE,
                                    &named_commands);

  for (extensions::CommandMap::const_iterator iter = named_commands.begin();
       iter != named_commands.end(); ++iter) {
    extensions::Command command = command_service->FindCommandByName(
        extension_->id(), iter->second.command_name());
    ui::Accelerator shortcut_assigned = command.accelerator();
    active = (shortcut_assigned.key_code() != ui::VKEY_UNKNOWN);

    command_list->Append(CreateCommandValue(iter->second, active));
  }

  return RespondNow(OneArgument(std::move(command_list)));
}
