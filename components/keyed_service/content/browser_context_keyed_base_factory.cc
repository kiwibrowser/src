// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/keyed_service/content/browser_context_keyed_base_factory.h"

#include "components/keyed_service/content/browser_context_dependency_manager.h"
#include "content/public/browser/browser_context.h"

BrowserContextKeyedBaseFactory::BrowserContextKeyedBaseFactory(
    const char* name,
    BrowserContextDependencyManager* manager)
    : KeyedServiceBaseFactory(name, manager) {
}

BrowserContextKeyedBaseFactory::~BrowserContextKeyedBaseFactory() {
}

content::BrowserContext* BrowserContextKeyedBaseFactory::GetBrowserContextToUse(
    content::BrowserContext* context) const {
  // TODO(crbug.com/701326): This DCHECK should be moved to GetContextToUse().
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  // Safe default for the Incognito mode: no service.
  if (context->IsOffTheRecord())
    return nullptr;

  return context;
}

void BrowserContextKeyedBaseFactory::RegisterUserPrefsOnBrowserContextForTest(
    content::BrowserContext* context) {
  KeyedServiceBaseFactory::RegisterUserPrefsOnContextForTest(context);
}

bool BrowserContextKeyedBaseFactory::ServiceIsCreatedWithBrowserContext()
    const {
  return KeyedServiceBaseFactory::ServiceIsCreatedWithContext();
}

bool BrowserContextKeyedBaseFactory::ServiceIsNULLWhileTesting() const {
  return KeyedServiceBaseFactory::ServiceIsNULLWhileTesting();
}

void BrowserContextKeyedBaseFactory::BrowserContextDestroyed(
    content::BrowserContext* context) {
  KeyedServiceBaseFactory::ContextDestroyed(context);
}

base::SupportsUserData* BrowserContextKeyedBaseFactory::GetContextToUse(
    base::SupportsUserData* context) const {
  AssertContextWasntDestroyed(context);
  return GetBrowserContextToUse(static_cast<content::BrowserContext*>(context));
}

bool BrowserContextKeyedBaseFactory::ServiceIsCreatedWithContext() const {
  return ServiceIsCreatedWithBrowserContext();
}

void BrowserContextKeyedBaseFactory::ContextShutdown(
    base::SupportsUserData* context) {
  BrowserContextShutdown(static_cast<content::BrowserContext*>(context));
}

void BrowserContextKeyedBaseFactory::ContextDestroyed(
    base::SupportsUserData* context) {
  BrowserContextDestroyed(static_cast<content::BrowserContext*>(context));
}

void BrowserContextKeyedBaseFactory::RegisterPrefs(
    user_prefs::PrefRegistrySyncable* registry) {
  RegisterProfilePrefs(registry);
}

void BrowserContextKeyedBaseFactory::SetEmptyTestingFactory(
    base::SupportsUserData* context) {
  SetEmptyTestingFactory(static_cast<content::BrowserContext*>(context));
}

bool BrowserContextKeyedBaseFactory::HasTestingFactory(
    base::SupportsUserData* context) {
  return HasTestingFactory(static_cast<content::BrowserContext*>(context));
}

void BrowserContextKeyedBaseFactory::CreateServiceNow(
    base::SupportsUserData* context) {
  CreateServiceNow(static_cast<content::BrowserContext*>(context));
}
