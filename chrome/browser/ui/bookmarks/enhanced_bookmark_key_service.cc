// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/bookmarks/enhanced_bookmark_key_service.h"

#include <algorithm>

#include "base/strings/string_number_conversions.h"
#include "chrome/browser/extensions/api/commands/command_service.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/accelerator_utils.h"
#include "chrome/common/extensions/command.h"
#include "components/crx_file/id_util.h"
#include "extensions/browser/extension_registry.h"
#include "extensions/browser/notification_types.h"
#include "extensions/common/manifest_constants.h"

EnhancedBookmarkKeyService::EnhancedBookmarkKeyService(
    content::BrowserContext* context) : browser_context_(context) {
  Profile* profile = Profile::FromBrowserContext(browser_context_);
  registrar_.Add(this,
                 extensions::NOTIFICATION_EXTENSION_COMMAND_REMOVED,
                 content::Source<Profile>(profile->GetOriginalProfile()));
}

EnhancedBookmarkKeyService::~EnhancedBookmarkKeyService() {
}

void EnhancedBookmarkKeyService::Observe(
    int type,
    const content::NotificationSource& source,
    const content::NotificationDetails& details) {
  DCHECK_EQ(extensions::NOTIFICATION_EXTENSION_COMMAND_REMOVED, type);

  const extensions::Extension* enhanced_bookmark_extension =
      GetEnhancedBookmarkExtension();
  if (!enhanced_bookmark_extension)
    return;

  extensions::ExtensionCommandRemovedDetails* payload =
      content::Details<extensions::ExtensionCommandRemovedDetails>(details)
          .ptr();

  if (payload->extension_id == enhanced_bookmark_extension->id())
    return;

  ui::Accelerator key = extensions::Command::StringToAccelerator(
      payload->accelerator, payload->command_name);
  ui::Accelerator bookmark_accelerator =
      chrome::GetPrimaryChromeAcceleratorForBookmarkPage();
  if (key == bookmark_accelerator) {
    extensions::CommandService* command_service =
        extensions::CommandService::Get(browser_context_);
    extensions::Command existing_command;
    if (!command_service->GetPageActionCommand(
            enhanced_bookmark_extension->id(),
            extensions::CommandService::ACTIVE,
            &existing_command, nullptr)) {
      command_service->AddKeybindingPref(
          bookmark_accelerator, enhanced_bookmark_extension->id(),
          extensions::manifest_values::kPageActionCommandEvent, false,
          false);
    }
  }
}

const extensions::Extension*
EnhancedBookmarkKeyService::GetEnhancedBookmarkExtension() const {
  const extensions::ExtensionSet& extensions =
      extensions::ExtensionRegistry::Get(browser_context_)
      ->enabled_extensions();
  extensions::ExtensionSet::const_iterator loc =
      std::find_if(extensions.begin(), extensions.end(),
                   [](scoped_refptr<const extensions::Extension> extension) {
                     static const char enhanced_ext_hash[] =
                         // http://crbug.com/312900
                         "D5736E4B5CF695CB93A2FB57E4FDC6E5AFAB6FE2";
                     std::string hashed_id =
                         crx_file::id_util::HashedIdInHex(extension->id());
                     return hashed_id == enhanced_ext_hash;
                   });
  return loc != extensions.end() ? loc->get() : nullptr;
}
