// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/common/extensions/manifest_handlers/extension_action_handler.h"

#include <memory>

#include "base/strings/stringprintf.h"
#include "base/strings/utf_string_conversions.h"
#include "base/values.h"
#include "chrome/common/extensions/api/extension_action/action_info.h"
#include "chrome/common/extensions/extension_constants.h"
#include "chrome/grit/generated_resources.h"
#include "extensions/common/extension.h"
#include "extensions/common/file_util.h"
#include "extensions/common/manifest_constants.h"

namespace extensions {

ExtensionActionHandler::ExtensionActionHandler() {
}

ExtensionActionHandler::~ExtensionActionHandler() {
}

bool ExtensionActionHandler::Parse(Extension* extension,
                                   base::string16* error) {
  const char* key = nullptr;
  const char* error_key = nullptr;
  if (extension->manifest()->HasKey(manifest_keys::kAction)) {
    key = manifest_keys::kAction;
    error_key = manifest_errors::kInvalidAction;
  }

  if (extension->manifest()->HasKey(manifest_keys::kPageAction)) {
    if (key != nullptr) {
      // An extension can only have one action.
      *error = base::ASCIIToUTF16(manifest_errors::kOneUISurfaceOnly);
      return false;
    }
    key = manifest_keys::kPageAction;
    error_key = manifest_errors::kInvalidPageAction;
  }

  if (extension->manifest()->HasKey(manifest_keys::kBrowserAction)) {
    if (key != nullptr) {
      // An extension can only have one action.
      *error = base::ASCIIToUTF16(manifest_errors::kOneUISurfaceOnly);
      return false;
    }
    key = manifest_keys::kBrowserAction;
    error_key = manifest_errors::kInvalidBrowserAction;
  }

  if (key) {
    const base::DictionaryValue* dict = nullptr;
    if (!extension->manifest()->GetDictionary(key, &dict)) {
      *error = base::ASCIIToUTF16(error_key);
      return false;
    }

    std::unique_ptr<ActionInfo> action_info =
        ActionInfo::Load(extension, dict, error);
    if (!action_info)
      return false;  // Failed to parse extension action definition.

    if (key == manifest_keys::kAction) {
      ActionInfo::SetExtensionActionInfo(extension, action_info.release());
    } else {
      if (dict->HasKey(manifest_keys::kActionDefaultState)) {
        *error =
            base::ASCIIToUTF16(manifest_errors::kDefaultStateShouldNotBeSet);
        return false;
      }

      if (key == manifest_keys::kPageAction)
        ActionInfo::SetPageActionInfo(extension, action_info.release());
      else
        ActionInfo::SetBrowserActionInfo(extension, action_info.release());
    }
  } else {  // No key, used for synthesizing an action for extensions with none.
    if (Manifest::IsComponentLocation(extension->location()))
      return true;  // Don't synthesize actions for component extensions.
    if (extension->was_installed_by_default())
      return true;  // Don't synthesize actions for default extensions.
    if (extension->manifest()->HasKey(
            manifest_keys::kSynthesizeExtensionAction)) {
      *error = base::ASCIIToUTF16(base::StringPrintf(
          "Key %s is reserved.", manifest_keys::kSynthesizeExtensionAction));
      return false;  // No one should use this key.
    }

    // Set an empty page action. We use a page action (instead of a browser
    // action) because the action should not be seen as enabled on every page.
    std::unique_ptr<ActionInfo> action_info(new ActionInfo());
    action_info->synthesized = true;
    ActionInfo::SetPageActionInfo(extension, action_info.release());
  }

  return true;
}

bool ExtensionActionHandler::Validate(
    const Extension* extension,
    std::string* error,
    std::vector<InstallWarning>* warnings) const {
  int error_message = 0;
  const ActionInfo* action = ActionInfo::GetPageActionInfo(extension);
  if (action) {
    error_message = IDS_EXTENSION_LOAD_ICON_FOR_PAGE_ACTION_FAILED;
  } else {
    action = ActionInfo::GetBrowserActionInfo(extension);
    error_message = IDS_EXTENSION_LOAD_ICON_FOR_BROWSER_ACTION_FAILED;
  }

  if (action && !action->default_icon.empty() &&
      !file_util::ValidateExtensionIconSet(
          action->default_icon, extension, error_message, error)) {
    return false;
  }
  return true;
}

bool ExtensionActionHandler::AlwaysParseForType(Manifest::Type type) const {
  return type == Manifest::TYPE_EXTENSION || type == Manifest::TYPE_USER_SCRIPT;
}

base::span<const char* const> ExtensionActionHandler::Keys() const {
  static constexpr const char* kKeys[] = {
      manifest_keys::kPageAction, manifest_keys::kBrowserAction,
      manifest_keys::kSynthesizeExtensionAction};
  return kKeys;
}

}  // namespace extensions
