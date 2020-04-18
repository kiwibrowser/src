// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "net/third_party/quic/core/crypto/p256_key_exchange.h"

#include <memory>

#include "net/third_party/quic/platform/api/quic_string.h"
#include "net/third_party/quic/platform/api/quic_test.h"

namespace net {
namespace test {

class P256KeyExchangeTest : public QuicTest {};

// SharedKey just tests that the basic key exchange identity holds: that both
// parties end up with the same key.
TEST_F(P256KeyExchangeTest, SharedKey) {
  for (int i = 0; i < 5; i++) {
    QuicString alice_private(P256KeyExchange::NewPrivateKey());
    QuicString bob_private(P256KeyExchange::NewPrivateKey());

    ASSERT_FALSE(alice_private.empty());
    ASSERT_FALSE(bob_private.empty());
    ASSERT_NE(alice_private, bob_private);

    std::unique_ptr<P256KeyExchange> alice(P256KeyExchange::New(alice_private));
    std::unique_ptr<P256KeyExchange> bob(P256KeyExchange::New(bob_private));

    ASSERT_TRUE(alice != nullptr);
    ASSERT_TRUE(bob != nullptr);

    const QuicStringPiece alice_public(alice->public_value());
    const QuicStringPiece bob_public(bob->public_value());

    QuicString alice_shared, bob_shared;
    ASSERT_TRUE(alice->CalculateSharedKey(bob_public, &alice_shared));
    ASSERT_TRUE(bob->CalculateSharedKey(alice_public, &bob_shared));
    ASSERT_EQ(alice_shared, bob_shared);
  }
}

}  // namespace test
}  // namespace net
