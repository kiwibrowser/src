// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROMEOS_LOGIN_AUTH_AUTH_ATTEMPT_STATE_RESOLVER_H_
#define CHROMEOS_LOGIN_AUTH_AUTH_ATTEMPT_STATE_RESOLVER_H_

#include "chromeos/chromeos_export.h"

namespace chromeos {

class CHROMEOS_EXPORT AuthAttemptStateResolver {
 public:
  AuthAttemptStateResolver();
  virtual ~AuthAttemptStateResolver();
  // Gather existing status info and attempt to resolve it into one of a
  // set of discrete states.
  virtual void Resolve() = 0;
};

}  // namespace chromeos

#endif  // CHROMEOS_LOGIN_AUTH_AUTH_ATTEMPT_STATE_RESOLVER_H_
