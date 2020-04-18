// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_KEYED_SERVICE_CORE_KEYED_SERVICE_BASE_FACTORY_H_
#define COMPONENTS_KEYED_SERVICE_CORE_KEYED_SERVICE_BASE_FACTORY_H_

#include <set>

#include "base/sequence_checker.h"
#include "components/keyed_service/core/dependency_node.h"
#include "components/keyed_service/core/keyed_service_export.h"

class DependencyManager;
class PrefService;

namespace base {
class SupportsUserData;
}

namespace user_prefs {
class PrefRegistrySyncable;
}

// Base class for factories that take a base::SupportsUserData and return some
// service. Not for direct usage, instead use descendent classes that deal with
// more specific context objects.
//
// This object describes general dependency management between factories while
// direct subclasses react to lifecycle events and implement memory management.
class KEYED_SERVICE_EXPORT KeyedServiceBaseFactory : public DependencyNode {
 public:
#ifndef NDEBUG
  // Returns our name. We don't keep track of this in release mode.
  const char* name() const { return service_name_; }
#endif

 protected:
  KeyedServiceBaseFactory(const char* service_name, DependencyManager* manager);
  virtual ~KeyedServiceBaseFactory();

  // Registers preferences used in this service on the pref service associated
  // with |context|. This is safe to be called multiple times because testing
  // code can have multiple services of the same type attached to a single
  // |context|. Only test code is allowed to call this method.
  //
  // TODO(gab): This method can be removed entirely when
  // PrefService::DeprecatedGetPrefRegistry() is phased out.
  void RegisterUserPrefsOnContextForTest(base::SupportsUserData* context);

  // The main public interface for declaring dependencies between services
  // created by factories.
  void DependsOn(KeyedServiceBaseFactory* rhs);

  // Runtime assertion to check if |context| is considered stale. Should be used
  // by subclasses when accessing |context|.
  void AssertContextWasntDestroyed(base::SupportsUserData* context) const;

  // Marks |context| as live (i.e., not stale). This method can be called as a
  // safeguard against |AssertContextWasntDestroyed()| checks going off due to
  // |context| aliasing an instance from a prior construction (i.e., 0xWhatever
  // might be created, be destroyed, and then a new object might be created at
  // 0xWhatever).
  void MarkContextLive(base::SupportsUserData* context);

  // Calls RegisterProfilePrefs() after doing house keeping required to work
  // alongside RegisterUserPrefsOnContextForTest().
  // TODO(gab): This method can be replaced by RegisterProfilePrefs() directly
  // once RegisterUserPrefsOnContextForTest() is phased out.
  void RegisterPrefsIfNecessaryForContext(
      base::SupportsUserData* context,
      user_prefs::PrefRegistrySyncable* registry);

  // Returns the |user_pref::PrefRegistrySyncable| associated with |context|.
  // The way they are associated is controlled by the embedder.
  user_prefs::PrefRegistrySyncable* GetAssociatedPrefRegistry(
      base::SupportsUserData* context) const;

  // Finds which context (if any) to use.
  virtual base::SupportsUserData* GetContextToUse(
      base::SupportsUserData* context) const = 0;

  // By default, instance of a service are created lazily when GetForContext()
  // is called by the subclass. Some services need to be created as soon as the
  // context is created and should override this method to return true.
  virtual bool ServiceIsCreatedWithContext() const;

  // By default, testing contexts will be treated like normal contexts. If this
  // method is overriden to return true, then the service associated with the
  // testing context will be null.
  virtual bool ServiceIsNULLWhileTesting() const;

  // The service build by the factories goes through a two phase shutdown.
  // It is up to the individual factory types to determine what this two pass
  // shutdown means. The general framework guarantees the following:
  //
  // - Each ContextShutdown() is called in dependency order (and you may
  //   reach out to other services during this phase).
  //
  // - Each ContextDestroyed() is called in dependency order. Accessing a
  //   service with GetForContext() will NOTREACHED() and code should delete/
  //   deref/do other final memory management during this phase. The base class
  //   method *must* be called as the last thing.
  virtual void ContextShutdown(base::SupportsUserData* context) = 0;
  virtual void ContextDestroyed(base::SupportsUserData* context);

  // Returns whether the preferences have been registered on this context.
  bool ArePreferencesSetOn(base::SupportsUserData* context) const;

  // Mark context has having preferences registered.
  void MarkPreferencesSetOn(base::SupportsUserData* context);

  SEQUENCE_CHECKER(sequence_checker_);

 private:
  friend class DependencyManager;

  // The DependencyManager used. In real code, this will be a singleton used
  // by all the factories of a given type. Unit tests will use their own copy.
  DependencyManager* dependency_manager_;

  // Registers any preferences used by this service.
  virtual void RegisterPrefs(user_prefs::PrefRegistrySyncable* registry) {}

  // Used by DependencyManager to disable creation of the service when the
  // method ServiceIsNULLWhileTesting() returns true.
  virtual void SetEmptyTestingFactory(base::SupportsUserData* context) = 0;

  // Returns true if a testing factory function has been set for |context|.
  virtual bool HasTestingFactory(base::SupportsUserData* context) = 0;

  // Create the service associated with |context|.
  virtual void CreateServiceNow(base::SupportsUserData* context) = 0;

  // Contexts that have this service's preferences registered on them.
  std::set<base::SupportsUserData*> registered_preferences_;

#if !defined(NDEBUG)
  // A static string passed in to the constructor. Should be unique across all
  // services. This is used only for debugging in debug mode. (Used to print
  // pretty graphs with GraphViz.)
  const char* service_name_;
#endif
};

#endif  // COMPONENTS_KEYED_SERVICE_CORE_KEYED_SERVICE_BASE_FACTORY_H_
