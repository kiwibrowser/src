// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "base/bind.h"
#include "base/logging.h"
#include "base/memory/shared_memory.h"
#include "base/run_loop.h"
#include "content/public/test/test_browser_thread_bundle.h"
#include "storage/browser/blob/blob_data_builder.h"
#include "storage/browser/blob/blob_data_handle.h"
#include "storage/browser/blob/blob_storage_context.h"
#include "storage/browser/blob/blob_transport_host.h"
#include "storage/common/blob_storage/blob_storage_constants.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace content {
namespace {
const std::string kBlobUUID = "blobUUIDYAY";
const std::string kContentType = "content_type";
const std::string kContentDisposition = "content_disposition";
const std::string kCompletedBlobUUID = "completedBlob";
const std::string kCompletedBlobData = "completedBlobData";

const size_t kTestBlobStorageIPCThresholdBytes = 5;
const size_t kTestBlobStorageMaxSharedMemoryBytes = 20;

const size_t kTestBlobStorageMaxBlobMemorySize = 400;
const uint64_t kTestBlobStorageMaxDiskSpace = 4000;
const uint64_t kTestBlobStorageMinFileSizeBytes = 10;
const uint64_t kTestBlobStorageMaxFileSizeBytes = 100;

void PopulateBytes(char* bytes, size_t length) {
  for (size_t i = 0; i < length; i++) {
    bytes[i] = static_cast<char>(i);
  }
}

void AddMemoryItem(size_t length, std::vector<network::DataElement>* out) {
  network::DataElement bytes;
  bytes.SetToBytesDescription(length);
  out->push_back(std::move(bytes));
}

void AddShortcutMemoryItem(size_t length,
                           std::vector<network::DataElement>* out) {
  network::DataElement bytes;
  bytes.SetToAllocatedBytes(length);
  PopulateBytes(bytes.mutable_bytes(), length);
  out->push_back(std::move(bytes));
}

void AddShortcutMemoryItem(size_t length, storage::BlobDataBuilder* out) {
  network::DataElement bytes;
  bytes.SetToAllocatedBytes(length);
  PopulateBytes(bytes.mutable_bytes(), length);
  out->AppendData(bytes.bytes(), length);
}

void AddBlobItem(std::vector<network::DataElement>* out) {
  network::DataElement blob;
  blob.SetToBlob(kCompletedBlobUUID);
  out->push_back(std::move(blob));
}
}  // namespace

class BlobTransportHostTest : public testing::Test {
 public:
  BlobTransportHostTest()
      : status_code_(storage::BlobStatus::ERR_INVALID_CONSTRUCTION_ARGUMENTS),
        request_called_(false) {}
  ~BlobTransportHostTest() override {}

  void SetUp() override {
    status_code_ = storage::BlobStatus::ERR_INVALID_CONSTRUCTION_ARGUMENTS;
    request_called_ = false;
    requests_.clear();
    memory_handles_.clear();
    storage::BlobStorageLimits limits;
    limits.max_ipc_memory_size = kTestBlobStorageIPCThresholdBytes;
    limits.max_shared_memory_size = kTestBlobStorageMaxSharedMemoryBytes;
    limits.max_blob_in_memory_space = kTestBlobStorageMaxBlobMemorySize;
    limits.desired_max_disk_space = kTestBlobStorageMaxDiskSpace;
    limits.effective_max_disk_space = kTestBlobStorageMaxDiskSpace;
    limits.min_page_file_size = kTestBlobStorageMinFileSizeBytes;
    limits.max_file_size = kTestBlobStorageMaxFileSizeBytes;
    context_.mutable_memory_controller()->set_limits_for_testing(limits);
    storage::BlobDataBuilder builder(kCompletedBlobUUID);
    builder.AppendData(kCompletedBlobData);
    completed_blob_handle_ = context_.AddFinishedBlob(builder);
    EXPECT_EQ(storage::BlobStatus::DONE,
              completed_blob_handle_->GetBlobStatus());
  }

  void StatusCallback(storage::BlobStatus status) {
    status_called_ = true;
    status_code_ = status;
  }

