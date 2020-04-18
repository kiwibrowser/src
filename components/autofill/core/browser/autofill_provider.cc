// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/autofill/core/browser/autofill_provider.h"

#include "components/autofill/core/browser/autofill_handler_proxy.h"

namespace autofill {

AutofillProvider::AutofillProvider() {}

AutofillProvider::~AutofillProvider() {}

void AutofillProvider::SendFormDataToRenderer(AutofillHandlerProxy* handler,
                                              int requestId,
                                              const FormData& formData) {
  handler->SendFormDataToRenderer(
      requestId, AutofillDriver::FORM_DATA_ACTION_FILL, formData);
}

}  // namespace autofil
