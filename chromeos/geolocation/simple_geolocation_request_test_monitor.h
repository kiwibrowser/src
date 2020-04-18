// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROMEOS_GEOLOCATION_SIMPLE_GEOLOCATION_REQUEST_TEST_MONITOR_H_
#define CHROMEOS_GEOLOCATION_SIMPLE_GEOLOCATION_REQUEST_TEST_MONITOR_H_

#include "base/macros.h"
#include "chromeos/chromeos_export.h"

namespace chromeos {

class SimpleGeolocationRequest;

// This is global hook, that allows to monitor SimpleGeolocationRequest
// in tests.
//
// Note: we need CHROMEOS_EXPORT for tests.
class CHROMEOS_EXPORT SimpleGeolocationRequestTestMonitor {
 public:
  SimpleGeolocationRequestTestMonitor();

  virtual ~SimpleGeolocationRequestTestMonitor();
  virtual void OnRequestCreated(SimpleGeolocationRequest* request);
  virtual void OnStart(SimpleGeolocationRequest* request);

 private:
  DISALLOW_COPY_AND_ASSIGN(SimpleGeolocationRequestTestMonitor);
};

}  // namespace chromeos

#endif  // CHROMEOS_GEOLOCATION_SIMPLE_GEOLOCATION_REQUEST_TEST_MONITOR_H_
