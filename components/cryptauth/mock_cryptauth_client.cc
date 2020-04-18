// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/cryptauth/mock_cryptauth_client.h"

#include <utility>

#include "base/callback.h"

namespace cryptauth {

MockCryptAuthClient::MockCryptAuthClient() {
}

MockCryptAuthClient::~MockCryptAuthClient() {
}

MockCryptAuthClientFactory::MockCryptAuthClientFactory(MockType mock_type)
    : mock_type_(mock_type) {
}

MockCryptAuthClientFactory::~MockCryptAuthClientFactory() {
}

std::unique_ptr<CryptAuthClient> MockCryptAuthClientFactory::CreateInstance() {
  std::unique_ptr<MockCryptAuthClient> client;
  if (mock_type_ == MockType::MAKE_STRICT_MOCKS)
    client.reset(new testing::StrictMock<MockCryptAuthClient>());
  else
    client.reset(new testing::NiceMock<MockCryptAuthClient>());

  for (auto& observer : observer_list_)
    observer.OnCryptAuthClientCreated(client.get());
  return std::move(client);
}

void MockCryptAuthClientFactory::AddObserver(Observer* observer) {
  observer_list_.AddObserver(observer);
}

void MockCryptAuthClientFactory::RemoveObserver(Observer* observer) {
  observer_list_.RemoveObserver(observer);
}

}  // namespace cryptauth
