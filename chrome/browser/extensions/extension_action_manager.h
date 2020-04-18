// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_EXTENSIONS_EXTENSION_ACTION_MANAGER_H_
#define CHROME_BROWSER_EXTENSIONS_EXTENSION_ACTION_MANAGER_H_

#include <map>
#include <memory>
#include <string>

#include "base/scoped_observer.h"
#include "chrome/common/extensions/api/extension_action/action_info.h"
#include "components/keyed_service/core/keyed_service.h"
#include "extensions/browser/extension_registry_observer.h"

class ExtensionAction;
class Profile;

namespace extensions {

class Extension;
class ExtensionRegistry;

// Owns the ExtensionActions associated with each extension.  These actions live
// while an extension is loaded and are destroyed on unload.
class ExtensionActionManager : public KeyedService,
                               public ExtensionRegistryObserver {
 public:
  explicit ExtensionActionManager(Profile* profile);
  ~ExtensionActionManager() override;

  // Returns this profile's ExtensionActionManager.  One instance is
  // shared between a profile and its incognito version.
  static ExtensionActionManager* Get(content::BrowserContext* browser_context);

  // Retrieves the page action, browser action, or system indicator for
  // |extension|.
  // If the result is not NULL, it remains valid until the extension is
  // unloaded.
  ExtensionAction* GetPageAction(const Extension& extension) const;
  ExtensionAction* GetBrowserAction(const Extension& extension) const;
  ExtensionAction* GetSystemIndicator(const Extension& extension) const;

  // Returns either the PageAction or BrowserAction for |extension|, or NULL if
  // none exists. Since an extension can only declare one of Browser|PageAction,
  // this is okay to use anywhere you need a generic "ExtensionAction".
  // Since SystemIndicators are used differently and don't follow this
  // rule of mutual exclusion, they are not checked or returned.
  ExtensionAction* GetExtensionAction(const Extension& extension) const;

  // Gets the best fit ExtensionAction for the given |extension|. This takes
  // into account |extension|'s browser or page actions, if any, along with its
  // name and any declared icons.
  std::unique_ptr<ExtensionAction> GetBestFitAction(
      const Extension& extension,
      ActionInfo::Type type) const;

 private:
  // Implement ExtensionRegistryObserver.
  void OnExtensionUnloaded(content::BrowserContext* browser_context,
                           const Extension* extension,
                           UnloadedExtensionReason reason) override;

  Profile* profile_;

  // Listen to extension unloaded notifications.
  ScopedObserver<ExtensionRegistry, ExtensionRegistryObserver>
      extension_registry_observer_;

  // Keyed by Extension ID.  These maps are populated lazily when their
  // ExtensionAction is first requested, and the entries are removed when the
  // extension is unloaded.  Not every extension has a page action or browser
  // action.
  using ExtIdToActionMap =
      std::map<std::string, std::unique_ptr<ExtensionAction>>;
  mutable ExtIdToActionMap page_actions_;
  mutable ExtIdToActionMap browser_actions_;
  mutable ExtIdToActionMap system_indicators_;
};

}  // namespace extensions

#endif  // CHROME_BROWSER_EXTENSIONS_EXTENSION_ACTION_MANAGER_H_
