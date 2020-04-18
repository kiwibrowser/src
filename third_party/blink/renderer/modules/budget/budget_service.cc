// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/modules/budget/budget_service.h"

#include "services/service_manager/public/cpp/interface_provider.h"
#include "third_party/blink/public/platform/modules/budget_service/budget_service.mojom-blink.h"
#include "third_party/blink/public/platform/platform.h"
#include "third_party/blink/renderer/bindings/core/v8/script_promise.h"
#include "third_party/blink/renderer/bindings/core/v8/script_promise_resolver.h"
#include "third_party/blink/renderer/core/dom/dom_exception.h"
#include "third_party/blink/renderer/core/dom/exception_code.h"
#include "third_party/blink/renderer/core/execution_context/execution_context.h"
#include "third_party/blink/renderer/modules/budget/budget_state.h"
#include "third_party/blink/renderer/platform/bindings/script_state.h"

namespace blink {
namespace {

mojom::blink::BudgetOperationType StringToOperationType(
    const AtomicString& operation) {
  if (operation == "silent-push")
    return mojom::blink::BudgetOperationType::SILENT_PUSH;

  return mojom::blink::BudgetOperationType::INVALID_OPERATION;
}

DOMException* ErrorTypeToException(mojom::blink::BudgetServiceErrorType error) {
  switch (error) {
    case mojom::blink::BudgetServiceErrorType::NONE:
      return nullptr;
    case mojom::blink::BudgetServiceErrorType::DATABASE_ERROR:
      return DOMException::Create(kDataError,
                                  "Error reading the budget database.");
    case mojom::blink::BudgetServiceErrorType::NOT_SUPPORTED:
      return DOMException::Create(kNotSupportedError,
                                  "Requested opration was not supported");
  }
  NOTREACHED();
  return nullptr;
}

}  // namespace

BudgetService::BudgetService(
    service_manager::InterfaceProvider* interface_provider) {
  interface_provider->GetInterface(mojo::MakeRequest(&service_));

  // Set a connection error handler, so that if an embedder doesn't
  // implement a BudgetSerice mojo service, the developer will get a
  // actionable information.
  service_.set_connection_error_handler(
      WTF::Bind(&BudgetService::OnConnectionError, WrapWeakPersistent(this)));
}

BudgetService::~BudgetService() = default;

ScriptPromise BudgetService::getCost(ScriptState* script_state,
                                     const AtomicString& operation) {
  DCHECK(service_);

  mojom::blink::BudgetOperationType type = StringToOperationType(operation);
  DCHECK_NE(type, mojom::blink::BudgetOperationType::INVALID_OPERATION);

  ScriptPromiseResolver* resolver = ScriptPromiseResolver::Create(script_state);
  ScriptPromise promise = resolver->Promise();

  // Get the cost for the action from the browser BudgetService.
  service_->GetCost(type,
                    WTF::Bind(&BudgetService::GotCost, WrapPersistent(this),
                              WrapPersistent(resolver)));
  return promise;
}

void BudgetService::GotCost(ScriptPromiseResolver* resolver,
                            double cost) const {
  resolver->Resolve(cost);
}

ScriptPromise BudgetService::getBudget(ScriptState* script_state) {
  DCHECK(service_);

  ScriptPromiseResolver* resolver = ScriptPromiseResolver::Create(script_state);
  ScriptPromise promise = resolver->Promise();

  // Get the budget from the browser BudgetService.
  service_->GetBudget(WTF::Bind(&BudgetService::GotBudget, WrapPersistent(this),
                                WrapPersistent(resolver)));
  return promise;
}

void BudgetService::GotBudget(
    ScriptPromiseResolver* resolver,
    mojom::blink::BudgetServiceErrorType error,
    const WTF::Vector<mojom::blink::BudgetStatePtr> expectations) const {
  if (error != mojom::blink::BudgetServiceErrorType::NONE) {
    resolver->Reject(ErrorTypeToException(error));
    return;
  }

  // Copy the chunks into the budget array.
  HeapVector<Member<BudgetState>> budget(expectations.size());
  for (size_t i = 0; i < expectations.size(); i++) {
    // Return the largest integer less than the budget, so it's easier for
    // developer to reason about budget. Flooring is also significant from a
    // privacy perspective, as we don't want to share precise data as it could
    // aid fingerprinting. See https://crbug.com/710809.
    budget[i] = new BudgetState(floor(expectations[i]->budget_at),
                                expectations[i]->time);
  }

  resolver->Resolve(budget);
}

ScriptPromise BudgetService::reserve(ScriptState* script_state,
                                     const AtomicString& operation) {
  DCHECK(service_);

  mojom::blink::BudgetOperationType type = StringToOperationType(operation);
  DCHECK_NE(type, mojom::blink::BudgetOperationType::INVALID_OPERATION);

  ScriptPromiseResolver* resolver = ScriptPromiseResolver::Create(script_state);
  ScriptPromise promise = resolver->Promise();

  // Call to the BudgetService to place the reservation.
  service_->Reserve(
      type, WTF::Bind(&BudgetService::GotReservation, WrapPersistent(this),
                      WrapPersistent(resolver)));
  return promise;
}

void BudgetService::GotReservation(ScriptPromiseResolver* resolver,
                                   mojom::blink::BudgetServiceErrorType error,
                                   bool success) const {
  if (error != mojom::blink::BudgetServiceErrorType::NONE) {
    resolver->Reject(ErrorTypeToException(error));
    return;
  }

  resolver->Resolve(success);
}

void BudgetService::OnConnectionError() {
  LOG(ERROR) << "Unable to connect to the Mojo BudgetService.";
  // TODO(harkness): Reject in flight promises.
}

}  // namespace blink
