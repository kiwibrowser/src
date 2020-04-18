// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/network/network_usage_accumulator.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace network {

namespace {

struct BytesTransferredKey {
  uint32_t process_id;
  uint32_t routing_id;
};

}  // namespace

class NetworkUsageAccumulatorTest : public testing::Test {
 public:
  NetworkUsageAccumulatorTest() {}
  ~NetworkUsageAccumulatorTest() override {}

  void SimulateRawBytesTransferred(const BytesTransferredKey& key,
                                   int64_t bytes_received,
                                   int64_t bytes_sent) {
    network_usage_accumulator_.OnBytesTransferred(
        key.process_id, key.routing_id, bytes_received, bytes_sent);
  }

  mojom::NetworkUsage* GetUsageForKey(
      const std::vector<mojom::NetworkUsagePtr>& usages,
      const BytesTransferredKey& key) {
    for (const auto& usage : usages) {
      if (key.process_id == usage->process_id &&
          key.routing_id == usage->routing_id)
        return usage.get();
    }
    return nullptr;
  }

  void ClearBytesTransferredForProcess(uint32_t process_id) {
    network_usage_accumulator_.ClearBytesTransferredForProcess(process_id);
  }

  std::vector<mojom::NetworkUsagePtr> GetTotalNetworkUsages() const {
    return network_usage_accumulator_.GetTotalNetworkUsages();
  }

 private:
  NetworkUsageAccumulator network_usage_accumulator_;

  DISALLOW_COPY_AND_ASSIGN(NetworkUsageAccumulatorTest);
};

// Tests that the |process_id| and |routing_id| are used in the map correctly.
TEST_F(NetworkUsageAccumulatorTest, ChildRouteData) {
  BytesTransferredKey key = {100, 190};

  int64_t correct_read_bytes = 0;
  int64_t correct_sent_bytes = 0;

  int read_bytes_array[] = {900, 300, 100};
  int sent_bytes_array[] = {130, 153, 934};

  for (int i : read_bytes_array) {
    SimulateRawBytesTransferred(key, i, 0);
    correct_read_bytes += i;
  }
  for (int i : sent_bytes_array) {
    SimulateRawBytesTransferred(key, 0, i);
    correct_sent_bytes += i;
  }

  auto returned_usages = GetTotalNetworkUsages();
  EXPECT_EQ(1U, returned_usages.size());
  EXPECT_EQ(correct_sent_bytes,
            GetUsageForKey(returned_usages, key)->total_bytes_sent);
  EXPECT_EQ(correct_read_bytes,
            GetUsageForKey(returned_usages, key)->total_bytes_received);
}

// Tests that two distinct |process_id| and |routing_id| pairs are tracked
// separately in the unordered map.
TEST_F(NetworkUsageAccumulatorTest, TwoChildRouteData) {
  BytesTransferredKey key1 = {32, 1};
  BytesTransferredKey key2 = {17, 2};

  int64_t correct_read_bytes1 = 0;
  int64_t correct_sent_bytes1 = 0;

  int64_t correct_read_bytes2 = 0;
  int64_t correct_sent_bytes2 = 0;

  int read_bytes_array1[] = {453, 987654, 946650};
  int sent_bytes_array1[] = {138450, 1556473, 954434};

  int read_bytes_array2[] = {905643, 324340, 654150};
  int sent_bytes_array2[] = {1232138, 157312, 965464};

  for (int i : read_bytes_array1) {
    SimulateRawBytesTransferred(key1, i, 0);
    correct_read_bytes1 += i;
  }
  for (int i : sent_bytes_array1) {
    SimulateRawBytesTransferred(key1, 0, i);
    correct_sent_bytes1 += i;
  }

  for (int i : read_bytes_array2) {
    SimulateRawBytesTransferred(key2, i, 0);
    correct_read_bytes2 += i;
  }
  for (int i : sent_bytes_array2) {
    SimulateRawBytesTransferred(key2, 0, i);
    correct_sent_bytes2 += i;
  }

  auto returned_usages = GetTotalNetworkUsages();
  EXPECT_EQ(2U, returned_usages.size());
  EXPECT_EQ(correct_sent_bytes1,
            GetUsageForKey(returned_usages, key1)->total_bytes_sent);
  EXPECT_EQ(correct_read_bytes1,
            GetUsageForKey(returned_usages, key1)->total_bytes_received);
  EXPECT_EQ(correct_sent_bytes2,
            GetUsageForKey(returned_usages, key2)->total_bytes_sent);
  EXPECT_EQ(correct_read_bytes2,
            GetUsageForKey(returned_usages, key2)->total_bytes_received);
}

