// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/autofill/core/browser/card_unmask_delegate.h"

namespace autofill {

CardUnmaskDelegate::UnmaskResponse::UnmaskResponse()
    : should_store_pan(false) {}

CardUnmaskDelegate::UnmaskResponse::UnmaskResponse(
    const UnmaskResponse& other) = default;

CardUnmaskDelegate::UnmaskResponse::~UnmaskResponse() {}

}  // namespace autofill
