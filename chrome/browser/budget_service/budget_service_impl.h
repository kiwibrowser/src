// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_BUDGET_SERVICE_BUDGET_SERVICE_IMPL_H_
#define CHROME_BROWSER_BUDGET_SERVICE_BUDGET_SERVICE_IMPL_H_

#include "base/macros.h"
#include "third_party/blink/public/platform/modules/budget_service/budget_service.mojom.h"
#include "url/origin.h"

namespace content {
class RenderProcessHost;
}

// Implementation of the BudgetService Mojo service provided by the browser
// layer. It is responsible for dispatching budget requests to the
// BudgetManager.
class BudgetServiceImpl : public blink::mojom::BudgetService {
 public:
  BudgetServiceImpl(int render_process_id, const url::Origin& origin);
  ~BudgetServiceImpl() override;

  static void Create(blink::mojom::BudgetServiceRequest request,
                     content::RenderProcessHost* host,
                     const url::Origin& origin);

  // blink::mojom::BudgetService implementation.
  void GetCost(blink::mojom::BudgetOperationType operation,
               GetCostCallback callback) override;
  void GetBudget(GetBudgetCallback callback) override;
  void Reserve(blink::mojom::BudgetOperationType operation,
               ReserveCallback callback) override;

 private:
  // Render process ID is used to get the browser context.
  const int render_process_id_;

  const url::Origin origin_;

  DISALLOW_COPY_AND_ASSIGN(BudgetServiceImpl);
};

#endif  // CHROME_BROWSER_BUDGET_SERVICE_BUDGET_SERVICE_IMPL_H_