// Tests that two keys with the same |process_id| and |routing_id| are tracked
// together in the accumulator.
TEST_F(NetworkUsageAccumulatorTest, TwoSameChildRouteData) {
  BytesTransferredKey key1 = {123, 456};
  BytesTransferredKey key2 = {123, 456};

  int64_t correct_read_bytes = 0;
  int64_t correct_sent_bytes = 0;

  int read_bytes_array[] = {90440, 12300, 103420};
  int sent_bytes_array[] = {44130, 12353, 93234};

  for (int i : read_bytes_array) {
    SimulateRawBytesTransferred(key1, i, 0);
    correct_read_bytes += i;
  }
  for (int i : sent_bytes_array) {
    SimulateRawBytesTransferred(key1, 0, i);
    correct_sent_bytes += i;
  }

  for (int i : read_bytes_array) {
    SimulateRawBytesTransferred(key2, i, 0);
    correct_read_bytes += i;
  }
  for (int i : sent_bytes_array) {
    SimulateRawBytesTransferred(key2, 0, i);
    correct_sent_bytes += i;
  }

  auto returned_usages = GetTotalNetworkUsages();
  EXPECT_EQ(1U, returned_usages.size());
  EXPECT_EQ(correct_sent_bytes,
            GetUsageForKey(returned_usages, key1)->total_bytes_sent);
  EXPECT_EQ(correct_read_bytes,
            GetUsageForKey(returned_usages, key1)->total_bytes_received);
  EXPECT_EQ(correct_sent_bytes,
            GetUsageForKey(returned_usages, key2)->total_bytes_sent);
  EXPECT_EQ(correct_read_bytes,
            GetUsageForKey(returned_usages, key2)->total_bytes_received);
}

// Tests that the map can handle two process_ids with the same routing_id.
TEST_F(NetworkUsageAccumulatorTest, SameRouteDifferentProcesses) {
  BytesTransferredKey key1 = {12, 143};
  BytesTransferredKey key2 = {13, 143};

  int64_t correct_read_bytes1 = 0;
  int64_t correct_sent_bytes1 = 0;

  int64_t correct_read_bytes2 = 0;
  int64_t correct_sent_bytes2 = 0;

  int read_bytes_array1[] = {453, 98754, 94650};
  int sent_bytes_array1[] = {1350, 15643, 95434};

  int read_bytes_array2[] = {905643, 3243, 654150};
  int sent_bytes_array2[] = {12338, 157312, 9664};

  for (int i : read_bytes_array1) {
    SimulateRawBytesTransferred(key1, i, 0);
    correct_read_bytes1 += i;
  }
  for (int i : sent_bytes_array1) {
    SimulateRawBytesTransferred(key1, 0, i);
    correct_sent_bytes1 += i;
  }

  for (int i : read_bytes_array2) {
    SimulateRawBytesTransferred(key2, i, 0);
    correct_read_bytes2 += i;
  }
  for (int i : sent_bytes_array2) {
    SimulateRawBytesTransferred(key2, 0, i);
    correct_sent_bytes2 += i;
  }

  auto returned_usages = GetTotalNetworkUsages();
  EXPECT_EQ(2U, returned_usages.size());
  EXPECT_EQ(correct_sent_bytes1,
            GetUsageForKey(returned_usages, key1)->total_bytes_sent);
  EXPECT_EQ(correct_read_bytes1,
            GetUsageForKey(returned_usages, key1)->total_bytes_received);
  EXPECT_EQ(correct_sent_bytes2,
            GetUsageForKey(returned_usages, key2)->total_bytes_sent);
  EXPECT_EQ(correct_read_bytes2,
            GetUsageForKey(returned_usages, key2)->total_bytes_received);
}

// Tests that process data is cleared after termination.
TEST_F(NetworkUsageAccumulatorTest, ClearAfterTermination) {
  // |key1| and |key2| belongs to the same process.
  BytesTransferredKey key1 = {100, 190};
  BytesTransferredKey key2 = {100, 191};
  BytesTransferredKey key3 = {101, 191};

  // No data has been transferred yet.
  auto returned_usages = GetTotalNetworkUsages();
  EXPECT_EQ(0U, returned_usages.size());
  EXPECT_EQ(nullptr, GetUsageForKey(returned_usages, key1));
  EXPECT_EQ(nullptr, GetUsageForKey(returned_usages, key2));
  EXPECT_EQ(nullptr, GetUsageForKey(returned_usages, key3));

  // Simulate data transfer on all three keys.
  SimulateRawBytesTransferred(key1, 100, 1);
  SimulateRawBytesTransferred(key2, 2, 200);
  SimulateRawBytesTransferred(key3, 33, 333);
  returned_usages = GetTotalNetworkUsages();
  // Should have data observed on all three keys.
  EXPECT_EQ(3U, returned_usages.size());
  EXPECT_NE(nullptr, GetUsageForKey(returned_usages, key1));
  EXPECT_NE(nullptr, GetUsageForKey(returned_usages, key2));
  EXPECT_NE(nullptr, GetUsageForKey(returned_usages, key3));

  // Simulate process termination on the first process.
  ClearBytesTransferredForProcess(key1.process_id);

  // |key1| and |key2| should both be cleared.
  returned_usages = GetTotalNetworkUsages();
  EXPECT_EQ(1U, returned_usages.size());
  EXPECT_EQ(nullptr, GetUsageForKey(returned_usages, key1));
  EXPECT_EQ(nullptr, GetUsageForKey(returned_usages, key2));
  // |key3| shouldn't be affected.
  EXPECT_NE(nullptr, GetUsageForKey(returned_usages, key3));
}

