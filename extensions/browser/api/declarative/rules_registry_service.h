// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef EXTENSIONS_BROWSER_API_DECLARATIVE_RULES_REGISTRY_SERVICE_H__
#define EXTENSIONS_BROWSER_API_DECLARATIVE_RULES_REGISTRY_SERVICE_H__

#include <map>
#include <string>
#include <tuple>
#include <vector>

#include "base/callback_forward.h"
#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "base/scoped_observer.h"
#include "extensions/browser/api/declarative/rules_registry.h"
#include "extensions/browser/browser_context_keyed_api_factory.h"
#include "extensions/browser/extension_registry_observer.h"

namespace content {
class BrowserContext;
}

namespace extensions {
class ContentRulesRegistry;
class ExtensionRegistry;
}

namespace extensions {

// This class owns all RulesRegistries implementations of an ExtensionService.
// This class lives on the UI thread.
class RulesRegistryService : public BrowserContextKeyedAPI,
                             public ExtensionRegistryObserver {
 public:
  static const int kDefaultRulesRegistryID;
  static const int kInvalidRulesRegistryID;

  struct RulesRegistryKey {
    std::string event_name;
    int rules_registry_id;
    RulesRegistryKey(const std::string& event_name, int rules_registry_id)
        : event_name(event_name), rules_registry_id(rules_registry_id) {}
    bool operator<(const RulesRegistryKey& other) const {
      return std::tie(event_name, rules_registry_id) <
             std::tie(other.event_name, other.rules_registry_id);
    }
  };

  explicit RulesRegistryService(content::BrowserContext* context);
  ~RulesRegistryService() override;

  // Unregisters refptrs to concrete RulesRegistries at other objects that were
  // created by us so that the RulesRegistries can be released.
  void Shutdown() override;

  // BrowserContextKeyedAPI implementation.
  static BrowserContextKeyedAPIFactory<RulesRegistryService>*
      GetFactoryInstance();

  // Convenience method to get the RulesRegistryService for a context. If a
  // RulesRegistryService does not already exist for |context|, one will be
  // created and returned.
  static RulesRegistryService* Get(content::BrowserContext* context);

  // The same as Get(), except that if a RulesRegistryService does not already
  // exist for |context|, nullptr is returned.
  static RulesRegistryService* GetIfExists(content::BrowserContext* context);

  int GetNextRulesRegistryID();

  // Registers the default RulesRegistries used in Chromium.
  void EnsureDefaultRulesRegistriesRegistered(int rules_registry_id);

  // Registers a RulesRegistry and wraps it in an InitializingRulesRegistry.
  void RegisterRulesRegistry(scoped_refptr<RulesRegistry> rule_registry);

  // Returns the RulesRegistry for |event_name| and |rules_registry_id| or
  // NULL if no such registry has been registered. Default rules registries
  // (such as the WebRequest rules registry) will be created on first access.
  scoped_refptr<RulesRegistry> GetRulesRegistry(int rules_registry_id,
                                                const std::string& event_name);

  // Remove all rules registries of the given rules_registry_id.
  void RemoveRulesRegistriesByID(int rules_registry_id);

  // Accessors for each type of rules registry.
  ContentRulesRegistry* content_rules_registry() const {
    CHECK(content_rules_registry_);
    return content_rules_registry_;
  }

  // Indicates whether any registry has rules registered.
  bool HasAnyRegisteredRules() const;

  // For testing.
  void SimulateExtensionUninstalled(const Extension* extension);

 private:
  friend class BrowserContextKeyedAPIFactory<RulesRegistryService>;

  // Maps <event name, rules registry ID> to RuleRegistries that handle these
  // events.
  typedef std::map<RulesRegistryKey, scoped_refptr<RulesRegistry> >
      RulesRegistryMap;

  // ExtensionRegistryObserver implementation.
  void OnExtensionLoaded(content::BrowserContext* browser_context,
                         const Extension* extension) override;
  void OnExtensionUnloaded(content::BrowserContext* browser_context,
                           const Extension* extension,
                           UnloadedExtensionReason reason) override;
  void OnExtensionUninstalled(content::BrowserContext* browser_context,
                              const Extension* extension,
                              extensions::UninstallReason reason) override;

  // Iterates over all registries, and calls |notification_callback| on them
  // with |extension| as the argument. If a registry lives on a different
  // thread, the call is posted to that thread, so no guarantee of synchronous
  // processing.
  void NotifyRegistriesHelper(
      void (RulesRegistry::*notification_callback)(const Extension*),
      const Extension* extension);

  // BrowserContextKeyedAPI implementation.
  static const char* service_name() {
    return "RulesRegistryService";
  }
  static const bool kServiceHasOwnInstanceInIncognito = true;
  static const bool kServiceIsNULLWhileTesting = true;

  int current_rules_registry_id_;

  RulesRegistryMap rule_registries_;

  // We own the parts of the registries which need to run on the UI thread.
  std::vector<std::unique_ptr<RulesCacheDelegate>> cache_delegates_;

  // Weak pointer into rule_registries_ to make it easier to handle content rule
  // conditions.
  ContentRulesRegistry* content_rules_registry_;

  // Listen to extension load, unloaded notification.
  ScopedObserver<ExtensionRegistry, ExtensionRegistryObserver>
      extension_registry_observer_;

  content::BrowserContext* browser_context_;

  DISALLOW_COPY_AND_ASSIGN(RulesRegistryService);
};

}  // namespace extensions

#endif  // EXTENSIONS_BROWSER_API_DECLARATIVE_RULES_REGISTRY_SERVICE_H__
