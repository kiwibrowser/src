// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_PLATFORM_LOADER_TESTING_CRYPTO_TESTING_PLATFORM_SUPPORT_H_
#define THIRD_PARTY_BLINK_RENDERER_PLATFORM_LOADER_TESTING_CRYPTO_TESTING_PLATFORM_SUPPORT_H_

#include <memory>
#include "third_party/blink/renderer/platform/loader/testing/fetch_testing_platform_support.h"
#include "third_party/blink/renderer/platform/testing/mock_web_crypto.h"
#include "third_party/blink/renderer/platform/wtf/allocator.h"

namespace blink {

class CryptoTestingPlatformSupport : public FetchTestingPlatformSupport {
 public:
  CryptoTestingPlatformSupport() = default;
  ~CryptoTestingPlatformSupport() override = default;

  // Platform:
  WebCrypto* Crypto() override { return mock_web_crypto_.get(); }

  class SetMockCryptoScope final {
    STACK_ALLOCATED();

   public:
    explicit SetMockCryptoScope(CryptoTestingPlatformSupport& platform)
        : platform_(platform) {
      DCHECK(!platform_.Crypto());
      platform_.SetMockCrypto(MockWebCrypto::Create());
    }
    ~SetMockCryptoScope() { platform_.SetMockCrypto(nullptr); }
    MockWebCrypto& MockCrypto() { return *platform_.mock_web_crypto_; }

   private:
    CryptoTestingPlatformSupport& platform_;
  };

 private:
  void SetMockCrypto(std::unique_ptr<MockWebCrypto> crypto) {
    mock_web_crypto_ = std::move(crypto);
  }

  std::unique_ptr<MockWebCrypto> mock_web_crypto_;

  DISALLOW_COPY_AND_ASSIGN(CryptoTestingPlatformSupport);
};

}  // namespace blink

#endif