// Tests that the map can store both types of keys and that it does update after
// a process has gone.
TEST_F(NetworkUsageAccumulatorTest, MultipleWavesMixedData) {
  BytesTransferredKey key1 = {12, 143};
  BytesTransferredKey key2 = {0, 0};

  int64_t correct_read_bytes1 = 0;
  int64_t correct_sent_bytes1 = 0;

  int read_bytes_array1[] = {453, 98754, 94650};
  int sent_bytes_array1[] = {1350, 15643, 95434};

  for (int i : read_bytes_array1) {
    SimulateRawBytesTransferred(key1, i, 0);
    correct_read_bytes1 += i;
  }
  for (int i : sent_bytes_array1) {
    SimulateRawBytesTransferred(key1, 0, i);
    correct_sent_bytes1 += i;
  }

  auto returned_usages = GetTotalNetworkUsages();
  EXPECT_NE(nullptr, GetUsageForKey(returned_usages, key1));
  // |key2| has not been used yet so it shouldn't exist in the returned usages.
  EXPECT_EQ(nullptr, GetUsageForKey(returned_usages, key2));

  SimulateRawBytesTransferred(key2, 0, 10);
  returned_usages = GetTotalNetworkUsages();
  EXPECT_NE(nullptr, GetUsageForKey(returned_usages, key2));

  ClearBytesTransferredForProcess(key1.process_id);
  ClearBytesTransferredForProcess(key2.process_id);

  correct_sent_bytes1 = 0;
  correct_read_bytes1 = 0;

  SimulateRawBytesTransferred(key1, 0, 10);
  correct_sent_bytes1 += 10;

  returned_usages = GetTotalNetworkUsages();
  EXPECT_EQ(1U, returned_usages.size());
  EXPECT_EQ(correct_sent_bytes1,
            GetUsageForKey(returned_usages, key1)->total_bytes_sent);
  EXPECT_EQ(correct_read_bytes1,
            GetUsageForKey(returned_usages, key1)->total_bytes_received);
  // |key2| has been cleared.
  EXPECT_EQ(nullptr, GetUsageForKey(returned_usages, key2));
  ClearBytesTransferredForProcess(key1.process_id);

  correct_read_bytes1 = 0;
  correct_sent_bytes1 = 0;

  int correct_read_bytes2 = 0;
  int correct_sent_bytes2 = 0;

  int read_bytes_array_second_1[] = {4153, 987154, 946501};
  int sent_bytes_array_second_1[] = {13510, 115643, 954134};

  int read_bytes_array2[] = {9056243, 32243, 6541250};
  int sent_bytes_array2[] = {123238, 1527312, 96624};

  for (int i : read_bytes_array_second_1) {
    SimulateRawBytesTransferred(key1, i, 0);
    correct_read_bytes1 += i;
  }
  for (int i : sent_bytes_array_second_1) {
    SimulateRawBytesTransferred(key1, 0, i);
    correct_sent_bytes1 += i;
  }

  for (int i : read_bytes_array2) {
    SimulateRawBytesTransferred(key2, i, 0);
    correct_read_bytes2 += i;
  }
  for (int i : sent_bytes_array2) {
    SimulateRawBytesTransferred(key2, 0, i);
    correct_sent_bytes2 += i;
  }

  returned_usages = GetTotalNetworkUsages();
  EXPECT_EQ(2U, returned_usages.size());
  EXPECT_EQ(correct_sent_bytes1,
            GetUsageForKey(returned_usages, key1)->total_bytes_sent);
  EXPECT_EQ(correct_read_bytes1,
            GetUsageForKey(returned_usages, key1)->total_bytes_received);
  EXPECT_EQ(correct_sent_bytes2,
            GetUsageForKey(returned_usages, key2)->total_bytes_sent);
  EXPECT_EQ(correct_read_bytes2,
            GetUsageForKey(returned_usages, key2)->total_bytes_received);
}

}  // namespace network
