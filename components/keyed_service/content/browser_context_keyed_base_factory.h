// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_KEYED_SERVICE_CONTENT_BROWSER_CONTEXT_KEYED_BASE_FACTORY_H_
#define COMPONENTS_KEYED_SERVICE_CONTENT_BROWSER_CONTEXT_KEYED_BASE_FACTORY_H_

#include "components/keyed_service/core/keyed_service_base_factory.h"
#include "components/keyed_service/core/keyed_service_export.h"

class BrowserContextDependencyManager;
class PrefService;

namespace content {
class BrowserContext;
}

namespace user_prefs {
class PrefRegistrySyncable;
}

// Base class for Factories that take a BrowserContext object and return some
// service.
//
// Unless you're trying to make a new type of Factory, you probably don't want
// this class, but its subclasses: BrowserContextKeyedServiceFactory and
// RefcountedBrowserContextKeyedServiceFactory. This object describes general
// dependency management between Factories; subclasses react to lifecycle
// events and implement memory management.
//
// Note: this class is deprecated and should not be used and will be removed
// once http://crbug.com/131843 and http://crbug.com/131844 are closed. If you
// need to implement a new way to manage KeyedService lifetime, base your code
// on KeyedServiceBaseFactory instead.
class KEYED_SERVICE_EXPORT BrowserContextKeyedBaseFactory
    : public KeyedServiceBaseFactory {
 public:
  // Registers preferences used in this service on the pref service of
  // |context|. This is the public interface and is safe to be called multiple
  // times because testing code can have multiple services of the same type
  // attached to a single |context|. Only test code is allowed to call this
  // method.
  // TODO(gab): This method can be removed entirely when
  // PrefService::DeprecatedGetPrefRegistry() is phased out.
  void RegisterUserPrefsOnBrowserContextForTest(
      content::BrowserContext* context);

 protected:
  BrowserContextKeyedBaseFactory(const char* name,
                                 BrowserContextDependencyManager* manager);
  ~BrowserContextKeyedBaseFactory() override;

  // Interface for people building a concrete FooServiceFactory: --------------

  // Finds which browser context (if any) to use.
  virtual content::BrowserContext* GetBrowserContextToUse(
      content::BrowserContext* context) const;

  // By default, we create instances of a service lazily and wait until
  // GetForBrowserContext() is called on our subclass. Some services need to be
  // created as soon as the BrowserContext has been brought up.
  virtual bool ServiceIsCreatedWithBrowserContext() const;

  // By default, TestingBrowserContexts will be treated like normal contexts.
  // You can override this so that by default, the service associated with the
  // TestingBrowserContext is NULL. (This is just a shortcut around
  // SetTestingFactory() to make sure our contexts don't directly refer to the
  // services they use.)
  bool ServiceIsNULLWhileTesting() const override;

  // Interface for people building a type of BrowserContextKeyedFactory: -------

  // A helper object actually listens for notifications about BrowserContext
  // destruction, calculates the order in which things are destroyed and then
  // does a two pass shutdown.
  //
  // It is up to the individual factory types to determine what this two pass
  // shutdown means. The general framework guarantees the following:
  //
  // - Each BrowserContextShutdown() is called in dependency order (and you may
  //   reach out to other services during this phase).
  //
  // - Each BrowserContextDestroyed() is called in dependency order. We will
  //   NOTREACHED() if you attempt to GetForBrowserContext() any other service.
  //   You should delete/deref/do other final memory management things during
  //   this phase. You must also call the base class method as the last thing
  //   you do.
  virtual void BrowserContextShutdown(content::BrowserContext* context) = 0;
  virtual void BrowserContextDestroyed(content::BrowserContext* context);

 private:
  // Registers any user preferences on this service. This is called by
  // RegisterPrefsIfNecessaryForContext() and should be overriden by any service
  // that wants to register profile-specific preferences.
  virtual void RegisterProfilePrefs(
      user_prefs::PrefRegistrySyncable* registry) {}

  // Because of ServiceIsNULLWhileTesting(), we need a way to tell different
  // subclasses that they should disable testing.
  virtual void SetEmptyTestingFactory(content::BrowserContext* context) = 0;

  // Returns true if a testing factory function has been set for |context|.
  virtual bool HasTestingFactory(content::BrowserContext* context) = 0;

  // We also need a generalized, non-returning method that generates the object
  // now for when we're creating the context.
  virtual void CreateServiceNow(content::BrowserContext* context) = 0;

  // KeyedServiceBaseFactory:
  base::SupportsUserData* GetContextToUse(
      base::SupportsUserData* context) const final;
  bool ServiceIsCreatedWithContext() const final;
  void ContextShutdown(base::SupportsUserData* context) final;
  void ContextDestroyed(base::SupportsUserData* context) final;
  void RegisterPrefs(user_prefs::PrefRegistrySyncable* registry) final;
  void SetEmptyTestingFactory(base::SupportsUserData* context) final;
  bool HasTestingFactory(base::SupportsUserData* context) final;
  void CreateServiceNow(base::SupportsUserData* context) final;
};

#endif  // COMPONENTS_KEYED_SERVICE_CONTENT_BROWSER_CONTEXT_KEYED_BASE_FACTORY_H_
