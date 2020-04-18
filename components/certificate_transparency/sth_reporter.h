// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_CERTIFICATE_TRANSPARENCY_STH_REPORTER_H_
#define COMPONENTS_CERTIFICATE_TRANSPARENCY_STH_REPORTER_H_

#include <set>

namespace certificate_transparency {

class STHObserver;

// Interface for registering/unregistering observers.
class STHReporter {
 public:
  virtual ~STHReporter() {}

  virtual void RegisterObserver(STHObserver* observer) = 0;
  virtual void UnregisterObserver(STHObserver* observer) = 0;
};

}  // namespace certificate_transparency

#endif  // COMPONENTS_CERTIFICATE_TRANSPARENCY_STH_REPORTER_H_
