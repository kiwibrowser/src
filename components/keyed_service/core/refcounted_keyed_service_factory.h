// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_KEYED_SERVICE_CORE_REFCOUNTED_KEYED_SERVICE_FACTORY_H_
#define COMPONENTS_KEYED_SERVICE_CORE_REFCOUNTED_KEYED_SERVICE_FACTORY_H_

#include <map>

#include "base/compiler_specific.h"
#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "components/keyed_service/core/keyed_service_base_factory.h"
#include "components/keyed_service/core/keyed_service_export.h"

class RefcountedKeyedService;

// A specialized KeyedServiceBaseFactory that manages a RefcountedThreadSafe<>.
//
// While the factory returns RefcountedThreadSafe<>s, the factory itself is a
// base::NotThreadSafe. Only call methods on this object on the UI thread.
//
// Implementers of RefcountedKeyedService should note that we guarantee that
// ShutdownOnUIThread() is called on the UI thread, but actual object
// destruction can happen anywhere.
class KEYED_SERVICE_EXPORT RefcountedKeyedServiceFactory
    : public KeyedServiceBaseFactory {
 protected:
  RefcountedKeyedServiceFactory(const char* name, DependencyManager* manager);
  ~RefcountedKeyedServiceFactory() override;

  // A function that supplies the instance of a KeyedService for a given
  // |context|. This is used primarily for testing, where we want to feed
  // a specific mock into the KeyedServiceFactory system.
  typedef std::function<scoped_refptr<RefcountedKeyedService>(
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
  scoped_refptr<RefcountedKeyedService> SetTestingFactoryAndUse(
      base::SupportsUserData* context,
      TestingFactoryFunction factory);

  // Common implementation that maps |context| to some service object. Deals
  // with incognito contexts per subclass instructions with GetContextToUse()
  // method on the base.  If |create| is true, the service will be created
  // using BuildServiceInstanceFor() if it doesn't already exist.
  scoped_refptr<RefcountedKeyedService> GetServiceForContext(
      base::SupportsUserData* context,
      bool create);

  // Maps |context| to |service| with debug checks to prevent duplication.
  void Associate(base::SupportsUserData* context,
                 const scoped_refptr<RefcountedKeyedService>& service);

  // Returns a new RefcountedKeyedService that will be associated with
  // |context|.
  virtual scoped_refptr<RefcountedKeyedService> BuildServiceInstanceFor(
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
  typedef std::map<base::SupportsUserData*,
                   scoped_refptr<RefcountedKeyedService>> KeyedServices;
  typedef std::map<base::SupportsUserData*, TestingFactoryFunction>
      OverriddenTestingFunctions;

  // The mapping between a context and its refcounted service.
  KeyedServices mapping_;

  // The mapping between a context and its overridden
  // TestingFactoryFunction.
  OverriddenTestingFunctions testing_factories_;

  DISALLOW_COPY_AND_ASSIGN(RefcountedKeyedServiceFactory);
};

#endif  // COMPONENTS_KEYED_SERVICE_CORE_REFCOUNTED_KEYED_SERVICE_FACTORY_H_
