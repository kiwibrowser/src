// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/command_updater_impl.h"

#include <algorithm>

#include "base/logging.h"
#include "base/observer_list.h"
#include "chrome/browser/command_observer.h"
#include "chrome/browser/command_updater_delegate.h"

class CommandUpdaterImpl::Command {
 public:
  bool enabled;
  base::ObserverList<CommandObserver> observers;

  Command() : enabled(true) {}
};

CommandUpdaterImpl::CommandUpdaterImpl(CommandUpdaterDelegate* delegate)
    : delegate_(delegate) {
}

CommandUpdaterImpl::~CommandUpdaterImpl() {
}

bool CommandUpdaterImpl::SupportsCommand(int id) const {
  return commands_.find(id) != commands_.end();
}

bool CommandUpdaterImpl::IsCommandEnabled(int id) const {
  auto command = commands_.find(id);
  if (command == commands_.end())
    return false;
  return command->second->enabled;
}

bool CommandUpdaterImpl::ExecuteCommand(int id) {
  return ExecuteCommandWithDisposition(id, WindowOpenDisposition::CURRENT_TAB);
}

bool CommandUpdaterImpl::ExecuteCommandWithDisposition(
    int id,
    WindowOpenDisposition disposition) {
  if (SupportsCommand(id) && IsCommandEnabled(id)) {
    delegate_->ExecuteCommandWithDisposition(id, disposition);
    return true;
  }
  return false;
}

void CommandUpdaterImpl::AddCommandObserver(int id, CommandObserver* observer) {
  GetCommand(id, true)->observers.AddObserver(observer);
}

void CommandUpdaterImpl::RemoveCommandObserver(
    int id, CommandObserver* observer) {
  GetCommand(id, false)->observers.RemoveObserver(observer);
}

void CommandUpdaterImpl::RemoveCommandObserver(CommandObserver* observer) {
  for (const auto& command_pair : commands_) {
    Command* command = command_pair.second.get();
    if (command)
      command->observers.RemoveObserver(observer);
  }
}

bool CommandUpdaterImpl::UpdateCommandEnabled(int id, bool enabled) {
  Command* command = GetCommand(id, true);
  if (command->enabled == enabled)
    return true;  // Nothing to do.
  command->enabled = enabled;
  for (auto& observer : command->observers)
    observer.EnabledStateChangedForCommand(id, enabled);
  return true;
}

void CommandUpdaterImpl::DisableAllCommands() {
  for (const auto& command_pair : commands_)
    UpdateCommandEnabled(command_pair.first, false);
}

std::vector<int> CommandUpdaterImpl::GetAllIds() {
  std::vector<int> result;
  for (const auto& command_pair : commands_)
    result.push_back(command_pair.first);
  return result;
}

CommandUpdaterImpl::Command*
CommandUpdaterImpl::GetCommand(int id, bool create) {
  bool supported = SupportsCommand(id);
  if (supported)
    return commands_[id].get();

  DCHECK(create);
  std::unique_ptr<Command>& entry = commands_[id];
  entry = std::make_unique<Command>();
  return entry.get();
}
