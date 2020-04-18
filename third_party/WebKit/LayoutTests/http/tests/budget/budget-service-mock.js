/**
 * Mock implementation of the budget service.
 */

"use strict";

const TEST_BUDGET_COST = 1.2;
const TEST_BUDGET_AT = 2;
const TEST_BUDGET_TIME = new Date().getTime();

class BudgetServiceMock {
  constructor() {
    // Values to return for the next getBudget and getCost calls.
    this.cost_ = {};
    this.budget_ = [];
    this.error_ = blink.mojom.BudgetServiceErrorType.NONE;
    this.bindingSet_ = new mojo.BindingSet(blink.mojom.BudgetService);

    this.interceptor_ = new MojoInterfaceInterceptor(
        blink.mojom.BudgetService.name);
    this.interceptor_.oninterfacerequest =
        e => this.bindingSet_.addBinding(this, e.handle);
    this.interceptor_.start();
  }

  // This is called directly from test JavaScript to set up the return value
  // for the getCost Mojo call. The operationType mapping is needed to mirror
  // the mapping that happens in the Mojo layer.
  setCost(operationType, cost) {
    let mojoOperationType = blink.mojom.BudgetOperationType.INVALID_OPERATION;
    if (operationType == "silent-push")
      mojoOperationType = blink.mojom.BudgetOperationType.SILENT_PUSH;

    this.cost_[mojoOperationType] = cost;
  }


  // This is called directly from test JavaScript to set up the budget that is
  // returned from a later getBudget Mojo call. This adds an entry into the
  // budget array.
  addBudget(addTime, addBudget) {
    this.budget_.push({ time: addTime, budgetAt: addBudget });
  }

  // This is called from test JavaScript. It sets whether the next reserve
  // call should return success or not.
  setReserveSuccess(success) {
    this.success_ = success;
  }

  // Called from test JavaScript, this sets the error to be returned by the
  // Mojo service to the BudgetService in Blink. This error is never surfaced
  // to the test JavaScript, but the test code may get an exception if one of
  // these is set.
  setError(errorName) {
    switch (errorName) {
      case "database-error":
        this.error_ = blink.mojom.BudgetServiceErrorType.DATABASE_ERROR;
        break;
      case "not-supported":
        this.error_ = blink.mojom.BudgetServiceErrorType.NOT_SUPPORTED;
        break;
      case "no-error":
        this.error_ = blink.mojom.BudgetServiceErrorType.NONE;
        break;
    }
  }

  // This provides an implementation for the Mojo serivce getCost call.
  async getCost(operationType) {
    return { cost: this.cost_[operationType] };
  }

  // This provides an implementation for the Mojo serivce getBudget call.
  async getBudget() {
    return { errorType: this.error_, budget: this.budget_ };
  }

  // This provides an implementation for the Mojo serivce reserve call.
  async reserve() {
    return { errorType: this.error_, success: this.success_ };
  }
}

let budgetServiceMock = new BudgetServiceMock();
