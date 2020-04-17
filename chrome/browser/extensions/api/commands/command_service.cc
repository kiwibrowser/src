// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/extensions/api/commands/command_service.h"

#include <memory>
#include <utility>
#include <vector>

#include "base/lazy_instance.h"
#include "base/strings/string_split.h"
#include "base/strings/string_util.h"
#include "base/strings/utf_string_conversions.h"
#include "base/values.h"
#include "chrome/browser/extensions/api/commands/commands.h"
#include "chrome/browser/extensions/extension_commands_global_registry.h"
#include "chrome/browser/extensions/extension_keybinding_registry.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/accelerator_utils.h"
#include "chrome/common/extensions/api/commands/commands_handler.h"
#include "chrome/common/extensions/manifest_handlers/ui_overrides_handler.h"
#include "chrome/common/pref_names.h"
#include "components/pref_registry/pref_registry_syncable.h"
#include "components/prefs/scoped_user_pref_update.h"
#include "content/public/browser/notification_details.h"
#include "content/public/browser/notification_service.h"
#include "extensions/browser/extension_function_registry.h"
#include "extensions/browser/extension_prefs.h"
#include "extensions/browser/extension_registry.h"
#include "extensions/browser/extension_system.h"
#include "extensions/browser/notification_types.h"
#include "extensions/common/feature_switch.h"
#include "extensions/common/manifest_constants.h"
#include "extensions/common/permissions/permissions_data.h"

