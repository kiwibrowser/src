// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "extensions/common/manifest_handler.h"

#include <stddef.h>

#include <map>

#include "base/logging.h"
#include "base/stl_util.h"
#include "base/threading/thread_restrictions.h"
#include "extensions/common/extension.h"
#include "extensions/common/permissions/manifest_permission.h"
#include "extensions/common/permissions/manifest_permission_set.h"

namespace extensions {

namespace {

static base::LazyInstance<ManifestHandlerRegistry>::DestructorAtExit
    g_registry = LAZY_INSTANCE_INITIALIZER;
static ManifestHandlerRegistry* g_registry_override = NULL;

ManifestHandlerRegistry* GetRegistry() {
  if (!g_registry_override)
    return g_registry.Pointer();
  return g_registry_override;
}

}  // namespace

ManifestHandler::ManifestHandler() {
}

ManifestHandler::~ManifestHandler() {
}

bool ManifestHandler::Validate(const Extension* extension,
                               std::string* error,
                               std::vector<InstallWarning>* warnings) const {
  return true;
}

bool ManifestHandler::AlwaysParseForType(Manifest::Type type) const {
  return false;
}

bool ManifestHandler::AlwaysValidateForType(Manifest::Type type) const {
  return false;
}

const std::vector<std::string> ManifestHandler::PrerequisiteKeys() const {
  return std::vector<std::string>();
}

void ManifestHandler::Register() {
  linked_ptr<ManifestHandler> this_linked(this);

  ManifestHandlerRegistry* registry = GetRegistry();
  for (const char* key : Keys())
    registry->RegisterManifestHandler(key, this_linked);
}

ManifestPermission* ManifestHandler::CreatePermission() {
  return NULL;
}

ManifestPermission* ManifestHandler::CreateInitialRequiredPermission(
    const Extension* extension) {
  return NULL;
}

// static
void ManifestHandler::FinalizeRegistration() {
  GetRegistry()->Finalize();
}

// static
bool ManifestHandler::IsRegistrationFinalized() {
  return GetRegistry()->is_finalized_;
}

// static
bool ManifestHandler::ParseExtension(Extension* extension,
                                     base::string16* error) {
  return GetRegistry()->ParseExtension(extension, error);
}

// static
bool ManifestHandler::ValidateExtension(const Extension* extension,
                                        std::string* error,
                                        std::vector<InstallWarning>* warnings) {
  base::AssertBlockingAllowed();
  return GetRegistry()->ValidateExtension(extension, error, warnings);
}

// static
ManifestPermission* ManifestHandler::CreatePermission(const std::string& name) {
  return GetRegistry()->CreatePermission(name);
}

// static
void ManifestHandler::AddExtensionInitialRequiredPermissions(
    const Extension* extension, ManifestPermissionSet* permission_set) {
  return GetRegistry()->AddExtensionInitialRequiredPermissions(extension,
                                                               permission_set);
}

// static
const std::vector<std::string> ManifestHandler::SingleKey(
    const std::string& key) {
  return std::vector<std::string>(1, key);
}

ManifestHandlerRegistry::ManifestHandlerRegistry() : is_finalized_(false) {
}

ManifestHandlerRegistry::~ManifestHandlerRegistry() {
}

void ManifestHandlerRegistry::Finalize() {
  CHECK(!is_finalized_);
  SortManifestHandlers();
  is_finalized_ = true;
}

void ManifestHandlerRegistry::RegisterManifestHandler(
    const char* key,
    linked_ptr<ManifestHandler> handler) {
  CHECK(!is_finalized_);
  handlers_[key] = handler;
}

bool ManifestHandlerRegistry::ParseExtension(Extension* extension,
                                             base::string16* error) {
  std::map<int, ManifestHandler*> handlers_by_priority;
  for (ManifestHandlerMap::iterator iter = handlers_.begin();
       iter != handlers_.end(); ++iter) {
    ManifestHandler* handler = iter->second.get();
    if (extension->manifest()->HasPath(iter->first) ||
        handler->AlwaysParseForType(extension->GetType())) {
      handlers_by_priority[priority_map_[handler]] = handler;
    }
  }
  for (std::map<int, ManifestHandler*>::iterator iter =
           handlers_by_priority.begin();
       iter != handlers_by_priority.end(); ++iter) {
    if (!(iter->second)->Parse(extension, error))
      return false;
  }
  return true;
}

bool ManifestHandlerRegistry::ValidateExtension(
    const Extension* extension,
    std::string* error,
    std::vector<InstallWarning>* warnings) {
  std::set<ManifestHandler*> handlers;
  for (ManifestHandlerMap::iterator iter = handlers_.begin();
       iter != handlers_.end(); ++iter) {
    ManifestHandler* handler = iter->second.get();
    if (extension->manifest()->HasPath(iter->first) ||
        handler->AlwaysValidateForType(extension->GetType())) {
      handlers.insert(handler);
    }
  }
  for (std::set<ManifestHandler*>::iterator iter = handlers.begin();
       iter != handlers.end(); ++iter) {
    if (!(*iter)->Validate(extension, error, warnings))
      return false;
  }
  return true;
}

ManifestPermission* ManifestHandlerRegistry::CreatePermission(
    const std::string& name) {
  ManifestHandlerMap::const_iterator it = handlers_.find(name);
  if (it == handlers_.end())
    return NULL;

  return it->second->CreatePermission();
}

void ManifestHandlerRegistry::AddExtensionInitialRequiredPermissions(
    const Extension* extension, ManifestPermissionSet* permission_set) {
  for (ManifestHandlerMap::const_iterator it = handlers_.begin();
      it != handlers_.end(); ++it) {
    ManifestPermission* permission =
        it->second->CreateInitialRequiredPermission(extension);
    if (permission) {
      permission_set->insert(permission);
    }
  }
}

// static
ManifestHandlerRegistry* ManifestHandlerRegistry::SetForTesting(
    ManifestHandlerRegistry* new_registry) {
  ManifestHandlerRegistry* old_registry = GetRegistry();
  if (new_registry != g_registry.Pointer())
    g_registry_override = new_registry;
  else
    g_registry_override = NULL;
  return old_registry;
}

void ManifestHandlerRegistry::SortManifestHandlers() {
  std::set<ManifestHandler*> unsorted_handlers;
  for (ManifestHandlerMap::const_iterator iter = handlers_.begin();
       iter != handlers_.end(); ++iter) {
    unsorted_handlers.insert(iter->second.get());
  }

  int priority = 0;
  while (true) {
    std::set<ManifestHandler*> next_unsorted_handlers;
    for (std::set<ManifestHandler*>::const_iterator iter =
             unsorted_handlers.begin();
         iter != unsorted_handlers.end(); ++iter) {
      ManifestHandler* handler = *iter;
      const std::vector<std::string>& prerequisites =
          handler->PrerequisiteKeys();
      int unsatisfied = prerequisites.size();
      for (size_t i = 0; i < prerequisites.size(); ++i) {
        ManifestHandlerMap::const_iterator prereq_iter =
            handlers_.find(prerequisites[i]);
        // If the prerequisite does not exist, crash.
        CHECK(prereq_iter != handlers_.end())
            << "Extension manifest handler depends on unrecognized key "
            << prerequisites[i];
        // Prerequisite is in our map.
        if (base::ContainsKey(priority_map_, prereq_iter->second.get()))
          unsatisfied--;
      }
      if (unsatisfied == 0) {
        priority_map_[handler] = priority;
        priority++;
      } else {
        // Put in the list for next time.
        next_unsorted_handlers.insert(handler);
      }
    }
    if (next_unsorted_handlers.size() == unsorted_handlers.size())
      break;
    unsorted_handlers.swap(next_unsorted_handlers);
  }

  // If there are any leftover unsorted handlers, they must have had
  // circular dependencies.
  CHECK_EQ(unsorted_handlers.size(), std::set<ManifestHandler*>::size_type(0))
      << "Extension manifest handlers have circular dependencies!";
}

}  // namespace extensions
