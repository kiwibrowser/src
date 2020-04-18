// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/run_loop.h"
#include "base/test/scoped_mock_time_message_loop_task_runner.h"
#include "chrome/browser/task_manager/sampling/task_manager_impl.h"
#include "content/public/test/test_browser_thread_bundle.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace task_manager {

class TaskManagerIoThreadHelperTest : public testing::Test {
 public:
  TaskManagerIoThreadHelperTest()
      : helper_manager_(
            base::BindRepeating(&TaskManagerIoThreadHelperTest::GotData,
                                base::Unretained(this))) {
    base::RunLoop().RunUntilIdle();
  }
  ~TaskManagerIoThreadHelperTest() override {}

  void GotData(BytesTransferredMap params) {
    EXPECT_TRUE(returned_map_.empty()) << "GotData() delivered twice";
    EXPECT_FALSE(params.empty()) << "GotData() called with empty map";
    returned_map_ = params;
  }

 protected:
  BytesTransferredMap returned_map_;

 private:
  content::TestBrowserThreadBundle thread_bundle_;
  IoThreadHelperManager helper_manager_;

  DISALLOW_COPY_AND_ASSIGN(TaskManagerIoThreadHelperTest);
};

// Tests that the |child_id| and |route_id| are used in the map correctly.
TEST_F(TaskManagerIoThreadHelperTest, ChildRouteData) {
  base::ScopedMockTimeMessageLoopTaskRunner mock_main_runner;

  BytesTransferredKey key = {100, 190};

  int64_t correct_read_bytes = 0;
  int64_t correct_sent_bytes = 0;

  int read_bytes_array[] = {900, 300, 100};
  int sent_bytes_array[] = {130, 153, 934};

  for (int i : read_bytes_array) {
    TaskManagerIoThreadHelper::OnRawBytesTransferred(key, i, 0);
    correct_read_bytes += i;
  }
  for (int i : sent_bytes_array) {
    TaskManagerIoThreadHelper::OnRawBytesTransferred(key, 0, i);
    correct_sent_bytes += i;
  }

  EXPECT_TRUE(mock_main_runner->HasPendingTask());
  mock_main_runner->FastForwardBy(base::TimeDelta::FromSeconds(1));
  base::RunLoop().RunUntilIdle();
  EXPECT_FALSE(mock_main_runner->HasPendingTask());
  EXPECT_EQ(1U, returned_map_.size());
  EXPECT_EQ(correct_sent_bytes, returned_map_[key].byte_sent_count);
  EXPECT_EQ(correct_read_bytes, returned_map_[key].byte_read_count);
}

// Tests that two distinct |child_id| and |route_id| pairs are tracked
// separately in the unordered map.
TEST_F(TaskManagerIoThreadHelperTest, TwoChildRouteData) {
  base::ScopedMockTimeMessageLoopTaskRunner mock_main_runner;

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
    TaskManagerIoThreadHelper::OnRawBytesTransferred(key1, i, 0);
    correct_read_bytes1 += i;
  }
  for (int i : sent_bytes_array1) {
    TaskManagerIoThreadHelper::OnRawBytesTransferred(key1, 0, i);
    correct_sent_bytes1 += i;
  }

  for (int i : read_bytes_array2) {
    TaskManagerIoThreadHelper::OnRawBytesTransferred(key2, i, 0);
    correct_read_bytes2 += i;
  }
  for (int i : sent_bytes_array2) {
    TaskManagerIoThreadHelper::OnRawBytesTransferred(key2, 0, i);
    correct_sent_bytes2 += i;
  }

  EXPECT_TRUE(mock_main_runner->HasPendingTask());
  mock_main_runner->FastForwardBy(base::TimeDelta::FromSeconds(1));
  base::RunLoop().RunUntilIdle();
  const unsigned long number_of_keys = 2;
  EXPECT_EQ(number_of_keys, returned_map_.size());
  EXPECT_FALSE(mock_main_runner->HasPendingTask());
  EXPECT_EQ(correct_sent_bytes1, returned_map_[key1].byte_sent_count);
  EXPECT_EQ(correct_read_bytes1, returned_map_[key1].byte_read_count);
  EXPECT_EQ(correct_sent_bytes2, returned_map_[key2].byte_sent_count);
  EXPECT_EQ(correct_read_bytes2, returned_map_[key2].byte_read_count);
}

