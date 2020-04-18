// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_INVALIDATION_IMPL_TICL_SETTINGS_PROVIDER_H_
#define COMPONENTS_INVALIDATION_IMPL_TICL_SETTINGS_PROVIDER_H_

#include "base/macros.h"
#include "base/observer_list.h"

namespace invalidation {

// Provides configuration settings to TiclInvalidationService and notifies it
// when the settings change.
class TiclSettingsProvider {
 public:
  class Observer {
   public:
    virtual void OnUseGCMChannelChanged() = 0;

   protected:
    virtual ~Observer();
  };

  TiclSettingsProvider();
  virtual ~TiclSettingsProvider();

  virtual bool UseGCMChannel() const = 0;

  void AddObserver(Observer* observer);
  void RemoveObserver(Observer* observer);

 protected:
  void FireOnUseGCMChannelChanged();

 private:
  base::ObserverList<Observer, true> observers_;

  DISALLOW_COPY_AND_ASSIGN(TiclSettingsProvider);
};

}  // namespace invalidation

#endif  // COMPONENTS_INVALIDATION_IMPL_TICL_SETTINGS_PROVIDER_H_
