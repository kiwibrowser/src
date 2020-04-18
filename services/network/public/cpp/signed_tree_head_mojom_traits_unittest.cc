// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/network/public/cpp/signed_tree_head_mojom_traits.h"

#include "base/time/time.h"
#include "mojo/public/cpp/base/time_mojom_traits.h"
#include "mojo/public/cpp/test_support/test_utils.h"
#include "net/cert/signed_certificate_timestamp.h"
#include "net/cert/signed_tree_head.h"
#include "net/test/ct_test_util.h"
#include "services/network/public/cpp/digitally_signed_mojom_traits.h"
#include "services/network/public/mojom/signed_tree_head.mojom.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace network {
namespace {

TEST(SignedTreeHeadTraitsTest, Roundtrips) {
  net::ct::SignedTreeHead original;
  net::ct::SignedTreeHead copied;

  // First try with a populated STH.
  ASSERT_TRUE(net::ct::GetSampleSignedTreeHead(&original));
  EXPECT_TRUE(mojo::test::SerializeAndDeserialize<mojom::SignedTreeHead>(
      &original, &copied));
  EXPECT_EQ(original, copied);

  // Then try an STH for an empty tree.
  ASSERT_TRUE(net::ct::GetSampleEmptySignedTreeHead(&original));
  EXPECT_TRUE(mojo::test::SerializeAndDeserialize<mojom::SignedTreeHead>(
      &original, &copied));
  EXPECT_EQ(original, copied);

  // Then try a syntactically-valid but semantically-bad STH.
  ASSERT_TRUE(net::ct::GetBadEmptySignedTreeHead(&original));
  EXPECT_TRUE(mojo::test::SerializeAndDeserialize<mojom::SignedTreeHead>(
      &original, &copied));
  EXPECT_EQ(original, copied);
}

TEST(SignedTreeHeadTraitsTest, RequiresLogId) {
  net::ct::SignedTreeHead original;
  ASSERT_TRUE(net::ct::GetSampleSignedTreeHead(&original));
  original.log_id.clear();

  net::ct::SignedTreeHead copied;
  EXPECT_FALSE(mojo::test::SerializeAndDeserialize<mojom::SignedTreeHead>(
      &original, &copied));
}

TEST(SignedTreeHeadTraitsTest, RequiresValidDigitallySigned) {
  net::ct::SignedTreeHead original;
  ASSERT_TRUE(net::ct::GetSampleSignedTreeHead(&original));
  original.signature.signature_data.clear();

  net::ct::SignedTreeHead copied;
  EXPECT_FALSE(mojo::test::SerializeAndDeserialize<mojom::SignedTreeHead>(
      &original, &copied));
}

}  // namespace
}  // namespace network
