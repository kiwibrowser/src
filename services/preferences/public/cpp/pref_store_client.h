// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_PREFERENCES_PUBLIC_CPP_PREF_STORE_CLIENT_H_
#define SERVICES_PREFERENCES_PUBLIC_CPP_PREF_STORE_CLIENT_H_

#include "base/macros.h"
#include "components/prefs/pref_store.h"
#include "services/preferences/public/cpp/pref_store_client_mixin.h"
#include "services/preferences/public/mojom/preferences.mojom.h"

namespace prefs {

// An implementation of PrefStore which uses prefs::mojom::PrefStore as
// the backing store of the preferences.
//
// PrefStoreClient provides synchronous access to the preferences stored by the
// backing store by caching them locally.
class PrefStoreClient : public PrefStoreClientMixin<::PrefStore> {
 public:
  explicit PrefStoreClient(mojom::PrefStoreConnectionPtr connection);

 private:
  ~PrefStoreClient() override;

  DISALLOW_COPY_AND_ASSIGN(PrefStoreClient);
};

}  // namespace prefs

#endif  // SERVICES_PREFERENCES_PUBLIC_CPP_PREF_STORE_CLIENT_H_
