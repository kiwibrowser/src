// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_SIGNIN_CORE_BROWSER_SCOPED_UNIFIED_CONSENT_H_
#define COMPONENTS_SIGNIN_CORE_BROWSER_SCOPED_UNIFIED_CONSENT_H_

#include <memory>

#include "base/macros.h"
#include "base/test/scoped_feature_list.h"
#include "components/signin/core/browser/profile_management_switches.h"

namespace signin {

// Changes the unified consent feature state while it is in scope. Useful for
// tests.
class ScopedUnifiedConsent {
 public:
  explicit ScopedUnifiedConsent(UnifiedConsentFeatureState state);
  ~ScopedUnifiedConsent();

 private:
  base::test::ScopedFeatureList scoped_feature_list_;

  DISALLOW_COPY_AND_ASSIGN(ScopedUnifiedConsent);
};

}  // namespace signin

#endif  // COMPONENTS_SIGNIN_CORE_BROWSER_SCOPED_UNIFIED_CONSENT_H_
