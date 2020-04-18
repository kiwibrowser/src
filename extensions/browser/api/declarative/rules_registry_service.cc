// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "extensions/browser/api/declarative/rules_registry_service.h"

#include <algorithm>
#include <memory>
#include <utility>

#include "base/bind.h"
#include "base/lazy_instance.h"
#include "base/logging.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/render_process_host.h"
#include "extensions/browser/api/declarative/rules_cache_delegate.h"
#include "extensions/browser/api/declarative_content/content_rules_registry.h"
#include "extensions/browser/api/declarative_webrequest/webrequest_constants.h"
#include "extensions/browser/api/declarative_webrequest/webrequest_rules_registry.h"
#include "extensions/browser/api/extensions_api_client.h"
#include "extensions/browser/api/web_request/web_request_api.h"
#include "extensions/browser/extension_registry.h"
#include "extensions/common/extension.h"

namespace extensions {

namespace {

// Registers |web_request_rules_registry| on the IO thread.
void RegisterToExtensionWebRequestEventRouterOnIO(
    content::BrowserContext* browser_context,
    int rules_registry_id,
    scoped_refptr<WebRequestRulesRegistry> web_request_rules_registry) {
  ExtensionWebRequestEventRouter::GetInstance()->RegisterRulesRegistry(
      browser_context, rules_registry_id, web_request_rules_registry);
}

void NotifyWithExtensionSafe(
    scoped_refptr<const Extension> extension,
    void (RulesRegistry::*notification_callback)(const Extension*),
    scoped_refptr<RulesRegistry> registry) {
  (registry.get()->*notification_callback)(extension.get());
}

}  // namespace

const int RulesRegistryService::kDefaultRulesRegistryID = 0;
const int RulesRegistryService::kInvalidRulesRegistryID = -1;

RulesRegistryService::RulesRegistryService(content::BrowserContext* context)
    : current_rules_registry_id_(kDefaultRulesRegistryID),
      content_rules_registry_(NULL),
      extension_registry_observer_(this),
      browser_context_(context) {
  if (browser_context_) {
    extension_registry_observer_.Add(ExtensionRegistry::Get(browser_context_));
    EnsureDefaultRulesRegistriesRegistered(kDefaultRulesRegistryID);
  }
}

RulesRegistryService::~RulesRegistryService() {}

int RulesRegistryService::GetNextRulesRegistryID() {
  return ++current_rules_registry_id_;
}

void RulesRegistryService::EnsureDefaultRulesRegistriesRegistered(
    int rules_registry_id) {
  if (!browser_context_)
    return;
  RulesRegistryKey key(declarative_webrequest_constants::kOnRequest,
                       rules_registry_id);

  // If we can find the key in the |rule_registries_| then we have already
  // installed the default registries.
  if (ContainsKey(rule_registries_, key))
    return;

  // We create at least an ephemeral WebRequest cache for all registries, but we
  // only persist the cache if it pertains to regular pages (i.e., not
  // webviews).
  RulesCacheDelegate::Type web_request_cache_delegate_type =
      rules_registry_id == kDefaultRulesRegistryID
          ? RulesCacheDelegate::Type::kPersistent
          : RulesCacheDelegate::Type::kEphemeral;

  auto web_request_cache_delegate = std::make_unique<RulesCacheDelegate>(
      web_request_cache_delegate_type, true /* log_storage_init_delay */);
  auto web_request_rules_registry =
      base::MakeRefCounted<WebRequestRulesRegistry>(
          browser_context_, web_request_cache_delegate.get(),
          rules_registry_id);
  cache_delegates_.push_back(std::move(web_request_cache_delegate));
  RegisterRulesRegistry(web_request_rules_registry);
  content::BrowserThread::PostTask(
      content::BrowserThread::IO, FROM_HERE,
      base::Bind(&RegisterToExtensionWebRequestEventRouterOnIO,
                 browser_context_, rules_registry_id,
                 web_request_rules_registry));

  // Only create a ContentRulesRegistry for regular pages.
  if (rules_registry_id == kDefaultRulesRegistryID) {
    auto content_rules_cache_delegate = std::make_unique<RulesCacheDelegate>(
        RulesCacheDelegate::Type::kPersistent,
        false /* log_storage_init_delay */);
    scoped_refptr<ContentRulesRegistry> content_rules_registry =
        ExtensionsAPIClient::Get()->CreateContentRulesRegistry(
            browser_context_, content_rules_cache_delegate.get());
    cache_delegates_.push_back(std::move(content_rules_cache_delegate));
    if (content_rules_registry) {
      RegisterRulesRegistry(content_rules_registry);
      content_rules_registry_ = content_rules_registry.get();
    }
  }
}

void RulesRegistryService::Shutdown() {
  // Release the references to all registries. This would happen soon during
  // destruction of |*this|, but we need the ExtensionWebRequestEventRouter to
  // be the last to reference the WebRequestRulesRegistry objects, so that
  // the posted task below causes their destruction on the IO thread, not on UI
  // where the destruction of |*this| takes place.
  // TODO(vabr): Remove once http://crbug.com/218451#c6 gets addressed.
  rule_registries_.clear();
  content::BrowserThread::PostTask(
      content::BrowserThread::IO, FROM_HERE,
      base::Bind(&RegisterToExtensionWebRequestEventRouterOnIO,
                 browser_context_,
                 RulesRegistryService::kDefaultRulesRegistryID,
                 scoped_refptr<WebRequestRulesRegistry>(NULL)));
}

static base::LazyInstance<BrowserContextKeyedAPIFactory<RulesRegistryService>>::
    DestructorAtExit g_factory = LAZY_INSTANCE_INITIALIZER;

// static
BrowserContextKeyedAPIFactory<RulesRegistryService>*
RulesRegistryService::GetFactoryInstance() {
  return g_factory.Pointer();
}

// static
RulesRegistryService* RulesRegistryService::Get(
    content::BrowserContext* context) {
  return BrowserContextKeyedAPIFactory<RulesRegistryService>::Get(context);
}

// static
RulesRegistryService* RulesRegistryService::GetIfExists(
    content::BrowserContext* context) {
  return BrowserContextKeyedAPIFactory<RulesRegistryService>::GetIfExists(
      context);
}

void RulesRegistryService::RegisterRulesRegistry(
    scoped_refptr<RulesRegistry> rule_registry) {
  const std::string event_name(rule_registry->event_name());
  RulesRegistryKey key(event_name, rule_registry->id());
  DCHECK(rule_registries_.find(key) == rule_registries_.end());
  rule_registries_[key] = rule_registry;
}

scoped_refptr<RulesRegistry> RulesRegistryService::GetRulesRegistry(
    int rules_registry_id,
    const std::string& event_name) {
  EnsureDefaultRulesRegistriesRegistered(rules_registry_id);

  RulesRegistryKey key(event_name, rules_registry_id);
  RulesRegistryMap::const_iterator i = rule_registries_.find(key);
  if (i == rule_registries_.end())
    return scoped_refptr<RulesRegistry>();
  return i->second;
}

void RulesRegistryService::RemoveRulesRegistriesByID(int rules_registry_id) {
  std::set<RulesRegistryKey> registries_to_delete;
  for (RulesRegistryMap::iterator it = rule_registries_.begin();
       it != rule_registries_.end(); ++it) {
    const RulesRegistryKey& key = it->first;
    if (key.rules_registry_id != rules_registry_id)
      continue;
    // Modifying a container while iterating over it can lead to badness. So we
    // save the keys in another container and delete them in another loop.
    registries_to_delete.insert(key);
  }

  for (std::set<RulesRegistryKey>::iterator it = registries_to_delete.begin();
       it != registries_to_delete.end(); ++it) {
    rule_registries_.erase(*it);
  }
}

bool RulesRegistryService::HasAnyRegisteredRules() const {
  return std::any_of(cache_delegates_.begin(), cache_delegates_.end(),
                     [](const std::unique_ptr<RulesCacheDelegate>& delegate) {
                       return delegate->HasRules();
                     });
}

void RulesRegistryService::SimulateExtensionUninstalled(
    const Extension* extension) {
  NotifyRegistriesHelper(&RulesRegistry::OnExtensionUninstalled, extension);
}

void RulesRegistryService::NotifyRegistriesHelper(
    void (RulesRegistry::*notification_callback)(const Extension*),
    const Extension* extension) {
  RulesRegistryMap::iterator i;
  for (i = rule_registries_.begin(); i != rule_registries_.end(); ++i) {
    scoped_refptr<RulesRegistry> registry = i->second;
    if (content::BrowserThread::CurrentlyOn(registry->owner_thread())) {
      (registry.get()->*notification_callback)(extension);
    } else {
      content::BrowserThread::PostTask(
          registry->owner_thread(), FROM_HERE,
          base::Bind(&NotifyWithExtensionSafe, base::WrapRefCounted(extension),
                     notification_callback, registry));
    }
  }
}

void RulesRegistryService::OnExtensionLoaded(
    content::BrowserContext* browser_context,
    const Extension* extension) {
  NotifyRegistriesHelper(&RulesRegistry::OnExtensionLoaded, extension);
}

void RulesRegistryService::OnExtensionUnloaded(
    content::BrowserContext* browser_context,
    const Extension* extension,
    UnloadedExtensionReason reason) {
  NotifyRegistriesHelper(&RulesRegistry::OnExtensionUnloaded, extension);
}

void RulesRegistryService::OnExtensionUninstalled(
    content::BrowserContext* browser_context,
    const Extension* extension,
    extensions::UninstallReason reason) {
  NotifyRegistriesHelper(&RulesRegistry::OnExtensionUninstalled, extension);
}

}  // namespace extensions
