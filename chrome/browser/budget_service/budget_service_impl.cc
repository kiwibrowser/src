// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/budget_service/budget_service_impl.h"

#include "chrome/browser/budget_service/budget_manager.h"
#include "chrome/browser/budget_service/budget_manager_factory.h"
#include "chrome/browser/permissions/permission_manager.h"
#include "chrome/browser/permissions/permission_manager_factory.h"
#include "chrome/browser/profiles/profile.h"
#include "content/public/browser/permission_type.h"
#include "content/public/browser/render_process_host.h"
#include "mojo/public/cpp/bindings/strong_binding.h"

BudgetServiceImpl::BudgetServiceImpl(int render_process_id,
                                     const url::Origin& origin)
    : render_process_id_(render_process_id), origin_(origin) {}

BudgetServiceImpl::~BudgetServiceImpl() = default;

// static
void BudgetServiceImpl::Create(blink::mojom::BudgetServiceRequest request,
                               content::RenderProcessHost* host,
                               const url::Origin& origin) {
  mojo::MakeStrongBinding(
      std::make_unique<BudgetServiceImpl>(host->GetID(), origin),
      std::move(request));
}

void BudgetServiceImpl::GetCost(blink::mojom::BudgetOperationType type,
                                GetCostCallback callback) {
  // The RenderProcessHost should still be alive as long as any connections are
  // alive, and if the BudgetService mojo connection is down, the
  // BudgetServiceImpl should have been destroyed.
  content::RenderProcessHost* host =
      content::RenderProcessHost::FromID(render_process_id_);
  DCHECK(host);

  // Query the BudgetManager for the cost and return it.
  content::BrowserContext* context = host->GetBrowserContext();
  double cost = BudgetManagerFactory::GetForProfile(context)->GetCost(type);
  std::move(callback).Run(cost);
}

void BudgetServiceImpl::GetBudget(GetBudgetCallback callback) {
  // The RenderProcessHost should still be alive as long as any connections are
  // alive, and if the BudgetService mojo connection is down, the
  // BudgetServiceImpl should have been destroyed.
  content::RenderProcessHost* host =
      content::RenderProcessHost::FromID(render_process_id_);
  DCHECK(host);

  Profile* profile = Profile::FromBrowserContext(host->GetBrowserContext());

  PermissionManager* permission_manager =
      PermissionManagerFactory::GetForProfile(profile);
  DCHECK(permission_manager);

  // By request of the Privacy Team, we only communicate the budget buckets with
  // the developer when the notification permission has been granted. This is
  // something the impact of which has to be reconsidered when the feature is
  // ready to ship for real. See https://crbug.com/710809 for context.
  if (permission_manager->GetPermissionStatus(
          content::PermissionType::NOTIFICATIONS, origin_.GetURL(),
          origin_.GetURL()) != blink::mojom::PermissionStatus::GRANTED) {
    blink::mojom::BudgetStatePtr empty_state(blink::mojom::BudgetState::New());
    empty_state->budget_at = 0;
    empty_state->time =
        base::Time::Now().ToDoubleT() * base::Time::kMillisecondsPerSecond;

    std::vector<blink::mojom::BudgetStatePtr> predictions;
    predictions.push_back(std::move(empty_state));

    std::move(callback).Run(blink::mojom::BudgetServiceErrorType::NONE,
                            std::move(predictions));
    return;
  }

  // Query the BudgetManager for the budget.
  BudgetManagerFactory::GetForProfile(profile)->GetBudget(origin_,
                                                          std::move(callback));
}

void BudgetServiceImpl::Reserve(blink::mojom::BudgetOperationType operation,
                                ReserveCallback callback) {
  // The RenderProcessHost should still be alive as long as any connections are
  // alive, and if the BudgetService mojo connection is down, the
  // BudgetServiceImpl should have been destroyed.
  content::RenderProcessHost* host =
      content::RenderProcessHost::FromID(render_process_id_);
  DCHECK(host);

  // Request a reservation from the BudgetManager.
  content::BrowserContext* context = host->GetBrowserContext();
  BudgetManagerFactory::GetForProfile(context)->Reserve(origin_, operation,
                                                        std::move(callback));
}
