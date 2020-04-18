// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_CRYPTAUTH_CRYPTAUTH_ENROLLER_H_
#define COMPONENTS_CRYPTAUTH_CRYPTAUTH_ENROLLER_H_

#include <memory>
#include <string>

#include "base/callback_forward.h"
#include "components/cryptauth/proto/cryptauth_api.pb.h"

namespace cryptauth {

// Interface for enrolling a device with CryptAuth.
class CryptAuthEnroller {
 public:
  virtual ~CryptAuthEnroller() {}

  // Enrolls the device:
  // |user_public_key|: The user's persistent public key identifying the device.
  // |user_private_key|: The corresponding private key to |user_public_key|.
  // |device_info|: Contains information about the local device. Note that the
  //     enroller may change fields in this proto before it is finally uploaded.
  // |invocation_reason|: The reason why the enrollment occurred.
  // |callback|: Called will be called with true if the enrollment
  //     succeeds and false otherwise.
  typedef base::Callback<void(bool)> EnrollmentFinishedCallback;
  virtual void Enroll(const std::string& user_public_key,
                      const std::string& user_private_key,
                      const GcmDeviceInfo& device_info,
                      InvocationReason invocation_reason,
                      const EnrollmentFinishedCallback& callback) = 0;
};

// Interface for creating CryptAuthEnroller instances.
class CryptAuthEnrollerFactory {
 public:
  virtual ~CryptAuthEnrollerFactory() {}

  virtual std::unique_ptr<CryptAuthEnroller> CreateInstance() = 0;
};

}  // namespace cryptauth

#endif  // COMPONENTS_CRYPTAUTH_CRYPTAUTH_ENROLLER_H_
