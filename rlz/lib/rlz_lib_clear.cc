// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// The methods in this file belong conceptually to rlz_lib.cc. However, some
// programs depend on rlz only to call ClearAllProductEvents(), so this file
// contains this in fairly self-contained form to make it easier for linkers
// to strip away most of rlz. In particular, this file should not reference any
// symbols defined in financial_ping.cc.

#include "rlz/lib/rlz_lib.h"

#include "base/lazy_instance.h"
#include "rlz/lib/assert.h"
#include "rlz/lib/rlz_value_store.h"

namespace rlz_lib {

bool ClearAllProductEvents(Product product) {
  rlz_lib::ScopedRlzValueStoreLock lock;
  rlz_lib::RlzValueStore* store = lock.GetStore();
  if (!store || !store->HasAccess(rlz_lib::RlzValueStore::kWriteAccess))
    return false;

  bool result;
  result = store->ClearAllProductEvents(product);
  result &= store->ClearAllStatefulEvents(product);
  return result;
}

void ClearProductState(Product product, const AccessPoint* access_points) {
  rlz_lib::ScopedRlzValueStoreLock lock;
  rlz_lib::RlzValueStore* store = lock.GetStore();
  if (!store || !store->HasAccess(rlz_lib::RlzValueStore::kWriteAccess))
    return;

  // Delete all product specific state.
  VERIFY(ClearAllProductEvents(product));
  VERIFY(store->ClearPingTime(product));

  // Delete all RLZ's for access points being uninstalled.
  if (access_points) {
    for (int i = 0; access_points[i] != NO_ACCESS_POINT; i++) {
      VERIFY(store->ClearAccessPointRlz(access_points[i]));
    }
  }

  store->CollectGarbage();
}

static base::LazyInstance<std::string>::Leaky g_supplemental_branding;

SupplementaryBranding::SupplementaryBranding(const char* brand)
    : lock_(new ScopedRlzValueStoreLock) {
  if (!lock_->GetStore())
    return;

  if (!g_supplemental_branding.Get().empty()) {
    ASSERT_STRING("ProductBranding: existing brand is not empty");
    return;
  }

  if (brand == NULL || brand[0] == 0) {
    ASSERT_STRING("ProductBranding: new brand is empty");
    return;
  }

  g_supplemental_branding.Get() = brand;
}

SupplementaryBranding::~SupplementaryBranding() {
  if (lock_->GetStore())
    g_supplemental_branding.Get().clear();
  delete lock_;
}

// static
const std::string& SupplementaryBranding::GetBrand() {
  return g_supplemental_branding.Get();
}

}  // namespace rlz_lib
