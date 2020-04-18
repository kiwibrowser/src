// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_KEYED_SERVICE_CORE_DEPENDENCY_MANAGER_H_
#define COMPONENTS_KEYED_SERVICE_CORE_DEPENDENCY_MANAGER_H_

#include <set>
#include <string>

#include "components/keyed_service/core/dependency_graph.h"
#include "components/keyed_service/core/keyed_service_export.h"

class KeyedServiceBaseFactory;

namespace base {
class FilePath;
class SupportsUserData;
}

namespace user_prefs {
class PrefRegistrySyncable;
}

// DependencyManager manages dependency between KeyedServiceBaseFactory
// broadcasting the context creation and destruction to each factory in
// a safe order based on the stated dependencies.
class KEYED_SERVICE_EXPORT DependencyManager {
 protected:
  DependencyManager();
  virtual ~DependencyManager();

  // Adds/Removes a component from our list of live components. Removing will
  // also remove live dependency links.
  void AddComponent(KeyedServiceBaseFactory* component);
  void RemoveComponent(KeyedServiceBaseFactory* component);

  // Adds a dependency between two factories.
  void AddEdge(KeyedServiceBaseFactory* depended,
               KeyedServiceBaseFactory* dependee);

  // Registers preferences for all services via |registry| associated with
  // |context| (the association is managed by the embedder). The |context|
  // is used as a key to prevent multiple registration during tests.
  void RegisterPrefsForServices(base::SupportsUserData* context,
                                user_prefs::PrefRegistrySyncable* registry);

  // Called upon creation of |context| to create services that want to be
  // started at the creation of a context and register service-related
  // preferences.
  //
  // To have a KeyedService started when a context is created the method
  // KeyedServiceBaseFactory::ServiceIsCreatedWithContext() must be overridden
  // to return true.
  //
  // If |is_testing_context| then the service will not be started unless the
  // method KeyedServiceBaseFactory::ServiceIsNULLWhileTesting() return false.
  void CreateContextServices(base::SupportsUserData* context,
                             bool is_testing_context);

  // Called upon destruction of |context| to destroy all services associated
  // with it.
  void DestroyContextServices(base::SupportsUserData* context);

  // Runtime assertion called as a part of GetServiceForContext() to check if
  // |context| is considered stale. This will NOTREACHED() or
  // base::debug::DumpWithoutCrashing() depending on the DCHECK_IS_ON() value.
  void AssertContextWasntDestroyed(base::SupportsUserData* context) const;

  // Marks |context| as live (i.e., not stale). This method can be called as a
  // safeguard against |AssertContextWasntDestroyed()| checks going off due to
  // |context| aliasing an instance from a prior construction (i.e., 0xWhatever
  // might be created, be destroyed, and then a new object might be created at
  // 0xWhatever).
  void MarkContextLive(base::SupportsUserData* context);

#ifndef NDEBUG
  // Dumps service dependency graph as a Graphviz dot file |dot_file| with a
  // title |top_level_name|. Helper for |DumpContextDependencies|.
  void DumpDependenciesAsGraphviz(const std::string& top_level_name,
                                  const base::FilePath& dot_file) const;
#endif  // NDEBUG

 private:
  friend class KeyedServiceBaseFactory;

#ifndef NDEBUG
  // Hook for subclass to dump the dependency graph of service for |context|.
  virtual void DumpContextDependencies(
      base::SupportsUserData* context) const = 0;
#endif  // NDEBUG

  DependencyGraph dependency_graph_;

  // A list of context objects that have gone through the Shutdown() phase.
  // These pointers are most likely invalid, but we keep track of their
  // locations in memory so we can nicely assert if we're asked to do anything
  // with them.
  std::set<base::SupportsUserData*> dead_context_pointers_;
};

#endif  // COMPONENTS_KEYED_SERVICE_CORE_DEPENDENCY_MANAGER_H_
