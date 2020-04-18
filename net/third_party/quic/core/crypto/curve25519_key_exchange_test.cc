// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "net/third_party/quic/core/crypto/curve25519_key_exchange.h"

#include <memory>

#include "net/third_party/quic/core/crypto/quic_random.h"
#include "net/third_party/quic/platform/api/quic_string.h"
#include "net/third_party/quic/platform/api/quic_test.h"

namespace net {
namespace test {

class Curve25519KeyExchangeTest : public QuicTest {};

// SharedKey just tests that the basic key exchange identity holds: that both
// parties end up with the same key.
TEST_F(Curve25519KeyExchangeTest, SharedKey) {
  QuicRandom* const rand = QuicRandom::GetInstance();

  for (int i = 0; i < 5; i++) {
    const QuicString alice_key(Curve25519KeyExchange::NewPrivateKey(rand));
    const QuicString bob_key(Curve25519KeyExchange::NewPrivateKey(rand));

    std::unique_ptr<Curve25519KeyExchange> alice(
        Curve25519KeyExchange::New(alice_key));
    std::unique_ptr<Curve25519KeyExchange> bob(
        Curve25519KeyExchange::New(bob_key));

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