  void RequestMemoryCallback(
      std::vector<storage::BlobItemBytesRequest> requests,
      std::vector<base::SharedMemoryHandle> shared_memory_handles,
      std::vector<base::File> files) {
    requests_ = std::move(requests);
    memory_handles_ = std::move(shared_memory_handles);
    request_called_ = true;
  }

  storage::BlobStatus BuildBlobAsync(
      const std::string& uuid,
      const std::vector<network::DataElement>& descriptions,
      std::unique_ptr<storage::BlobDataHandle>* storage) {
    EXPECT_NE(storage, nullptr);
    request_called_ = false;
    status_called_ = false;
    *storage = host_.StartBuildingBlob(
        uuid, kContentType, kContentDisposition, descriptions, &context_,
        nullptr,
        base::Bind(&BlobTransportHostTest::RequestMemoryCallback,
                   base::Unretained(this)),
        base::Bind(&BlobTransportHostTest::StatusCallback,
                   base::Unretained(this)));
    if (status_called_)
      return status_code_;
    else
      return context_.GetBlobStatus(uuid);
  }

  storage::BlobStatus GetBlobStatus(const std::string& uuid) const {
    return context_.GetBlobStatus(uuid);
  }

  bool IsBeingBuiltInContext(const std::string& uuid) const {
    return BlobStatusIsPending(context_.GetBlobStatus(uuid));
  }

  TestBrowserThreadBundle browser_thread_bundle_;
  storage::BlobStorageContext context_;
  storage::BlobTransportHost host_;
  bool status_called_;
  storage::BlobStatus status_code_;

  bool request_called_;
  std::vector<storage::BlobItemBytesRequest> requests_;
  std::vector<base::SharedMemoryHandle> memory_handles_;
  std::unique_ptr<storage::BlobDataHandle> completed_blob_handle_;
};

// The 'shortcut' method is when the data is included in the initial IPCs and
// the browser uses that instead of requesting the memory.
TEST_F(BlobTransportHostTest, TestShortcut) {
  std::vector<network::DataElement> descriptions;

  AddShortcutMemoryItem(10, &descriptions);
  AddBlobItem(&descriptions);
  AddShortcutMemoryItem(300, &descriptions);

  storage::BlobDataBuilder expected(kBlobUUID);
  expected.set_content_type(kContentType);
  expected.set_content_disposition(kContentDisposition);
  AddShortcutMemoryItem(10, &expected);
  expected.AppendData(kCompletedBlobData);
  AddShortcutMemoryItem(300, &expected);

  std::unique_ptr<storage::BlobDataHandle> handle;
  EXPECT_EQ(storage::BlobStatus::DONE,
            BuildBlobAsync(kBlobUUID, descriptions, &handle));

  EXPECT_FALSE(request_called_);
  EXPECT_EQ(0u, host_.blob_building_count());
  EXPECT_FALSE(handle->IsBeingBuilt());
  ASSERT_FALSE(handle->IsBroken());
  std::unique_ptr<storage::BlobDataSnapshot> data = handle->CreateSnapshot();
  EXPECT_EQ(expected, *data);
  data.reset();
  handle.reset();
  base::RunLoop().RunUntilIdle();
};

TEST_F(BlobTransportHostTest, TestShortcutNoRoom) {
  std::vector<network::DataElement> descriptions;

  AddShortcutMemoryItem(10, &descriptions);
  AddBlobItem(&descriptions);
  AddShortcutMemoryItem(5000, &descriptions);

  std::unique_ptr<storage::BlobDataHandle> handle;
  EXPECT_EQ(storage::BlobStatus::ERR_OUT_OF_MEMORY,
            BuildBlobAsync(kBlobUUID, descriptions, &handle));

  EXPECT_FALSE(request_called_);
  EXPECT_EQ(0u, host_.blob_building_count());
};

