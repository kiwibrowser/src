// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/keyed_service/content/refcounted_browser_context_keyed_service_factory.h"

#include "base/logging.h"
#include "components/keyed_service/content/browser_context_dependency_manager.h"
#include "components/keyed_service/core/refcounted_keyed_service.h"
#include "content/public/browser/browser_context.h"

void RefcountedBrowserContextKeyedServiceFactory::SetTestingFactory(
    content::BrowserContext* context,
    TestingFactoryFunction testing_factory) {
  RefcountedKeyedServiceFactory::TestingFactoryFunction func;
  if (testing_factory) {
    func = [=](base::SupportsUserData* context) {
      return testing_factory(static_cast<content::BrowserContext*>(context));
    };
  }
  RefcountedKeyedServiceFactory::SetTestingFactory(context, func);
}

scoped_refptr<RefcountedKeyedService>
RefcountedBrowserContextKeyedServiceFactory::SetTestingFactoryAndUse(
    content::BrowserContext* context,
    TestingFactoryFunction testing_factory) {
  RefcountedKeyedServiceFactory::TestingFactoryFunction func;
  if (testing_factory) {
    func = [=](base::SupportsUserData* context) {
      return testing_factory(static_cast<content::BrowserContext*>(context));
    };
  }
  return RefcountedKeyedServiceFactory::SetTestingFactoryAndUse(context, func);
}

RefcountedBrowserContextKeyedServiceFactory::
    RefcountedBrowserContextKeyedServiceFactory(
        const char* name,
        BrowserContextDependencyManager* manager)
    : RefcountedKeyedServiceFactory(name, manager) {
}

RefcountedBrowserContextKeyedServiceFactory::
    ~RefcountedBrowserContextKeyedServiceFactory() {
}

scoped_refptr<RefcountedKeyedService>
RefcountedBrowserContextKeyedServiceFactory::GetServiceForBrowserContext(
    content::BrowserContext* context,
    bool create) {
  return RefcountedKeyedServiceFactory::GetServiceForContext(context, create);
}

content::BrowserContext*
RefcountedBrowserContextKeyedServiceFactory::GetBrowserContextToUse(
    content::BrowserContext* context) const {
  // TODO(crbug.com/701326): This DCHECK should be moved to GetContextToUse().
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  // Safe default for Incognito mode: no service.
  if (context->IsOffTheRecord())
    return nullptr;

  return context;
}

bool RefcountedBrowserContextKeyedServiceFactory::
    ServiceIsCreatedWithBrowserContext() const {
  return KeyedServiceBaseFactory::ServiceIsCreatedWithContext();
}

bool RefcountedBrowserContextKeyedServiceFactory::ServiceIsNULLWhileTesting()
    const {
  return KeyedServiceBaseFactory::ServiceIsNULLWhileTesting();
}

void RefcountedBrowserContextKeyedServiceFactory::BrowserContextShutdown(
    content::BrowserContext* context) {
  RefcountedKeyedServiceFactory::ContextShutdown(context);
}

void RefcountedBrowserContextKeyedServiceFactory::BrowserContextDestroyed(
    content::BrowserContext* context) {
  RefcountedKeyedServiceFactory::ContextDestroyed(context);
}

scoped_refptr<RefcountedKeyedService>
RefcountedBrowserContextKeyedServiceFactory::BuildServiceInstanceFor(
    base::SupportsUserData* context) const {
  return BuildServiceInstanceFor(
      static_cast<content::BrowserContext*>(context));
}

bool RefcountedBrowserContextKeyedServiceFactory::IsOffTheRecord(
    base::SupportsUserData* context) const {
  return static_cast<content::BrowserContext*>(context)->IsOffTheRecord();
}

base::SupportsUserData*
RefcountedBrowserContextKeyedServiceFactory::GetContextToUse(
    base::SupportsUserData* context) const {
  AssertContextWasntDestroyed(context);
  return GetBrowserContextToUse(static_cast<content::BrowserContext*>(context));
}

bool RefcountedBrowserContextKeyedServiceFactory::ServiceIsCreatedWithContext()
    const {
  return ServiceIsCreatedWithBrowserContext();
}

void RefcountedBrowserContextKeyedServiceFactory::ContextShutdown(
    base::SupportsUserData* context) {
  BrowserContextShutdown(static_cast<content::BrowserContext*>(context));
}

void RefcountedBrowserContextKeyedServiceFactory::ContextDestroyed(
    base::SupportsUserData* context) {
  BrowserContextDestroyed(static_cast<content::BrowserContext*>(context));
}

void RefcountedBrowserContextKeyedServiceFactory::RegisterPrefs(
    user_prefs::PrefRegistrySyncable* registry) {
  RegisterProfilePrefs(registry);
}
