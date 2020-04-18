// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_BUDGET_SERVICE_BUDGET_MANAGER_H_
#define CHROME_BROWSER_BUDGET_SERVICE_BUDGET_MANAGER_H_

#include <map>
#include <memory>

#include "base/callback_forward.h"
#include "base/gtest_prod_util.h"
#include "chrome/browser/budget_service/budget_database.h"
#include "components/keyed_service/core/keyed_service.h"
#include "third_party/blink/public/platform/modules/budget_service/budget_service.mojom.h"

class Profile;

namespace url {
class Origin;
}

// A budget manager to help Chrome decide how much background work a service
// worker should be able to do on behalf of the user. The budget is calculated
// based on the Site Engagment Score and is consumed when a origin does
// background work on behalf of the user.
class BudgetManager : public KeyedService {
 public:
  explicit BudgetManager(Profile* profile);
  ~BudgetManager() override;

  // Query for the base cost for any background processing.
  static double GetCost(blink::mojom::BudgetOperationType type);

  using GetBudgetCallback = blink::mojom::BudgetService::GetBudgetCallback;
  using ReserveCallback = blink::mojom::BudgetService::ReserveCallback;
  using ConsumeCallback = base::OnceCallback<void(bool success)>;

  // Get the budget associated with the origin. This is passed to the
  // callback. Budget will be a sequence of points describing the time and
  // the budget at that time.
  void GetBudget(const url::Origin& origin, GetBudgetCallback callback);

  // Spend enough budget to cover the cost of the desired action and create
  // a reservation for that action. If this returns true to the callback, then
  // the next action will consume that reservation and not cost any budget.
  void Reserve(const url::Origin& origin,
               blink::mojom::BudgetOperationType type,
               ReserveCallback callback);

  // Spend budget, first consuming a reservation if one exists, or spend
  // directly from the budget.
  void Consume(const url::Origin& origin,
               blink::mojom::BudgetOperationType type,
               ConsumeCallback callback);

 private:
  friend class BudgetManagerTest;

  void DidGetBudget(GetBudgetCallback callback,
                    blink::mojom::BudgetServiceErrorType error,
                    std::vector<blink::mojom::BudgetStatePtr> budget);

  void DidConsume(ConsumeCallback callback,
                  blink::mojom::BudgetServiceErrorType error,
                  bool success);

  void DidReserve(const url::Origin& origin,
                  ReserveCallback callback,
                  blink::mojom::BudgetServiceErrorType error,
                  bool success);

  Profile* profile_;
  BudgetDatabase db_;

  std::map<url::Origin, int> reservation_map_;
  base::WeakPtrFactory<BudgetManager> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(BudgetManager);
};

#endif  // CHROME_BROWSER_BUDGET_SERVICE_BUDGET_MANAGER_H_
