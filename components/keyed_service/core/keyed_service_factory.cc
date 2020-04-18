// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/keyed_service/core/keyed_service_factory.h"

#include <utility>

#include "base/logging.h"
#include "base/stl_util.h"
#include "base/trace_event/trace_event.h"
#include "components/keyed_service/core/dependency_manager.h"
#include "components/keyed_service/core/keyed_service.h"

KeyedServiceFactory::KeyedServiceFactory(const char* name,
                                         DependencyManager* manager)
    : KeyedServiceBaseFactory(name, manager) {
}

KeyedServiceFactory::~KeyedServiceFactory() {
  DCHECK(mapping_.empty());
}

void KeyedServiceFactory::SetTestingFactory(
    base::SupportsUserData* context,
    TestingFactoryFunction testing_factory) {
  // Destroying the context may cause us to lose data about whether |context|
  // has our preferences registered on it (since the context object itself
  // isn't dead). See if we need to readd it once we've gone through normal
  // destruction.
  bool add_context = ArePreferencesSetOn(context);

  // Ensure that |context| is not marked as stale (e.g., due to it aliasing an
  // instance that was destroyed in an earlier test) in order to avoid accesses
  // to |context| in |BrowserContextShutdown| from causing
  // |AssertBrowserContextWasntDestroyed| to raise an error.
  MarkContextLive(context);

  // We have to go through the shutdown and destroy mechanisms because there
  // are unit tests that create a service on a context and then change the
  // testing service mid-test.
  ContextShutdown(context);
  ContextDestroyed(context);

  if (add_context)
    MarkPreferencesSetOn(context);

  testing_factories_[context] = testing_factory;
}

KeyedService* KeyedServiceFactory::SetTestingFactoryAndUse(
    base::SupportsUserData* context,
    TestingFactoryFunction testing_factory) {
  DCHECK(testing_factory);
  SetTestingFactory(context, testing_factory);
  return GetServiceForContext(context, true);
}

KeyedService* KeyedServiceFactory::GetServiceForContext(
    base::SupportsUserData* context,
    bool create) {
  TRACE_EVENT0("browser,startup", "KeyedServiceFactory::GetServiceForContext");
  context = GetContextToUse(context);
  if (!context)
    return nullptr;

  // NOTE: If you modify any of the logic below, make sure to update the
  // refcounted version in refcounted_context_keyed_service_factory.cc!
  const auto& it = mapping_.find(context);
  if (it != mapping_.end())
    return it->second;

  // Object not found.
  if (!create)
    return nullptr;  // And we're forbidden from creating one.

  // Create new object.
  // Check to see if we have a per-context testing factory that we should use
  // instead of default behavior.
  std::unique_ptr<KeyedService> service;
  const auto& jt = testing_factories_.find(context);
  if (jt != testing_factories_.end()) {
    if (jt->second) {
      if (!IsOffTheRecord(context))
        RegisterUserPrefsOnContextForTest(context);
      service = jt->second(context);
    }
  } else {
    service = BuildServiceInstanceFor(context);
  }

  Associate(context, std::move(service));
  return mapping_[context];
}

void KeyedServiceFactory::Associate(base::SupportsUserData* context,
                                    std::unique_ptr<KeyedService> service) {
  DCHECK(!base::ContainsKey(mapping_, context));
  mapping_.insert(std::make_pair(context, service.release()));
}

void KeyedServiceFactory::Disassociate(base::SupportsUserData* context) {
  const auto& it = mapping_.find(context);
  if (it != mapping_.end()) {
    delete it->second;
    mapping_.erase(it);
  }
}

void KeyedServiceFactory::ContextShutdown(base::SupportsUserData* context) {
  const auto& it = mapping_.find(context);
  if (it != mapping_.end() && it->second)
    it->second->Shutdown();
}

void KeyedServiceFactory::ContextDestroyed(base::SupportsUserData* context) {
  Disassociate(context);

  // For unit tests, we also remove the factory function both so we don't
  // maintain a big map of dead pointers, but also since we may have a second
  // object that lives at the same address (see other comments about unit tests
  // in this file).
  testing_factories_.erase(context);

  KeyedServiceBaseFactory::ContextDestroyed(context);
}

void KeyedServiceFactory::SetEmptyTestingFactory(
    base::SupportsUserData* context) {
  SetTestingFactory(context, nullptr);
}

bool KeyedServiceFactory::HasTestingFactory(base::SupportsUserData* context) {
  return testing_factories_.find(context) != testing_factories_.end();
}

void KeyedServiceFactory::CreateServiceNow(base::SupportsUserData* context) {
  GetServiceForContext(context, true);
}
