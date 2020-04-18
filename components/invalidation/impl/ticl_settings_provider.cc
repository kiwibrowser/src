// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/invalidation/impl/ticl_settings_provider.h"

namespace invalidation {

TiclSettingsProvider::Observer::~Observer() {
}

TiclSettingsProvider::TiclSettingsProvider() {
}

TiclSettingsProvider::~TiclSettingsProvider() {
}

void TiclSettingsProvider::AddObserver(Observer* observer) {
  observers_.AddObserver(observer);
}

void TiclSettingsProvider::RemoveObserver(Observer* observer) {
  observers_.RemoveObserver(observer);
}

void TiclSettingsProvider::FireOnUseGCMChannelChanged() {
  for (auto& observer : observers_)
    observer.OnUseGCMChannelChanged();
}

}  // namespace invalidation