namespace extensions {
namespace {

const char kExtension[] = "extension";
const char kCommandName[] = "command_name";
const char kGlobal[] = "global";

// Preference key name for saving whether the extension-suggested key was
// actually assigned.
const char kSuggestedKeyWasAssigned[] = "was_assigned";

std::string GetPlatformKeybindingKeyForAccelerator(
    const ui::Accelerator& accelerator, const std::string& extension_id) {
  std::string key = Command::CommandPlatform() + ":" +
                    Command::AcceleratorToString(accelerator);

  // Media keys have a 1-to-many relationship with targets, unlike regular
  // shortcut (1-to-1 relationship). That means two or more extensions can
  // register for the same media key so the extension ID needs to be added to
  // the key to make sure the key is unique.
  if (Command::IsMediaKey(accelerator))
    key += ":" + extension_id;

  return key;
}


// Merge |suggested_key_prefs| into the saved preferences for the extension. We
// merge rather than overwrite to preserve existing was_assigned preferences.
void MergeSuggestedKeyPrefs(
    const std::string& extension_id,
    ExtensionPrefs* extension_prefs,
    std::unique_ptr<base::DictionaryValue> suggested_key_prefs) {
}

}  // namespace

// static
void CommandService::RegisterProfilePrefs(
    user_prefs::PrefRegistrySyncable* registry) {
  registry->RegisterDictionaryPref(
      prefs::kExtensionCommands,
      user_prefs::PrefRegistrySyncable::SYNCABLE_PREF);
}

CommandService::CommandService(content::BrowserContext* context)
    : profile_(Profile::FromBrowserContext(context)),
      extension_registry_observer_(this) {
  ExtensionFunctionRegistry::GetInstance()
      .RegisterFunction<GetAllCommandsFunction>();

  extension_registry_observer_.Add(ExtensionRegistry::Get(profile_));
}

CommandService::~CommandService() {
}

static base::LazyInstance<BrowserContextKeyedAPIFactory<CommandService>>::
    DestructorAtExit g_command_service_factory = LAZY_INSTANCE_INITIALIZER;

// static
BrowserContextKeyedAPIFactory<CommandService>*
CommandService::GetFactoryInstance() {
  return g_command_service_factory.Pointer();
}

// static
CommandService* CommandService::Get(content::BrowserContext* context) {
  return BrowserContextKeyedAPIFactory<CommandService>::Get(context);
}

// static
bool CommandService::RemovesBookmarkShortcut(const Extension* extension) {
  return UIOverrides::RemovesBookmarkShortcut(extension) &&
      (extension->permissions_data()->HasAPIPermission(
          APIPermission::kBookmarkManagerPrivate) ||
       FeatureSwitch::enable_override_bookmarks_ui()->IsEnabled());
}

// static
bool CommandService::RemovesBookmarkOpenPagesShortcut(
    const Extension* extension) {
  return UIOverrides::RemovesBookmarkOpenPagesShortcut(extension) &&
      (extension->permissions_data()->HasAPIPermission(
          APIPermission::kBookmarkManagerPrivate) ||
       FeatureSwitch::enable_override_bookmarks_ui()->IsEnabled());
}

bool CommandService::GetBrowserActionCommand(const std::string& extension_id,
                                             QueryType type,
                                             Command* command,
                                             bool* active) const {
  return GetExtensionActionCommand(extension_id, type, command, active,
                                   Command::Type::kBrowserAction);
}

bool CommandService::GetPageActionCommand(const std::string& extension_id,
                                          QueryType type,
                                          Command* command,
                                          bool* active) const {
  return GetExtensionActionCommand(extension_id, type, command, active,
                                   Command::Type::kPageAction);
}

bool CommandService::GetNamedCommands(const std::string& extension_id,
                                      QueryType type,
                                      CommandScope scope,
                                      CommandMap* command_map) const {
  const ExtensionSet& extensions =
      ExtensionRegistry::Get(profile_)->enabled_extensions();
  const Extension* extension = extensions.GetByID(extension_id);
  CHECK(extension);

  command_map->clear();
  const CommandMap* commands = CommandsInfo::GetNamedCommands(extension);
  if (!commands)
    return false;

  for (CommandMap::const_iterator iter = commands->begin();
       iter != commands->end(); ++iter) {
    // Look up to see if the user has overridden how the command should work.
    Command saved_command =
        FindCommandByName(extension_id, iter->second.command_name());
    ui::Accelerator shortcut_assigned = saved_command.accelerator();

    if (type == ACTIVE && shortcut_assigned.key_code() == ui::VKEY_UNKNOWN)
      continue;

    Command command = iter->second;
    if (scope != ANY_SCOPE && ((scope == GLOBAL) != saved_command.global()))
      continue;

    if (type != SUGGESTED && shortcut_assigned.key_code() != ui::VKEY_UNKNOWN)
      command.set_accelerator(shortcut_assigned);
    command.set_global(saved_command.global());

    (*command_map)[iter->second.command_name()] = command;
  }

  return !command_map->empty();
}

bool CommandService::AddKeybindingPref(
    const ui::Accelerator& accelerator,
    const std::string& extension_id,
    const std::string& command_name,
    bool allow_overrides,
    bool global) {
  if (accelerator.key_code() == ui::VKEY_UNKNOWN)
    return false;

  // Nothing needs to be done if the existing command is the same as the desired
  // new one.
  Command existing_command = FindCommandByName(extension_id, command_name);
  if (existing_command.accelerator() == accelerator &&
      existing_command.global() == global)
    return true;

  // Media Keys are allowed to be used by named command only.
  DCHECK(!Command::IsMediaKey(accelerator) ||
         (command_name != manifest_values::kPageActionCommandEvent &&
          command_name != manifest_values::kBrowserActionCommandEvent));

  DictionaryPrefUpdate updater(profile_->GetPrefs(),
                               prefs::kExtensionCommands);
  base::DictionaryValue* bindings = updater.Get();

  std::string key = GetPlatformKeybindingKeyForAccelerator(accelerator,
                                                           extension_id);

  if (bindings->HasKey(key)) {
    if (!allow_overrides)
      return false;  // Already taken.

    // If the shortcut has been assigned to another command, it should be
    // removed before overriding, so that |ExtensionKeybindingRegistry| can get
    // a chance to do clean-up.
    const base::DictionaryValue* item = NULL;
    bindings->GetDictionary(key, &item);
    std::string old_extension_id;
    std::string old_command_name;
    item->GetString(kExtension, &old_extension_id);
    item->GetString(kCommandName, &old_command_name);
    RemoveKeybindingPrefs(old_extension_id, old_command_name);
  }

  // If the command that is taking a new shortcut already has a shortcut, remove
  // it before assigning the new one.
  if (existing_command.accelerator().key_code() != ui::VKEY_UNKNOWN)
    RemoveKeybindingPrefs(extension_id, command_name);

  // Set the keybinding pref.
  auto keybinding = std::make_unique<base::DictionaryValue>();
  keybinding->SetString(kExtension, extension_id);
  keybinding->SetString(kCommandName, command_name);
  keybinding->SetBoolean(kGlobal, global);

  bindings->Set(key, std::move(keybinding));

  // Set the was_assigned pref for the suggested key.
  std::unique_ptr<base::DictionaryValue> command_keys(
      new base::DictionaryValue);
  command_keys->SetBoolean(kSuggestedKeyWasAssigned, true);
  std::unique_ptr<base::DictionaryValue> suggested_key_prefs(
      new base::DictionaryValue);
  suggested_key_prefs->Set(command_name, std::move(command_keys));
  MergeSuggestedKeyPrefs(extension_id, ExtensionPrefs::Get(profile_),
                         std::move(suggested_key_prefs));

  // Fetch the newly-updated command, and notify the observers.
  for (auto& observer : observers_) {
    observer.OnExtensionCommandAdded(
        extension_id, FindCommandByName(extension_id, command_name));
  }

  // TODO(devlin): Deprecate this notification in favor of the observers.
  std::pair<const std::string, const std::string> details =
      std::make_pair(extension_id, command_name);
  content::NotificationService::current()->Notify(
      extensions::NOTIFICATION_EXTENSION_COMMAND_ADDED,
      content::Source<Profile>(profile_),
      content::Details<std::pair<const std::string, const std::string> >(
          &details));

  return true;
}

void CommandService::OnExtensionWillBeInstalled(
    content::BrowserContext* browser_context,
    const Extension* extension,
    bool is_update,
    const std::string& old_name) {
  UpdateKeybindings(extension);
}

void CommandService::OnExtensionUninstalled(
    content::BrowserContext* browser_context,
    const Extension* extension,
    extensions::UninstallReason reason) {
}

void CommandService::UpdateKeybindingPrefs(const std::string& extension_id,
                                           const std::string& command_name,
                                           const std::string& keystroke) {
}

bool CommandService::SetScope(const std::string& extension_id,
                              const std::string& command_name,
                              bool global) {
  return true;
}

Command CommandService::FindCommandByName(const std::string& extension_id,
                                          const std::string& command) const {
  return Command();
}

bool CommandService::GetSuggestedExtensionCommand(
    const std::string& extension_id,
    const ui::Accelerator& accelerator,
    Command* command) const {
  return false;
}

bool CommandService::RequestsBookmarkShortcutOverride(
    const Extension* extension) const {
  return RemovesBookmarkShortcut(extension) &&
         GetSuggestedExtensionCommand(
             extension->id(),
             chrome::GetPrimaryChromeAcceleratorForBookmarkPage(), nullptr);
}

void CommandService::AddObserver(Observer* observer) {
  observers_.AddObserver(observer);
}

void CommandService::RemoveObserver(Observer* observer) {
  observers_.RemoveObserver(observer);
}

void CommandService::UpdateKeybindings(const Extension* extension) {
}

void CommandService::RemoveRelinquishedKeybindings(const Extension* extension) {
}

void CommandService::AssignKeybindings(const Extension* extension) {
}

bool CommandService::CanAutoAssign(const Command &command,
                                   const Extension* extension) {
  return false;
}

void CommandService::UpdateExtensionSuggestedCommandPrefs(
    const Extension* extension) {
}

void CommandService::RemoveDefunctExtensionSuggestedCommandPrefs(
    const Extension* extension) {
}

bool CommandService::IsCommandShortcutUserModified(
    const Extension* extension,
    const std::string& command_name) {
  return false;
}

bool CommandService::IsKeybindingChanging(const Extension* extension,
                                          const std::string& command_name) {
  return false;
}

std::string CommandService::GetSuggestedKeyPref(
    const Extension* extension,
    const std::string& command_name) {
  return std::string();
}

void CommandService::RemoveKeybindingPrefs(const std::string& extension_id,
                                           const std::string& command_name) {
}

bool CommandService::GetExtensionActionCommand(
    const std::string& extension_id,
    QueryType query_type,
    Command* command,
    bool* active,
    Command::Type action_type) const {
  return false;
}

template <>
void
BrowserContextKeyedAPIFactory<CommandService>::DeclareFactoryDependencies() {
  DependsOn(ExtensionCommandsGlobalRegistry::GetFactoryInstance());
}

}  // namespace extensions
