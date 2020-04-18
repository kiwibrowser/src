// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_SYNC_DRIVER_SYNC_TYPE_PREFERENCE_PROVIDER_H_
#define COMPONENTS_SYNC_DRIVER_SYNC_TYPE_PREFERENCE_PROVIDER_H_

#include "components/sync/base/model_type.h"

namespace syncer {

class SyncTypePreferenceProvider {
 public:
  virtual ModelTypeSet GetPreferredDataTypes() const = 0;

 protected:
  virtual ~SyncTypePreferenceProvider() {}
};

}  // namespace syncer

#endif  // COMPONENTS_SYNC_DRIVER_SYNC_TYPE_PREFERENCE_PROVIDER_H_