TEST_F(BlobTransportHostTest, TestSingleSharedMemRequest) {
  std::vector<network::DataElement> descriptions;
  const size_t kSize = kTestBlobStorageIPCThresholdBytes + 1;
  AddMemoryItem(kSize, &descriptions);

  std::unique_ptr<storage::BlobDataHandle> handle;
  EXPECT_EQ(storage::BlobStatus::PENDING_TRANSPORT,
            BuildBlobAsync(kBlobUUID, descriptions, &handle));
  EXPECT_TRUE(handle);
  EXPECT_EQ(storage::BlobStatus::PENDING_TRANSPORT, handle->GetBlobStatus());

  EXPECT_TRUE(request_called_);
  EXPECT_EQ(1u, host_.blob_building_count());
  ASSERT_EQ(1u, requests_.size());
  request_called_ = false;

  EXPECT_EQ(storage::BlobItemBytesRequest::CreateSharedMemoryRequest(
                0, 0, 0, kSize, 0, 0),
            requests_.at(0));
};

TEST_F(BlobTransportHostTest, TestMultipleSharedMemRequests) {
  std::vector<network::DataElement> descriptions;
  const size_t kSize = kTestBlobStorageMaxSharedMemoryBytes + 1;
  const char kFirstBlockByte = 7;
  const char kSecondBlockByte = 19;
  AddMemoryItem(kSize, &descriptions);

  storage::BlobDataBuilder expected(kBlobUUID);
  expected.set_content_type(kContentType);
  expected.set_content_disposition(kContentDisposition);
  char data[kSize];
  memset(data, kFirstBlockByte, kTestBlobStorageMaxSharedMemoryBytes);
  expected.AppendData(data, kTestBlobStorageMaxSharedMemoryBytes);
  expected.AppendData(&kSecondBlockByte, 1);

  std::unique_ptr<storage::BlobDataHandle> handle;
  EXPECT_EQ(storage::BlobStatus::PENDING_TRANSPORT,
            BuildBlobAsync(kBlobUUID, descriptions, &handle));

  EXPECT_TRUE(request_called_);
  EXPECT_EQ(1u, host_.blob_building_count());
  ASSERT_EQ(1u, requests_.size());
  request_called_ = false;

  // We need to grab a duplicate handle so we can have two blocks open at the
  // same time.
  base::SharedMemoryHandle shared_mem_handle =
      base::SharedMemory::DuplicateHandle(memory_handles_.at(0));
  EXPECT_TRUE(base::SharedMemory::IsHandleValid(shared_mem_handle));
  base::SharedMemory shared_memory(shared_mem_handle, false);
  EXPECT_TRUE(shared_memory.Map(kTestBlobStorageMaxSharedMemoryBytes));

  EXPECT_EQ(storage::BlobItemBytesRequest::CreateSharedMemoryRequest(
                0, 0, 0, kTestBlobStorageMaxSharedMemoryBytes, 0, 0),
            requests_.at(0));

  memset(shared_memory.memory(), kFirstBlockByte,
         kTestBlobStorageMaxSharedMemoryBytes);

  storage::BlobItemBytesResponse response(0);
  std::vector<storage::BlobItemBytesResponse> responses = {response};
  host_.OnMemoryResponses(kBlobUUID, responses, &context_);
  EXPECT_EQ(storage::BlobStatus::PENDING_TRANSPORT, GetBlobStatus(kBlobUUID));
  ASSERT_TRUE(handle);
  EXPECT_EQ(storage::BlobStatus::PENDING_TRANSPORT, handle->GetBlobStatus());

  EXPECT_TRUE(request_called_);
  EXPECT_EQ(1u, host_.blob_building_count());
  ASSERT_EQ(1u, requests_.size());
  request_called_ = false;

  EXPECT_EQ(storage::BlobItemBytesRequest::CreateSharedMemoryRequest(
                1, 0, kTestBlobStorageMaxSharedMemoryBytes, 1, 0, 0),
            requests_.at(0));

  memset(shared_memory.memory(), kSecondBlockByte, 1);

  response.request_number = 1;
  responses[0] = response;
  host_.OnMemoryResponses(kBlobUUID, responses, &context_);
  EXPECT_TRUE(handle);
  EXPECT_EQ(storage::BlobStatus::DONE, handle->GetBlobStatus());
  EXPECT_FALSE(request_called_);
  EXPECT_EQ(0u, host_.blob_building_count());
  std::unique_ptr<storage::BlobDataHandle> blob_handle =
      context_.GetBlobDataFromUUID(kBlobUUID);
  EXPECT_FALSE(blob_handle->IsBeingBuilt());
  EXPECT_FALSE(blob_handle->IsBroken());
  std::unique_ptr<storage::BlobDataSnapshot> blob_data =
      blob_handle->CreateSnapshot();
  EXPECT_EQ(expected, *blob_data);
};

