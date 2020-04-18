// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_AUTOFILL_IOS_BROWSER_AUTOFILL_DRIVER_IOS_BRIDGE_H_
#define COMPONENTS_AUTOFILL_IOS_BROWSER_AUTOFILL_DRIVER_IOS_BRIDGE_H_

#include <stdint.h>

#include <vector>

#include "components/autofill/core/common/form_data_predictions.h"

namespace autofill {
struct FormData;
}

// Interface used to pipe form data from AutofillDriverIOS to the embedder.
@protocol AutofillDriverIOSBridge

- (void)onFormDataFilled:(uint16_t)query_id
                  result:(const autofill::FormData&)result;

- (void)sendAutofillTypePredictionsToRenderer:
    (const std::vector<autofill::FormDataPredictions>&)forms;

@end

#endif  // COMPONENTS_AUTOFILL_IOS_BROWSER_AUTOFILL_DRIVER_IOS_BRIDGE_H_
