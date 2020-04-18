// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_PAYMENTS_CORE_PAYMENT_REQUEST_DELEGATE_H_
#define COMPONENTS_PAYMENTS_CORE_PAYMENT_REQUEST_DELEGATE_H_

#include "components/payments/core/payment_request_base_delegate.h"

namespace payments {

class PaymentRequest;

class PaymentRequestDelegate : public PaymentRequestBaseDelegate {
 public:
  ~PaymentRequestDelegate() override {}

  // Shows the Payment Request dialog for the given |request|.
  virtual void ShowDialog(PaymentRequest* request) = 0;

  // Closes the same dialog that was opened by this delegate. Must be safe to
  // call when the dialog is not showing.
  virtual void CloseDialog() = 0;

  // Disables the dialog and shows an error message that the transaction has
  // failed.
  virtual void ShowErrorMessage() = 0;

  // Disables user interaction by showing a spinner.
  virtual void ShowProcessingSpinner() = 0;

  // Returns whether the browser window is active.
  virtual bool IsBrowserWindowActive() const = 0;
};

}  // namespace payments

#endif  // COMPONENTS_PAYMENTS_CORE_PAYMENT_REQUEST_DELEGATE_H_