TEST_F(BlobTransportHostTest, TestBasicIPCAndStopBuilding) {
  std::vector<network::DataElement> descriptions;

  AddMemoryItem(2, &descriptions);
  AddBlobItem(&descriptions);
  AddMemoryItem(2, &descriptions);

  storage::BlobDataBuilder expected(kBlobUUID);
  expected.set_content_type(kContentType);
  expected.set_content_disposition(kContentDisposition);
  AddShortcutMemoryItem(2, &expected);
  expected.AppendData(kCompletedBlobData);
  AddShortcutMemoryItem(2, &expected);

  std::unique_ptr<storage::BlobDataHandle> handle1;
  EXPECT_EQ(storage::BlobStatus::PENDING_TRANSPORT,
            BuildBlobAsync(kBlobUUID, descriptions, &handle1));
  EXPECT_TRUE(handle1);
  host_.CancelBuildingBlob(kBlobUUID, storage::BlobStatus::ERR_OUT_OF_MEMORY,
                           &context_);

  // Check that we're broken, and then remove the blob.
  EXPECT_FALSE(handle1->IsBeingBuilt());
  EXPECT_TRUE(handle1->IsBroken());
  handle1.reset();
  base::RunLoop().RunUntilIdle();
  handle1 = context_.GetBlobDataFromUUID(kBlobUUID);
  EXPECT_FALSE(handle1.get());

  // This should succeed because we've removed all references to the blob.
  std::unique_ptr<storage::BlobDataHandle> handle2;
  EXPECT_EQ(storage::BlobStatus::PENDING_TRANSPORT,
            BuildBlobAsync(kBlobUUID, descriptions, &handle2));

  EXPECT_TRUE(request_called_);
  EXPECT_EQ(1u, host_.blob_building_count());
  request_called_ = false;

  storage::BlobItemBytesResponse response1(0);
  PopulateBytes(response1.allocate_mutable_data(2), 2);
  storage::BlobItemBytesResponse response2(1);
  PopulateBytes(response2.allocate_mutable_data(2), 2);
  std::vector<storage::BlobItemBytesResponse> responses = {response1,
                                                           response2};

  host_.OnMemoryResponses(kBlobUUID, responses, &context_);
  EXPECT_EQ(storage::BlobStatus::DONE, handle2->GetBlobStatus());
  EXPECT_FALSE(request_called_);
  EXPECT_EQ(0u, host_.blob_building_count());
  EXPECT_FALSE(handle2->IsBeingBuilt());
  EXPECT_FALSE(handle2->IsBroken());
  std::unique_ptr<storage::BlobDataSnapshot> blob_data =
      handle2->CreateSnapshot();
  EXPECT_EQ(expected, *blob_data);
};

