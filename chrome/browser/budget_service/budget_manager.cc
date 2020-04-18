// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/budget_service/budget_manager.h"

#include <stdint.h>

#include "base/callback.h"
#include "base/metrics/histogram_macros.h"
#include "base/threading/thread_task_runner_handle.h"
#include "base/time/time.h"
#include "chrome/browser/engagement/site_engagement_score.h"
#include "chrome/browser/profiles/profile.h"
#include "content/public/common/origin_util.h"
#include "third_party/blink/public/platform/modules/budget_service/budget_service.mojom.h"
#include "url/origin.h"

BudgetManager::BudgetManager(Profile* profile)
    : profile_(profile),
      db_(profile,
          profile->GetPath().Append(FILE_PATH_LITERAL("BudgetDatabase"))),
      weak_ptr_factory_(this) {}

BudgetManager::~BudgetManager() {}

// static
double BudgetManager::GetCost(blink::mojom::BudgetOperationType type) {
  switch (type) {
    case blink::mojom::BudgetOperationType::SILENT_PUSH:
      return 2.0;
    case blink::mojom::BudgetOperationType::INVALID_OPERATION:
      return SiteEngagementScore::kMaxPoints + 1;
      // No default case.
  }
  NOTREACHED();
  return SiteEngagementScore::kMaxPoints + 1.0;
}

void BudgetManager::GetBudget(const url::Origin& origin,
                              GetBudgetCallback callback) {
  if (origin.unique() || !content::IsOriginSecure(origin.GetURL())) {
    std::move(callback).Run(blink::mojom::BudgetServiceErrorType::NOT_SUPPORTED,
                            std::vector<blink::mojom::BudgetStatePtr>());
    return;
  }
  db_.GetBudgetDetails(origin, base::BindOnce(&BudgetManager::DidGetBudget,
                                              weak_ptr_factory_.GetWeakPtr(),
                                              std::move(callback)));
}

void BudgetManager::Reserve(const url::Origin& origin,
                            blink::mojom::BudgetOperationType type,
                            ReserveCallback callback) {
  if (origin.unique() || !content::IsOriginSecure(origin.GetURL())) {
    std::move(callback).Run(blink::mojom::BudgetServiceErrorType::NOT_SUPPORTED,
                            false /* success */);
    return;
  }
  db_.SpendBudget(
      origin, GetCost(type),
      base::Bind(&BudgetManager::DidReserve, weak_ptr_factory_.GetWeakPtr(),
                 origin, base::Passed(&callback)));
}

void BudgetManager::Consume(const url::Origin& origin,
                            blink::mojom::BudgetOperationType type,
                            ConsumeCallback callback) {
  if (origin.unique() || !content::IsOriginSecure(origin.GetURL())) {
    std::move(callback).Run(false /* success */);
    return;
  }

  bool found_reservation = false;

  // First, see if there is a reservation already.
  auto count = reservation_map_.find(origin);
  if (count != reservation_map_.end()) {
    if (count->second == 1)
      reservation_map_.erase(origin);
    else
      reservation_map_[origin]--;
    found_reservation = true;
  }

  if (found_reservation) {
    std::move(callback).Run(true);
    return;
  }

  // If there wasn't a reservation already, try to directly consume budget.
  // The callback will return directly to the caller.
  db_.SpendBudget(
      origin, GetCost(type),
      base::Bind(&BudgetManager::DidConsume, weak_ptr_factory_.GetWeakPtr(),
                 base::Passed(&callback)));
}

void BudgetManager::DidGetBudget(
    GetBudgetCallback callback,
    const blink::mojom::BudgetServiceErrorType error,
    std::vector<blink::mojom::BudgetStatePtr> budget) {
  // If there was an error, just record a budget of 0, so the API query is still
  // counted.
  double budget_at = 0;
  if (error == blink::mojom::BudgetServiceErrorType::NONE)
    budget_at = budget[0]->budget_at;
  UMA_HISTOGRAM_COUNTS_100("Blink.BudgetAPI.QueryBudget", budget_at);

  std::move(callback).Run(error, std::move(budget));
}

void BudgetManager::DidConsume(ConsumeCallback callback,
                               blink::mojom::BudgetServiceErrorType error,
                               bool success) {
  // The caller of Consume only cares whether it succeeded or failed and not
  // why. So, only return a combined bool.
  if (error != blink::mojom::BudgetServiceErrorType::NONE) {
    std::move(callback).Run(false /* success */);
    return;
  }
  std::move(callback).Run(success);
}

void BudgetManager::DidReserve(const url::Origin& origin,
                               ReserveCallback callback,
                               blink::mojom::BudgetServiceErrorType error,
                               bool success) {
  // If the call succeeded, write the new reservation into the map.
  if (success && error == blink::mojom::BudgetServiceErrorType::NONE)
    reservation_map_[origin]++;

  UMA_HISTOGRAM_BOOLEAN("Blink.BudgetAPI.Reserve", success);

  std::move(callback).Run(error, success);
}
