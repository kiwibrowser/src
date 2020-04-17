// Copyright 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "osp/public/endpoint_request_ids.h"

#include "third_party/googletest/src/googletest/include/gtest/gtest.h"

namespace openscreen {

// These tests validate RequestId generation for two endpoints with IDs 3 and 7.

TEST(EndpointRequestIdsTest, StrictlyIncreasingRequestIdSequence) {
  EndpointRequestIds request_ids_client(EndpointRequestIds::Role::kClient);

  EXPECT_EQ(0u, request_ids_client.GetNextRequestId(7));
  EXPECT_EQ(2u, request_ids_client.GetNextRequestId(7));
  EXPECT_EQ(4u, request_ids_client.GetNextRequestId(7));
  EXPECT_EQ(0u, request_ids_client.GetNextRequestId(3));
  EXPECT_EQ(6u, request_ids_client.GetNextRequestId(7));
  EXPECT_EQ(2u, request_ids_client.GetNextRequestId(3));

  EndpointRequestIds request_ids_server(EndpointRequestIds::Role::kServer);
  EXPECT_EQ(1u, request_ids_server.GetNextRequestId(7));
  EXPECT_EQ(3u, request_ids_server.GetNextRequestId(7));
  EXPECT_EQ(5u, request_ids_server.GetNextRequestId(7));
  EXPECT_EQ(1u, request_ids_server.GetNextRequestId(3));
  EXPECT_EQ(7u, request_ids_server.GetNextRequestId(7));
  EXPECT_EQ(3u, request_ids_server.GetNextRequestId(3));
}

TEST(EndpointRequestIdsTest, ResetRequestId) {
  EndpointRequestIds request_ids_client(EndpointRequestIds::Role::kClient);

  EXPECT_EQ(0u, request_ids_client.GetNextRequestId(7));
  EXPECT_EQ(2u, request_ids_client.GetNextRequestId(7));
  request_ids_client.ResetRequestId(7);
  EXPECT_EQ(0u, request_ids_client.GetNextRequestId(7));
  EXPECT_EQ(2u, request_ids_client.GetNextRequestId(7));
  EXPECT_EQ(0u, request_ids_client.GetNextRequestId(3));
  EXPECT_EQ(2u, request_ids_client.GetNextRequestId(3));
  request_ids_client.ResetRequestId(7);
  EXPECT_EQ(0u, request_ids_client.GetNextRequestId(7));
  EXPECT_EQ(2u, request_ids_client.GetNextRequestId(7));
  EXPECT_EQ(4u, request_ids_client.GetNextRequestId(3));
  EXPECT_EQ(6u, request_ids_client.GetNextRequestId(3));

  EndpointRequestIds request_ids_server(EndpointRequestIds::Role::kServer);

  EXPECT_EQ(1u, request_ids_server.GetNextRequestId(7));
  EXPECT_EQ(3u, request_ids_server.GetNextRequestId(7));
  request_ids_server.ResetRequestId(7);
  EXPECT_EQ(1u, request_ids_server.GetNextRequestId(7));
  EXPECT_EQ(3u, request_ids_server.GetNextRequestId(7));
  EXPECT_EQ(1u, request_ids_server.GetNextRequestId(3));
  EXPECT_EQ(3u, request_ids_server.GetNextRequestId(3));
  request_ids_server.ResetRequestId(7);
  EXPECT_EQ(1u, request_ids_server.GetNextRequestId(7));
  EXPECT_EQ(3u, request_ids_server.GetNextRequestId(7));
  EXPECT_EQ(5u, request_ids_server.GetNextRequestId(3));
  EXPECT_EQ(7u, request_ids_server.GetNextRequestId(3));
}

TEST(EndpointRequestIdsTest, ResetAll) {
  EndpointRequestIds request_ids_client(EndpointRequestIds::Role::kClient);

  EXPECT_EQ(0u, request_ids_client.GetNextRequestId(7));
  EXPECT_EQ(2u, request_ids_client.GetNextRequestId(7));
  EXPECT_EQ(0u, request_ids_client.GetNextRequestId(3));
  EXPECT_EQ(2u, request_ids_client.GetNextRequestId(3));
  request_ids_client.Reset();
  EXPECT_EQ(0u, request_ids_client.GetNextRequestId(7));
  EXPECT_EQ(0u, request_ids_client.GetNextRequestId(3));

  EndpointRequestIds request_ids_server(EndpointRequestIds::Role::kServer);

  EXPECT_EQ(1u, request_ids_server.GetNextRequestId(7));
  EXPECT_EQ(3u, request_ids_server.GetNextRequestId(7));
  EXPECT_EQ(1u, request_ids_server.GetNextRequestId(3));
  EXPECT_EQ(3u, request_ids_server.GetNextRequestId(3));
  request_ids_server.Reset();
  EXPECT_EQ(1u, request_ids_server.GetNextRequestId(7));
  EXPECT_EQ(1u, request_ids_server.GetNextRequestId(3));
}

}  // namespace openscreen
