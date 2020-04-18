// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_CRYPTAUTH_MOCK_CRYPTAUTH_CLIENT_H_
#define COMPONENTS_CRYPTAUTH_MOCK_CRYPTAUTH_CLIENT_H_

#include "base/macros.h"
#include "base/observer_list.h"
#include "components/cryptauth/cryptauth_client.h"
#include "components/cryptauth/proto/cryptauth_api.pb.h"
#include "testing/gmock/include/gmock/gmock.h"

namespace cryptauth {

class MockCryptAuthClient : public CryptAuthClient {
 public:
  MockCryptAuthClient();
  ~MockCryptAuthClient() override;

  // CryptAuthClient:
  MOCK_METHOD4(GetMyDevices,
               void(const GetMyDevicesRequest& request,
                    const GetMyDevicesCallback& callback,
                    const ErrorCallback& error_callback,
                    const net::PartialNetworkTrafficAnnotationTag&
                        partial_traffic_annotation));
  MOCK_METHOD3(FindEligibleUnlockDevices,
               void(const FindEligibleUnlockDevicesRequest& request,
                    const FindEligibleUnlockDevicesCallback& callback,
                    const ErrorCallback& error_callback));
  MOCK_METHOD3(FindEligibleForPromotion,
               void(const FindEligibleForPromotionRequest& request,
                    const FindEligibleForPromotionCallback& callback,
                    const ErrorCallback& error_callback));
  MOCK_METHOD4(SendDeviceSyncTickle,
               void(const SendDeviceSyncTickleRequest& request,
                    const SendDeviceSyncTickleCallback& callback,
                    const ErrorCallback& error_callback,
                    const net::PartialNetworkTrafficAnnotationTag&
                        partial_traffic_annotation));
  MOCK_METHOD3(ToggleEasyUnlock,
               void(const ToggleEasyUnlockRequest& request,
                    const ToggleEasyUnlockCallback& callback,
                    const ErrorCallback& error_callback));
  MOCK_METHOD3(SetupEnrollment,
               void(const SetupEnrollmentRequest& request,
                    const SetupEnrollmentCallback& callback,
                    const ErrorCallback& error_callback));
  MOCK_METHOD3(FinishEnrollment,
               void(const FinishEnrollmentRequest& request,
                    const FinishEnrollmentCallback& callback,
                    const ErrorCallback& error_callback));
  MOCK_METHOD0(GetAccessTokenUsed, std::string());

 private:
  DISALLOW_COPY_AND_ASSIGN(MockCryptAuthClient);
};

class MockCryptAuthClientFactory : public CryptAuthClientFactory {
 public:
  class Observer {
   public:
    // Called with the new instance when it is requested from the factory,
    // allowing expectations to be set. Ownership of |client| will be taken by
    // the caller of CreateInstance().
    virtual void OnCryptAuthClientCreated(MockCryptAuthClient* client) = 0;
  };

  // Represents the type of mock instances to create.
  enum class MockType { MAKE_NICE_MOCKS, MAKE_STRICT_MOCKS };

  // If |mock_type| is STRICT, then StrictMocks will be created. Otherwise,
  // NiceMocks will be created.
  explicit MockCryptAuthClientFactory(MockType mock_type);
  ~MockCryptAuthClientFactory() override;

  // CryptAuthClientFactory:
  std::unique_ptr<CryptAuthClient> CreateInstance() override;

  void AddObserver(Observer* observer);
  void RemoveObserver(Observer* observer);

 private:
  // Whether to create StrictMocks or NiceMocks.
  const MockType mock_type_;

  // Observers of the factory.
  base::ObserverList<Observer> observer_list_;

  DISALLOW_COPY_AND_ASSIGN(MockCryptAuthClientFactory);
};

}  // namespace cryptauth

#endif  // COMPONENTS_CRYPTAUTH_MOCK_CRYPTAUTH_CLIENT_H_