// Tests that two keys with the same |child_id| and |route_id| are tracked
// together in the unordered map.
TEST_F(TaskManagerIoThreadHelperTest, TwoSameChildRouteData) {
  base::ScopedMockTimeMessageLoopTaskRunner mock_main_runner;

  BytesTransferredKey key1 = {123, 456};
  BytesTransferredKey key2 = {123, 456};

  int64_t correct_read_bytes = 0;
  int64_t correct_sent_bytes = 0;

  int read_bytes_array[] = {90440, 12300, 103420};
  int sent_bytes_array[] = {44130, 12353, 93234};

  for (int i : read_bytes_array) {
    TaskManagerIoThreadHelper::OnRawBytesTransferred(key1, i, 0);
    correct_read_bytes += i;
  }
  for (int i : sent_bytes_array) {
    TaskManagerIoThreadHelper::OnRawBytesTransferred(key1, 0, i);
    correct_sent_bytes += i;
  }

  for (int i : read_bytes_array) {
    TaskManagerIoThreadHelper::OnRawBytesTransferred(key2, i, 0);
    correct_read_bytes += i;
  }
  for (int i : sent_bytes_array) {
    TaskManagerIoThreadHelper::OnRawBytesTransferred(key2, 0, i);
    correct_sent_bytes += i;
  }

  mock_main_runner->FastForwardBy(base::TimeDelta::FromSeconds(1));
  base::RunLoop().RunUntilIdle();
  EXPECT_EQ(1U, returned_map_.size());
  EXPECT_EQ(correct_sent_bytes, returned_map_[key1].byte_sent_count);
  EXPECT_EQ(correct_read_bytes, returned_map_[key1].byte_read_count);
  EXPECT_EQ(correct_sent_bytes, returned_map_[key2].byte_sent_count);
  EXPECT_EQ(correct_read_bytes, returned_map_[key2].byte_read_count);
}

// Tests that the map can handle two child_ids with the same route_id.
TEST_F(TaskManagerIoThreadHelperTest, SameRouteDifferentProcesses) {
  base::ScopedMockTimeMessageLoopTaskRunner mock_main_runner;
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
    TaskManagerIoThreadHelper::OnRawBytesTransferred(key1, i, 0);
    correct_read_bytes1 += i;
  }
  for (int i : sent_bytes_array1) {
    TaskManagerIoThreadHelper::OnRawBytesTransferred(key1, 0, i);
    correct_sent_bytes1 += i;
  }

  for (int i : read_bytes_array2) {
    TaskManagerIoThreadHelper::OnRawBytesTransferred(key2, i, 0);
    correct_read_bytes2 += i;
  }
  for (int i : sent_bytes_array2) {
    TaskManagerIoThreadHelper::OnRawBytesTransferred(key2, 0, i);
    correct_sent_bytes2 += i;
  }

  mock_main_runner->FastForwardBy(base::TimeDelta::FromSeconds(1));
  base::RunLoop().RunUntilIdle();
  EXPECT_EQ(2U, returned_map_.size());
  EXPECT_EQ(correct_sent_bytes1, returned_map_[key1].byte_sent_count);
  EXPECT_EQ(correct_read_bytes1, returned_map_[key1].byte_read_count);
  EXPECT_EQ(correct_sent_bytes2, returned_map_[key2].byte_sent_count);
  EXPECT_EQ(correct_read_bytes2, returned_map_[key2].byte_read_count);
}

