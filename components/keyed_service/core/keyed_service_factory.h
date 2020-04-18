// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_KEYED_SERVICE_CORE_KEYED_SERVICE_FACTORY_H_
#define COMPONENTS_KEYED_SERVICE_CORE_KEYED_SERVICE_FACTORY_H_

#include <map>
#include <memory>

#include "base/callback_forward.h"
#include "base/compiler_specific.h"
#include "base/macros.h"
#include "components/keyed_service/core/keyed_service_base_factory.h"
#include "components/keyed_service/core/keyed_service_export.h"

class DependencyManager;
class KeyedService;

// Base class for Factories that take a base::SupportsUserData object and return
// some service on a one-to-one mapping. Each concrete factory that derives from
// this class *must* be a Singleton (only unit tests don't do that).
//
// We do this because services depend on each other and we need to control
// shutdown/destruction order. In each derived classes' constructors, the
// implementors must explicitly state which services are depended on.
class KEYED_SERVICE_EXPORT KeyedServiceFactory
    : public KeyedServiceBaseFactory {
 protected:
  KeyedServiceFactory(const char* name, DependencyManager* manager);
  ~KeyedServiceFactory() override;

  // A function that supplies the instance of a KeyedService for a given
  // |context|. This is used primarily for testing, where we want to feed
  // a specific mock into the KeyedServiceFactory system.
  typedef std::function<std::unique_ptr<KeyedService>(
      base::SupportsUserData* context)>
      TestingFactoryFunction;

  // Associates |factory| with |context| so that |factory| is used to create
  // the KeyedService when requested.  |factory| can be NULL to signal that
  // KeyedService should be NULL. Multiple calls to SetTestingFactory() are
  // allowed; previous services will be shut down.
  void SetTestingFactory(base::SupportsUserData* context,
                         TestingFactoryFunction factory);

  // Associates |factory| with |context| and immediately returns the created
  // KeyedService. Since the factory will be used immediately, it may not be
  // NULL.
  KeyedService* SetTestingFactoryAndUse(base::SupportsUserData* context,
                                        TestingFactoryFunction factory);

  // Common implementation that maps |context| to some service object. Deals
  // with incognito contexts per subclass instructions with GetContextToUse()
  // method on the base.  If |create| is true, the service will be created
  // using BuildServiceInstanceFor() if it doesn't already exist.
  KeyedService* GetServiceForContext(base::SupportsUserData* context,
                                     bool create);

  // Maps |context| to |service| with debug checks to prevent duplication.
  void Associate(base::SupportsUserData* context,
                 std::unique_ptr<KeyedService> service);

  // Removes the mapping from |context| to a service.
  void Disassociate(base::SupportsUserData* context);

  // Returns a new KeyedService that will be associated with |context|.
  virtual std::unique_ptr<KeyedService> BuildServiceInstanceFor(
      base::SupportsUserData* context) const = 0;

  // Returns whether the |context| is off-the-record or not.
  virtual bool IsOffTheRecord(base::SupportsUserData* context) const = 0;

  // KeyedServiceBaseFactory:
  void ContextShutdown(base::SupportsUserData* context) override;
  void ContextDestroyed(base::SupportsUserData* context) override;

  void SetEmptyTestingFactory(base::SupportsUserData* context) override;
  bool HasTestingFactory(base::SupportsUserData* context) override;
  void CreateServiceNow(base::SupportsUserData* context) override;

 private:
  friend class DependencyManager;
  friend class DependencyManagerUnittests;

  typedef std::map<base::SupportsUserData*, KeyedService*> KeyedServices;
  typedef std::map<base::SupportsUserData*, TestingFactoryFunction>
      OverriddenTestingFunctions;

  // The mapping between a context and its service.
  KeyedServices mapping_;

  // The mapping between a context and its overridden
  // TestingFactoryFunction.
  OverriddenTestingFunctions testing_factories_;

  DISALLOW_COPY_AND_ASSIGN(KeyedServiceFactory);
};

#endif  // COMPONENTS_KEYED_SERVICE_CORE_KEYED_SERVICE_FACTORY_H_
