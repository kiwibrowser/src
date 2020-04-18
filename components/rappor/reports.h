// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_RAPPOR_REPORTS_H_
#define COMPONENTS_RAPPOR_REPORTS_H_

#include "components/rappor/reports.h"

#include "components/rappor/byte_vector_utils.h"

namespace rappor {

namespace internal {

// Generate a randomized report for a single metric/field.
ByteVector GenerateReport(const std::string& secret,
                          const NoiseParameters& parameters,
                          const ByteVector& value);

}  // namespace internal

}  // namespace rappor

#endif  // COMPONENTS_RAPPOR_REPORTS_H_
