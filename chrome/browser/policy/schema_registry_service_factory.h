// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_POLICY_SCHEMA_REGISTRY_SERVICE_FACTORY_H_
#define CHROME_BROWSER_POLICY_SCHEMA_REGISTRY_SERVICE_FACTORY_H_

#include <map>
#include <memory>

#include "base/compiler_specific.h"
#include "base/macros.h"
#include "base/memory/singleton.h"
#include "components/keyed_service/content/browser_context_keyed_base_factory.h"

namespace content {
class BrowserContext;
}

namespace policy {

class CombinedSchemaRegistry;
class Schema;
class SchemaRegistryService;

// Creates SchemaRegistryServices for BrowserContexts.
// TODO(joaodasilva): Convert this class to a proper BCKS once the PrefService
// becomes a BCKS too. For now the PrefService depends on the
// UserCloudPolicyManager, which depends on this.
class SchemaRegistryServiceFactory : public BrowserContextKeyedBaseFactory {
 public:
  // Returns the SchemaRegistryServiceFactory singleton.
  static SchemaRegistryServiceFactory* GetInstance();

  // Returns the SchemaRegistryService associated with |context|. This is only
  // valid before |context| is shut down.
  static SchemaRegistryService* GetForContext(content::BrowserContext* context);

  // Creates a new SchemaRegistryService for |context|, which must be managed
  // by the caller. Subsequent calls to GetForContext() will return the instance
  // created, as long as it lives.
  static std::unique_ptr<SchemaRegistryService> CreateForContext(
      content::BrowserContext* context,
      const Schema& chrome_schema,
      CombinedSchemaRegistry* global_registry);

 private:
  friend struct base::DefaultSingletonTraits<SchemaRegistryServiceFactory>;

  SchemaRegistryServiceFactory();
  ~SchemaRegistryServiceFactory() override;

  SchemaRegistryService* GetForContextInternal(
      content::BrowserContext* context);

  std::unique_ptr<SchemaRegistryService> CreateForContextInternal(
      content::BrowserContext* context,
      const Schema& chrome_schema,
      CombinedSchemaRegistry* global_registry);

  // BrowserContextKeyedBaseFactory:
  void BrowserContextShutdown(content::BrowserContext* context) override;
  void BrowserContextDestroyed(content::BrowserContext* context) override;
  void SetEmptyTestingFactory(content::BrowserContext* context) override;
  bool HasTestingFactory(content::BrowserContext* context) override;
  void CreateServiceNow(content::BrowserContext* context) override;

  typedef std::map<content::BrowserContext*, SchemaRegistryService*>
      RegistryMap;
  RegistryMap registries_;

  DISALLOW_COPY_AND_ASSIGN(SchemaRegistryServiceFactory);
};

}  // namespace policy

#endif  // CHROME_BROWSER_POLICY_SCHEMA_REGISTRY_SERVICE_FACTORY_H_
