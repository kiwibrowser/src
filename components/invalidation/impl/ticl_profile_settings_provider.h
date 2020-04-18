// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_INVALIDATION_IMPL_TICL_PROFILE_SETTINGS_PROVIDER_H_
#define COMPONENTS_INVALIDATION_IMPL_TICL_PROFILE_SETTINGS_PROVIDER_H_

#include "base/compiler_specific.h"
#include "base/macros.h"
#include "components/invalidation/impl/ticl_settings_provider.h"
#include "components/prefs/pref_change_registrar.h"

class PrefService;

namespace invalidation {

// A specialization of TiclSettingsProvider that reads settings from user prefs.
class TiclProfileSettingsProvider : public TiclSettingsProvider {
 public:
  explicit TiclProfileSettingsProvider(PrefService* prefs);
  ~TiclProfileSettingsProvider() override;

  // TiclInvalidationServiceSettingsProvider:
  bool UseGCMChannel() const override;

 private:
  PrefChangeRegistrar registrar_;
  PrefService* const prefs_;

  DISALLOW_COPY_AND_ASSIGN(TiclProfileSettingsProvider);
};

}  // namespace invalidation

#endif  // COMPONENTS_INVALIDATION_IMPL_TICL_PROFILE_SETTINGS_PROVIDER_H_
