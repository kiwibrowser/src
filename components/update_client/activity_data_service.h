// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_UPDATE_CLIENT_ACTIVITY_DATA_SERVICE_H_
#define COMPONENTS_UPDATE_CLIENT_ACTIVITY_DATA_SERVICE_H_

#include <memory>
#include <string>

#include "base/macros.h"

namespace update_client {

const int kDateFirstTime = -1;
const int kDaysFirstTime = -1;
const int kDateUnknown = -2;
const int kDaysUnknown = -2;

// This is an interface that injects certain update information (active, days
// since ...) into the update engine of the update client.
// GetDaysSinceLastActive and GetDaysSinceLastRollCall are used for backward
// compatibility.
class ActivityDataService {
 public:
  // Returns the current state of the active bit of the specified |id|.
  virtual bool GetActiveBit(const std::string& id) const = 0;

  // Clears the active bit of the specified |id|.
  virtual void ClearActiveBit(const std::string& id) = 0;

  // The following 2 functions return the number of days since last
  // active/roll call.
  virtual int GetDaysSinceLastActive(const std::string& id) const = 0;
  virtual int GetDaysSinceLastRollCall(const std::string& id) const = 0;

  virtual ~ActivityDataService() {}
};

}  // namespace update_client

#endif  // COMPONENTS_UPDATE_CLIENT_ACTIVITY_DATA_SERVICE_H_
