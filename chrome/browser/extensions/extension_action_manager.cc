// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/extensions/extension_action_manager.h"

#include "chrome/browser/extensions/api/system_indicator/system_indicator_manager_factory.h"
#include "chrome/browser/extensions/extension_action.h"
#include "chrome/browser/profiles/profile.h"
#include "components/keyed_service/content/browser_context_dependency_manager.h"
#include "extensions/browser/extension_icon_image.h"
#include "extensions/browser/extension_registry.h"
#include "extensions/browser/extension_system.h"
#include "extensions/browser/extensions_browser_client.h"
#include "extensions/common/constants.h"
#include "ui/gfx/image/image_skia.h"

namespace extensions {

namespace {

// BrowserContextKeyedServiceFactory for ExtensionActionManager.
class ExtensionActionManagerFactory : public BrowserContextKeyedServiceFactory {
 public:
  // BrowserContextKeyedServiceFactory implementation:
  static ExtensionActionManager* GetForBrowserContext(
      content::BrowserContext* context) {
    return static_cast<ExtensionActionManager*>(
        GetInstance()->GetServiceForBrowserContext(context, true));
  }

  static ExtensionActionManagerFactory* GetInstance();

 private:
  friend struct base::DefaultSingletonTraits<ExtensionActionManagerFactory>;

  ExtensionActionManagerFactory()
      : BrowserContextKeyedServiceFactory(
          "ExtensionActionManager",
          BrowserContextDependencyManager::GetInstance()) {
  }

  KeyedService* BuildServiceInstanceFor(
      content::BrowserContext* profile) const override {
    return new ExtensionActionManager(static_cast<Profile*>(profile));
  }

  content::BrowserContext* GetBrowserContextToUse(
      content::BrowserContext* context) const override {
    return ExtensionsBrowserClient::Get()->GetOriginalContext(context);
  }
};

ExtensionActionManagerFactory*
ExtensionActionManagerFactory::GetInstance() {
  return base::Singleton<ExtensionActionManagerFactory>::get();
}

}  // namespace

ExtensionActionManager::ExtensionActionManager(Profile* profile)
    : profile_(profile), extension_registry_observer_(this) {
  CHECK_EQ(profile, profile->GetOriginalProfile())
      << "Don't instantiate this with an incognito profile.";
  extension_registry_observer_.Add(ExtensionRegistry::Get(profile_));
}

ExtensionActionManager::~ExtensionActionManager() {
  // Don't assert that the ExtensionAction maps are empty because Extensions are
  // sometimes (only in tests?) not unloaded before the Profile is destroyed.
}

ExtensionActionManager* ExtensionActionManager::Get(
    content::BrowserContext* context) {
  return ExtensionActionManagerFactory::GetForBrowserContext(context);
}

void ExtensionActionManager::OnExtensionUnloaded(
    content::BrowserContext* browser_context,
    const Extension* extension,
    UnloadedExtensionReason reason) {
  page_actions_.erase(extension->id());
  browser_actions_.erase(extension->id());
  system_indicators_.erase(extension->id());
}

namespace {

// Returns map[extension_id] if that entry exists. Otherwise, if
// action_info!=nullptr, creates an ExtensionAction from it, fills in the map,
// and returns that.  Otherwise (action_info==nullptr), returns nullptr.
ExtensionAction* GetOrCreateOrNull(
    std::map<std::string, std::unique_ptr<ExtensionAction>>* map,
    const Extension& extension,
    ActionInfo::Type action_type,
    const ActionInfo* action_info,
    Profile* profile) {
  auto it = map->find(extension.id());
  if (it != map->end())
    return it->second.get();
  if (!action_info)
    return nullptr;

  // Only create action info for enabled extensions.
  // This avoids bugs where actions are recreated just after being removed
  // in response to OnExtensionUnloaded().
  if (!ExtensionRegistry::Get(profile)
      ->enabled_extensions().Contains(extension.id())) {
    return nullptr;
  }

  auto action =
      std::make_unique<ExtensionAction>(extension, action_type, *action_info);

  if (action->default_icon()) {
    action->SetDefaultIconImage(std::make_unique<IconImage>(
        profile, &extension, *action->default_icon(),
        ExtensionAction::ActionIconSize(),
        ExtensionAction::FallbackIcon().AsImageSkia(), nullptr));
  }

  ExtensionAction* raw_action = action.get();
  (*map)[extension.id()] = std::move(action);
  return raw_action;
}

}  // namespace

ExtensionAction* ExtensionActionManager::GetPageAction(
    const Extension& extension) const {
  return GetOrCreateOrNull(&page_actions_, extension,
                           ActionInfo::TYPE_PAGE,
                           ActionInfo::GetPageActionInfo(&extension),
                           profile_);
}

ExtensionAction* ExtensionActionManager::GetBrowserAction(
    const Extension& extension) const {
  return GetOrCreateOrNull(&browser_actions_, extension,
                           ActionInfo::TYPE_BROWSER,
                           ActionInfo::GetBrowserActionInfo(&extension),
                           profile_);
}

std::unique_ptr<ExtensionAction> ExtensionActionManager::GetBestFitAction(
    const Extension& extension,
    ActionInfo::Type type) const {
  const ActionInfo* info = ActionInfo::GetBrowserActionInfo(&extension);
  if (!info)
    info = ActionInfo::GetPageActionInfo(&extension);

  // Create a new ExtensionAction of |type| with |extension|'s ActionInfo.
  // If no ActionInfo exists for |extension|, create and return a new action
  // with a blank ActionInfo.
  // Populate any missing values from |extension|'s manifest.
  return std::make_unique<ExtensionAction>(extension, type,
                                           info ? *info : ActionInfo());
}

ExtensionAction* ExtensionActionManager::GetSystemIndicator(
    const Extension& extension) const {
  // If it does not already exist, create the SystemIndicatorManager for the
  // given profile.  This could return NULL if the system indicator area is
  // unavailable on the current system.  If so, return NULL to signal that
  // the system indicator area is unusable.
  if (!SystemIndicatorManagerFactory::GetForProfile(profile_))
    return nullptr;

  return GetOrCreateOrNull(&system_indicators_, extension,
                           ActionInfo::TYPE_SYSTEM_INDICATOR,
                           ActionInfo::GetSystemIndicatorInfo(&extension),
                           profile_);
}

ExtensionAction* ExtensionActionManager::GetExtensionAction(
    const Extension& extension) const {
  ExtensionAction* action = GetBrowserAction(extension);
  return action ? action : GetPageAction(extension);
}

}  // namespace extensions