// Tests that the map can store both types of keys and that it does update after
// 1 mocked second passes and there is new traffic.
TEST_F(TaskManagerIoThreadHelperTest, MultipleWavesMixedData) {
  base::ScopedMockTimeMessageLoopTaskRunner mock_main_runner;

  BytesTransferredKey key1 = {12, 143};
  BytesTransferredKey key2 = {-1, -1};

  int64_t correct_read_bytes1 = 0;
  int64_t correct_sent_bytes1 = 0;

  int read_bytes_array1[] = {453, 98754, 94650};
  int sent_bytes_array1[] = {1350, 15643, 95434};

  for (int i : read_bytes_array1) {
    TaskManagerIoThreadHelper::OnRawBytesTransferred(key1, i, 0);
    correct_read_bytes1 += i;
  }
  for (int i : sent_bytes_array1) {
    TaskManagerIoThreadHelper::OnRawBytesTransferred(key1, 0, i);
    correct_sent_bytes1 += i;
  }

  mock_main_runner->FastForwardBy(base::TimeDelta::FromSeconds(1));
  base::RunLoop().RunUntilIdle();
  returned_map_.clear();

  correct_sent_bytes1 = 0;
  correct_read_bytes1 = 0;

  TaskManagerIoThreadHelper::OnRawBytesTransferred(key1, 0, 10);
  correct_sent_bytes1 += 10;

  mock_main_runner->FastForwardBy(base::TimeDelta::FromSeconds(1));
  base::RunLoop().RunUntilIdle();
  EXPECT_EQ(1U, returned_map_.size());
  EXPECT_EQ(correct_sent_bytes1, returned_map_[key1].byte_sent_count);
  EXPECT_EQ(correct_read_bytes1, returned_map_[key1].byte_read_count);
  // |key2| has not been used yet so it should call the implicit constructor
  // which starts at 0.
  EXPECT_EQ(0U, returned_map_[key2].byte_sent_count);
  EXPECT_EQ(0U, returned_map_[key2].byte_read_count);
  returned_map_.clear();

  correct_read_bytes1 = 0;
  correct_sent_bytes1 = 0;

  int correct_read_bytes2 = 0;
  int correct_sent_bytes2 = 0;

  int read_bytes_array_second_1[] = {4153, 987154, 946501};
  int sent_bytes_array_second_1[] = {13510, 115643, 954134};

  int read_bytes_array2[] = {9056243, 32243, 6541250};
  int sent_bytes_array2[] = {123238, 1527312, 96624};

  for (int i : read_bytes_array_second_1) {
    TaskManagerIoThreadHelper::OnRawBytesTransferred(key1, i, 0);
    correct_read_bytes1 += i;
  }
  for (int i : sent_bytes_array_second_1) {
    TaskManagerIoThreadHelper::OnRawBytesTransferred(key1, 0, i);
    correct_sent_bytes1 += i;
  }

  for (int i : read_bytes_array2) {
    TaskManagerIoThreadHelper::OnRawBytesTransferred(key2, i, 0);
    correct_read_bytes2 += i;
  }
  for (int i : sent_bytes_array2) {
    TaskManagerIoThreadHelper::OnRawBytesTransferred(key2, 0, i);
    correct_sent_bytes2 += i;
  }

  mock_main_runner->FastForwardBy(base::TimeDelta::FromSeconds(1));
  base::RunLoop().RunUntilIdle();
  EXPECT_EQ(2U, returned_map_.size());
  EXPECT_EQ(correct_sent_bytes1, returned_map_[key1].byte_sent_count);
  EXPECT_EQ(correct_read_bytes1, returned_map_[key1].byte_read_count);
  EXPECT_EQ(correct_sent_bytes2, returned_map_[key2].byte_sent_count);
  EXPECT_EQ(correct_read_bytes2, returned_map_[key2].byte_read_count);
}

}  // namespace task_manager