TEST_F(BlobTransportHostTest, TestBreakingAllBuilding) {
  const std::string& kBlob1 = "blob1";
  const std::string& kBlob2 = "blob2";
  const std::string& kBlob3 = "blob3";

  std::vector<network::DataElement> descriptions;
  AddMemoryItem(2, &descriptions);

  // Register blobs.
  std::unique_ptr<storage::BlobDataHandle> handle1;
  std::unique_ptr<storage::BlobDataHandle> handle2;
  std::unique_ptr<storage::BlobDataHandle> handle3;
  EXPECT_EQ(storage::BlobStatus::PENDING_TRANSPORT,
            BuildBlobAsync(kBlob1, descriptions, &handle1));
  EXPECT_EQ(storage::BlobStatus::PENDING_TRANSPORT,
            BuildBlobAsync(kBlob2, descriptions, &handle2));
  EXPECT_EQ(storage::BlobStatus::PENDING_TRANSPORT,
            BuildBlobAsync(kBlob3, descriptions, &handle3));

  EXPECT_TRUE(request_called_);
  EXPECT_TRUE(handle1->IsBeingBuilt() && handle2->IsBeingBuilt() &&
              handle3->IsBeingBuilt());
  EXPECT_FALSE(handle1->IsBroken() || handle2->IsBroken() ||
               handle3->IsBroken());

  EXPECT_TRUE(IsBeingBuiltInContext(kBlob1) && IsBeingBuiltInContext(kBlob2) &&
              IsBeingBuiltInContext(kBlob3));

  // This shouldn't call the transport complete callbacks, so our handles should
  // still be false.
  host_.CancelAll(&context_);

  EXPECT_FALSE(handle1->IsBeingBuilt() || handle2->IsBeingBuilt() ||
               handle3->IsBeingBuilt());
  EXPECT_TRUE(handle1->IsBroken() && handle2->IsBroken() &&
              handle3->IsBroken());

  base::RunLoop().RunUntilIdle();
};

TEST_F(BlobTransportHostTest, TestBadIPCs) {
  std::vector<network::DataElement> descriptions;

  // Test reusing same blob uuid.
  AddMemoryItem(10, &descriptions);
  AddBlobItem(&descriptions);
  AddMemoryItem(300, &descriptions);
  std::unique_ptr<storage::BlobDataHandle> handle1;
  EXPECT_EQ(storage::BlobStatus::PENDING_TRANSPORT,
            BuildBlobAsync(kBlobUUID, descriptions, &handle1));
  EXPECT_TRUE(host_.IsBeingBuilt(kBlobUUID));
  EXPECT_TRUE(request_called_);
  host_.CancelBuildingBlob(
      kBlobUUID, storage::BlobStatus::ERR_REFERENCED_BLOB_BROKEN, &context_);
  handle1.reset();
  EXPECT_FALSE(context_.GetBlobDataFromUUID(kBlobUUID).get());

  // Test empty responses.
  EXPECT_EQ(storage::BlobStatus::PENDING_TRANSPORT,
            BuildBlobAsync(kBlobUUID, descriptions, &handle1));
  std::vector<storage::BlobItemBytesResponse> responses;
  host_.OnMemoryResponses(kBlobUUID, responses, &context_);
  EXPECT_EQ(storage::BlobStatus::ERR_INVALID_CONSTRUCTION_ARGUMENTS,
            handle1->GetBlobStatus());
  handle1.reset();

  // Test response problems below here.
  descriptions.clear();
  AddMemoryItem(2, &descriptions);
  AddBlobItem(&descriptions);
  AddMemoryItem(2, &descriptions);
  EXPECT_EQ(storage::BlobStatus::PENDING_TRANSPORT,
            BuildBlobAsync(kBlobUUID, descriptions, &handle1));

  // Invalid request number.
  storage::BlobItemBytesResponse response1(3);
  PopulateBytes(response1.allocate_mutable_data(2), 2);
  responses = {response1};
  host_.OnMemoryResponses(kBlobUUID, responses, &context_);
  EXPECT_EQ(storage::BlobStatus::ERR_INVALID_CONSTRUCTION_ARGUMENTS,
            handle1->GetBlobStatus());
  EXPECT_TRUE(context_.GetBlobDataFromUUID(kBlobUUID)->IsBroken());
  handle1.reset();
  base::RunLoop().RunUntilIdle();

  // Duplicate request number responses.
  EXPECT_EQ(storage::BlobStatus::PENDING_TRANSPORT,
            BuildBlobAsync(kBlobUUID, descriptions, &handle1));
  response1.request_number = 0;
  storage::BlobItemBytesResponse response2(0);
  PopulateBytes(response2.allocate_mutable_data(2), 2);
  responses = {response1, response2};
  host_.OnMemoryResponses(kBlobUUID, responses, &context_);
  EXPECT_EQ(storage::BlobStatus::ERR_INVALID_CONSTRUCTION_ARGUMENTS,
            handle1->GetBlobStatus());
  EXPECT_TRUE(context_.GetBlobDataFromUUID(kBlobUUID)->IsBroken());
  handle1.reset();
  base::RunLoop().RunUntilIdle();
};

