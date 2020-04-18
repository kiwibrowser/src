// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_BUDGET_SERVICE_BUDGET_MANAGER_FACTORY_H_
#define CHROME_BROWSER_BUDGET_SERVICE_BUDGET_MANAGER_FACTORY_H_

#include "base/memory/singleton.h"
#include "components/keyed_service/content/browser_context_keyed_service_factory.h"

class BudgetManager;

class BudgetManagerFactory : public BrowserContextKeyedServiceFactory {
 public:
  static BudgetManager* GetForProfile(content::BrowserContext* context);
  static BudgetManagerFactory* GetInstance();

 private:
  friend struct base::DefaultSingletonTraits<BudgetManagerFactory>;

  BudgetManagerFactory();
  ~BudgetManagerFactory() override {}

  // BrowserContextKeyedServiceFactory interface.
  content::BrowserContext* GetBrowserContextToUse(
      content::BrowserContext* context) const override;
  KeyedService* BuildServiceInstanceFor(
      content::BrowserContext* context) const override;

  DISALLOW_COPY_AND_ASSIGN(BudgetManagerFactory);
};

#endif  //  CHROME_BROWSER_BUDGET_SERVICE_BUDGET_MANAGER_FACTORY_H_
