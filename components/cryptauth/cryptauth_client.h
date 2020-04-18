// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_CRYPTAUTH_CRYPTAUTH_CLIENT_H_
#define COMPONENTS_CRYPTAUTH_CRYPTAUTH_CLIENT_H_

#include <memory>
#include <string>

#include "base/callback_forward.h"
#include "base/macros.h"
#include "net/traffic_annotation/network_traffic_annotation.h"

namespace cryptauth {
class GetMyDevicesRequest;
class GetMyDevicesResponse;
class FindEligibleUnlockDevicesRequest;
class FindEligibleUnlockDevicesResponse;
class SendDeviceSyncTickleRequest;
class SendDeviceSyncTickleResponse;
class ToggleEasyUnlockRequest;
class ToggleEasyUnlockResponse;
class SetupEnrollmentRequest;
class SetupEnrollmentResponse;
class FinishEnrollmentRequest;
class FinishEnrollmentResponse;
class FindEligibleForPromotionRequest;
class FindEligibleForPromotionResponse;
}

namespace cryptauth {

// Interface for making API requests to the CryptAuth service, which
// manages cryptographic credentials (ie. public keys) for a user's devices.
// Implmentations shall only processes a single request, so create a new
// instance for each request you make. DO NOT REUSE.
// For documentation on each API call, see
// components/cryptauth/proto/cryptauth_api.proto
class CryptAuthClient {
 public:
  typedef base::Callback<void(const std::string&)> ErrorCallback;

  virtual ~CryptAuthClient() {}

  // GetMyDevices
  typedef base::Callback<void(const GetMyDevicesResponse&)>
      GetMyDevicesCallback;
  virtual void GetMyDevices(const GetMyDevicesRequest& request,
                            const GetMyDevicesCallback& callback,
                            const ErrorCallback& error_callback,
                            const net::PartialNetworkTrafficAnnotationTag&
                                partial_traffic_annotation) = 0;

  // FindEligibleUnlockDevices
  typedef base::Callback<void(
      const FindEligibleUnlockDevicesResponse&)>
      FindEligibleUnlockDevicesCallback;
  virtual void FindEligibleUnlockDevices(
      const FindEligibleUnlockDevicesRequest& request,
      const FindEligibleUnlockDevicesCallback& callback,
      const ErrorCallback& error_callback) = 0;

  // FindEligibleForPromotion
  typedef base::Callback<void(const FindEligibleForPromotionResponse&)>
      FindEligibleForPromotionCallback;
  virtual void FindEligibleForPromotion(
      const FindEligibleForPromotionRequest& request,
      const FindEligibleForPromotionCallback& callback,
      const ErrorCallback& error_callback) = 0;

  // SendDeviceSyncTickle
  typedef base::Callback<void(const SendDeviceSyncTickleResponse&)>
      SendDeviceSyncTickleCallback;
  virtual void SendDeviceSyncTickle(
      const SendDeviceSyncTickleRequest& request,
      const SendDeviceSyncTickleCallback& callback,
      const ErrorCallback& error_callback,
      const net::PartialNetworkTrafficAnnotationTag&
          partial_traffic_annotation) = 0;

  // ToggleEasyUnlock
  typedef base::Callback<void(const ToggleEasyUnlockResponse&)>
      ToggleEasyUnlockCallback;
  virtual void ToggleEasyUnlock(
      const ToggleEasyUnlockRequest& request,
      const ToggleEasyUnlockCallback& callback,
      const ErrorCallback& error_callback) = 0;

  // SetupEnrollment
  typedef base::Callback<void(const SetupEnrollmentResponse&)>
      SetupEnrollmentCallback;
  virtual void SetupEnrollment(const SetupEnrollmentRequest& request,
                               const SetupEnrollmentCallback& callback,
                               const ErrorCallback& error_callback) = 0;

  // FinishEnrollment
  typedef base::Callback<void(const FinishEnrollmentResponse&)>
      FinishEnrollmentCallback;
  virtual void FinishEnrollment(
      const FinishEnrollmentRequest& request,
      const FinishEnrollmentCallback& callback,
      const ErrorCallback& error_callback) = 0;

  // Returns the access token used to make the request. If no request has been
  // made yet, this function will return an empty string.
  virtual std::string GetAccessTokenUsed() = 0;
};

// Interface for creating CryptAuthClient instances. Because each
// CryptAuthClient instance can only be used for one API call, a factory makes
// it easier to make multiple requests in sequence or in parallel.
class CryptAuthClientFactory {
 public:
  virtual ~CryptAuthClientFactory() {}

  virtual std::unique_ptr<CryptAuthClient> CreateInstance() = 0;
};

}  // namespace cryptauth

#endif  // COMPONENTS_CRYPTAUTH_CRYPTAUTH_CLIENT_H_