TEST_F(BlobTransportHostTest, WaitOnReferencedBlob) {
  const std::string& kBlob1 = "blob1";
  const std::string& kBlob2 = "blob2";
  const std::string& kBlob3 = "blob3";

  std::vector<network::DataElement> descriptions;
  AddMemoryItem(2, &descriptions);

  // Register blobs.
  std::unique_ptr<storage::BlobDataHandle> handle1;

  std::unique_ptr<storage::BlobDataHandle> handle2;
  std::unique_ptr<storage::BlobDataHandle> handle3;
  EXPECT_EQ(storage::BlobStatus::PENDING_TRANSPORT,
            BuildBlobAsync(kBlob1, descriptions, &handle1));
  EXPECT_EQ(storage::BlobStatus::PENDING_TRANSPORT,
            BuildBlobAsync(kBlob2, descriptions, &handle2));
  EXPECT_TRUE(request_called_);
  request_called_ = false;

  // Finish the third one, with a reference to the first and second blob.
  network::DataElement element;
  element.SetToBlob(kBlob1);
  descriptions.push_back(std::move(element));
  element.SetToBlob(kBlob2);
  descriptions.push_back(std::move(element));

  EXPECT_EQ(storage::BlobStatus::PENDING_TRANSPORT,
            BuildBlobAsync(kBlob3, descriptions, &handle3));
  EXPECT_TRUE(request_called_);
  request_called_ = false;

  // Finish the third, but we should still be 'building' it.
  storage::BlobItemBytesResponse response1(0);
  PopulateBytes(response1.allocate_mutable_data(2), 2);
  std::vector<storage::BlobItemBytesResponse> responses = {response1};
  host_.OnMemoryResponses(kBlob3, responses, &context_);
  EXPECT_EQ(storage::BlobStatus::PENDING_INTERNALS, handle3->GetBlobStatus());
  EXPECT_FALSE(request_called_);
  EXPECT_FALSE(host_.IsBeingBuilt(kBlob3));
  EXPECT_TRUE(IsBeingBuiltInContext(kBlob3));

  // Finish the first.
  descriptions.clear();
  AddShortcutMemoryItem(2, &descriptions);
  host_.OnMemoryResponses(kBlob1, responses, &context_);
  EXPECT_EQ(storage::BlobStatus::DONE, handle1->GetBlobStatus());
  EXPECT_FALSE(request_called_);
  EXPECT_FALSE(host_.IsBeingBuilt(kBlob1));
  EXPECT_FALSE(IsBeingBuiltInContext(kBlob1));
  EXPECT_TRUE(context_.GetBlobDataFromUUID(kBlob1));

  // Run the message loop so we propogate the construction complete callbacks.
  base::RunLoop().RunUntilIdle();
  // Verify we're not done.
  EXPECT_TRUE(IsBeingBuiltInContext(kBlob3));

  // Finish the second.
  host_.OnMemoryResponses(kBlob2, responses, &context_);
  EXPECT_EQ(storage::BlobStatus::DONE, handle2->GetBlobStatus());
  EXPECT_FALSE(request_called_);
  EXPECT_FALSE(host_.IsBeingBuilt(kBlob2));
  EXPECT_FALSE(IsBeingBuiltInContext(kBlob2));
  EXPECT_TRUE(context_.GetBlobDataFromUUID(kBlob2));

  // Run the message loop so we propogate the construction complete callbacks.
  base::RunLoop().RunUntilIdle();
  // Finally, we should be finished with third blob.
  EXPECT_FALSE(host_.IsBeingBuilt(kBlob3));
  EXPECT_FALSE(IsBeingBuiltInContext(kBlob3));
  EXPECT_TRUE(context_.GetBlobDataFromUUID(kBlob3));
};

}  // namespace content
