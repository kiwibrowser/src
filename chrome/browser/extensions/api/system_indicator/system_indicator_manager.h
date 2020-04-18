// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_EXTENSIONS_API_SYSTEM_INDICATOR_SYSTEM_INDICATOR_MANAGER_H_
#define CHROME_BROWSER_EXTENSIONS_API_SYSTEM_INDICATOR_SYSTEM_INDICATOR_MANAGER_H_

#include <map>
#include <string>

#include "base/gtest_prod_util.h"
#include "base/macros.h"
#include "base/scoped_observer.h"
#include "base/threading/thread_checker.h"
#include "chrome/browser/extensions/api/extension_action/extension_action_api.h"
#include "chrome/browser/extensions/extension_action_icon_factory.h"
#include "components/keyed_service/core/keyed_service.h"
#include "extensions/browser/extension_registry_observer.h"

class ExtensionAction;
class Profile;
class StatusTray;

namespace extensions {
FORWARD_DECLARE_TEST(SystemIndicatorApiTest, SystemIndicator);

class ExtensionIndicatorIcon;
class ExtensionRegistry;

// Keeps track of all the systemIndicator icons created for a given Profile
// that are currently visible in the UI.  Use SystemIndicatorManagerFactory to
// create a SystemIndicatorManager object.
class SystemIndicatorManager : public ExtensionRegistryObserver,
                               public ExtensionActionAPI::Observer,
                               public KeyedService {
 public:
  SystemIndicatorManager(Profile* profile, StatusTray* status_tray);
  ~SystemIndicatorManager() override;

  // KeyedService implementation.
  void Shutdown() override;

 private:
  FRIEND_TEST_ALL_PREFIXES(SystemIndicatorApiTest, SystemIndicator);

  // ExtensionRegistryObserver implementation.
  void OnExtensionUnloaded(content::BrowserContext* browser_context,
                           const Extension* extension,
                           UnloadedExtensionReason reason) override;

  // ExtensionActionAPI::Observer implementation.
  void OnExtensionActionUpdated(
      ExtensionAction* extension_action,
      content::WebContents* web_contents,
      content::BrowserContext* browser_context) override;

  // Causes a call to OnStatusIconClicked for the specified extension_id.
  // Returns false if no ExtensionIndicatorIcon is found for the extension.
  bool SendClickEventToExtensionForTest(const std::string& extension_id);

  // Causes an indicator to be shown for the given extension_action.  Creates
  // the indicator if necessary.
  void CreateOrUpdateIndicator(
      const Extension* extension,
      ExtensionAction* extension_action);

  // Causes the indicator for the given extension to be hidden.
  void RemoveIndicator(const std::string& extension_id);

  using SystemIndicatorMap =
      std::map<const std::string, std::unique_ptr<ExtensionIndicatorIcon>>;

  Profile* profile_;
  StatusTray* status_tray_;
  SystemIndicatorMap system_indicators_;
  base::ThreadChecker thread_checker_;

  ScopedObserver<ExtensionActionAPI, ExtensionActionAPI::Observer>
      extension_action_observer_;

  // Listen to extension unloaded notifications.
  ScopedObserver<ExtensionRegistry, ExtensionRegistryObserver>
      extension_registry_observer_;

  DISALLOW_COPY_AND_ASSIGN(SystemIndicatorManager);
};

}  // namespace extensions

#endif  // CHROME_BROWSER_EXTENSIONS_API_SYSTEM_INDICATOR_SYSTEM_INDICATOR_MANAGER_H_
