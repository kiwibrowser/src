// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/certificate_transparency/features.h"

namespace certificate_transparency {

// Enables or disables auditing Certificate Transparency logs over DNS.
const base::Feature kCTLogAuditing = {"CertificateTransparencyLogAuditing",
                                      base::FEATURE_DISABLED_BY_DEFAULT};

}  // namespace certificate_transparency
