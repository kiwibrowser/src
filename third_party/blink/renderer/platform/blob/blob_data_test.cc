// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/platform/blob/blob_data.h"

#include <memory>
#include <utility>
#include "base/run_loop.h"
#include "base/test/scoped_task_environment.h"
#include "mojo/public/cpp/bindings/binding_set.h"
#include "mojo/public/cpp/bindings/strong_binding.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/blink/public/mojom/blob/blob_registry.mojom-blink.h"
#include "third_party/blink/public/platform/file_path_conversion.h"
#include "third_party/blink/public/platform/interface_provider.h"
#include "third_party/blink/renderer/platform/blob/blob_bytes_provider.h"
#include "third_party/blink/renderer/platform/blob/testing/fake_blob_registry.h"
#include "third_party/blink/renderer/platform/testing/testing_platform_support.h"
#include "third_party/blink/renderer/platform/uuid.h"

namespace blink {

using mojom::blink::Blob;
using mojom::blink::BlobPtr;
using mojom::blink::BlobRegistry;
using mojom::blink::BlobRegistryPtr;
using mojom::blink::BlobRegistryRequest;
using mojom::blink::BlobRequest;
using mojom::blink::DataElement;
using mojom::blink::DataElementBlob;
using mojom::blink::DataElementBytes;
using mojom::blink::DataElementFile;
using mojom::blink::DataElementFilesystemURL;
using mojom::blink::DataElementPtr;

namespace {

struct ExpectedElement {
  DataElementPtr element;
  String blob_uuid;
  Vector<uint8_t> large_data;

  static ExpectedElement EmbeddedBytes(Vector<uint8_t> embedded_data) {
    uint64_t size = embedded_data.size();
    return ExpectedElement{DataElement::NewBytes(
        DataElementBytes::New(size, std::move(embedded_data), nullptr))};
  }

  static ExpectedElement LargeBytes(Vector<uint8_t> data) {
    uint64_t size = data.size();
    return ExpectedElement{DataElement::NewBytes(DataElementBytes::New(
                               size, base::nullopt, nullptr)),
                           String(), std::move(data)};
  }

  static ExpectedElement File(const String& path,
                              uint64_t offset,
                              uint64_t length,
                              WTF::Time time) {
    return ExpectedElement{DataElement::NewFile(
        DataElementFile::New(WebStringToFilePath(path), offset, length, time))};
  }

  static ExpectedElement FileFilesystem(const KURL& url,
                                        uint64_t offset,
                                        uint64_t length,
                                        WTF::Time time) {
    return ExpectedElement{DataElement::NewFileFilesystem(
        DataElementFilesystemURL::New(url, offset, length, time))};
  }

  static ExpectedElement Blob(const String& uuid,
                              uint64_t offset,
                              uint64_t length) {
    return ExpectedElement{
        DataElement::NewBlob(DataElementBlob::New(nullptr, offset, length)),
        uuid};
  }
};

}  // namespace

class BlobDataHandleTest : public testing::Test {
 public:
  BlobDataHandleTest() : blob_registry_binding_(&mock_blob_registry_) {
    blob_registry_binding_.Bind(MakeRequest(&blob_registry_ptr_));
    BlobDataHandle::SetBlobRegistryForTesting(blob_registry_ptr_.get());
  }

  ~BlobDataHandleTest() override {
    BlobDataHandle::SetBlobRegistryForTesting(nullptr);
  }

  void SetUp() override {
    small_test_data_.resize(1024);
    medium_test_data_.resize(1024 * 32);
    large_test_data_.resize(1024 * 512);
    for (size_t i = 0; i < small_test_data_.size(); ++i)
      small_test_data_[i] = i;
    for (size_t i = 0; i < medium_test_data_.size(); ++i)
      medium_test_data_[i] = i % 191;
    for (size_t i = 0; i < large_test_data_.size(); ++i)
      large_test_data_[i] = i % 251;

    ASSERT_LT(small_test_data_.size(),
              BlobBytesProvider::kMaxConsolidatedItemSizeInBytes);
    ASSERT_LT(medium_test_data_.size(),
              DataElementBytes::kMaximumEmbeddedDataSize);
    ASSERT_GT(medium_test_data_.size(),
              BlobBytesProvider::kMaxConsolidatedItemSizeInBytes);
    ASSERT_GT(large_test_data_.size(),
              DataElementBytes::kMaximumEmbeddedDataSize);

    empty_blob_ = BlobDataHandle::Create();

    std::unique_ptr<BlobData> test_data = BlobData::Create();
    test_data->AppendBytes(large_test_data_.data(), large_test_data_.size());
    test_blob_ =
        BlobDataHandle::Create(std::move(test_data), large_test_data_.size());

    blob_registry_ptr_.FlushForTesting();
    ASSERT_EQ(2u, mock_blob_registry_.registrations.size());
    empty_blob_uuid_ = mock_blob_registry_.registrations[0].uuid;
    test_blob_uuid_ = mock_blob_registry_.registrations[1].uuid;
    mock_blob_registry_.registrations.clear();
  }

  void TestCreateBlob(std::unique_ptr<BlobData> data,
                      Vector<ExpectedElement> expected_elements) {
    size_t blob_size = data->length();
    String type = data->ContentType();
    bool is_single_unknown_size_file = data->IsSingleUnknownSizeFile();

    scoped_refptr<BlobDataHandle> handle =
        BlobDataHandle::Create(std::move(data), blob_size);
    EXPECT_EQ(blob_size, handle->size());
    EXPECT_EQ(type, handle->GetType());
    EXPECT_EQ(is_single_unknown_size_file, handle->IsSingleUnknownSizeFile());

    blob_registry_ptr_.FlushForTesting();
    EXPECT_EQ(0u, mock_blob_registry_.binding_requests.size());
    ASSERT_EQ(1u, mock_blob_registry_.registrations.size());
    auto& reg = mock_blob_registry_.registrations[0];
    EXPECT_EQ(handle->Uuid(), reg.uuid);
    EXPECT_EQ(type.IsNull() ? "" : type, reg.content_type);
    EXPECT_EQ("", reg.content_disposition);
    ASSERT_EQ(expected_elements.size(), reg.elements.size());
    for (size_t i = 0; i < expected_elements.size(); ++i) {
      const auto& expected = expected_elements[i].element;
      auto& actual = reg.elements[i];
      if (expected->is_bytes()) {
        ASSERT_TRUE(actual->is_bytes());
        EXPECT_EQ(expected->get_bytes()->length, actual->get_bytes()->length);
        EXPECT_EQ(expected->get_bytes()->embedded_data,
                  actual->get_bytes()->embedded_data);

        base::RunLoop loop;
        Vector<uint8_t> received_bytes;
        mojom::blink::BytesProviderPtr data(
            std::move(actual->get_bytes()->data));
        data->RequestAsReply(base::BindOnce(
            [](base::Closure quit_closure, Vector<uint8_t>* bytes_out,
               const Vector<uint8_t>& bytes) {
              *bytes_out = bytes;
              quit_closure.Run();
            },
            loop.QuitClosure(), &received_bytes));
        loop.Run();
        if (expected->get_bytes()->embedded_data)
          EXPECT_EQ(expected->get_bytes()->embedded_data, received_bytes);
        else
          EXPECT_EQ(expected_elements[i].large_data, received_bytes);
      } else if (expected->is_file()) {
        ASSERT_TRUE(actual->is_file());
        EXPECT_EQ(expected->get_file()->path, actual->get_file()->path);
        EXPECT_EQ(expected->get_file()->length, actual->get_file()->length);
        EXPECT_EQ(expected->get_file()->offset, actual->get_file()->offset);
        EXPECT_EQ(expected->get_file()->expected_modification_time,
                  actual->get_file()->expected_modification_time);
      } else if (expected->is_file_filesystem()) {
        ASSERT_TRUE(actual->is_file_filesystem());
        EXPECT_EQ(expected->get_file_filesystem()->url,
                  actual->get_file_filesystem()->url);
        EXPECT_EQ(expected->get_file_filesystem()->length,
                  actual->get_file_filesystem()->length);
        EXPECT_EQ(expected->get_file_filesystem()->offset,
                  actual->get_file_filesystem()->offset);
        EXPECT_EQ(expected->get_file_filesystem()->expected_modification_time,
                  actual->get_file_filesystem()->expected_modification_time);
      } else if (expected->is_blob()) {
        ASSERT_TRUE(actual->is_blob());
        EXPECT_EQ(expected->get_blob()->length, actual->get_blob()->length);
        EXPECT_EQ(expected->get_blob()->offset, actual->get_blob()->offset);

        base::RunLoop loop;
        String received_uuid;
        mojom::blink::BlobPtr blob(std::move(actual->get_blob()->blob));
        blob->GetInternalUUID(base::BindOnce(
            [](base::Closure quit_closure, String* uuid_out,
               const String& uuid) {
              *uuid_out = uuid;
              quit_closure.Run();
            },
            loop.QuitClosure(), &received_uuid));
        loop.Run();
        EXPECT_EQ(expected_elements[i].blob_uuid, received_uuid);
      }
    }
    mock_blob_registry_.registrations.clear();
  }

 protected:
  base::test::ScopedTaskEnvironment scoped_task_environment_;
  FakeBlobRegistry mock_blob_registry_;
  BlobRegistryPtr blob_registry_ptr_;
  mojo::Binding<BlobRegistry> blob_registry_binding_;

  // Significantly less than BlobData's kMaxConsolidatedItemSizeInBytes.
  Vector<uint8_t> small_test_data_;
  // Larger than kMaxConsolidatedItemSizeInBytes, but smaller than
  // max_data_population.
  Vector<uint8_t> medium_test_data_;
  // Larger than max_data_population.
  Vector<uint8_t> large_test_data_;
  scoped_refptr<BlobDataHandle> empty_blob_;
  String empty_blob_uuid_;
  scoped_refptr<BlobDataHandle> test_blob_;
  String test_blob_uuid_;
};

TEST_F(BlobDataHandleTest, CreateEmpty) {
  scoped_refptr<BlobDataHandle> handle = BlobDataHandle::Create();
  EXPECT_TRUE(handle->GetType().IsNull());
  EXPECT_EQ(0u, handle->size());
  EXPECT_FALSE(handle->IsSingleUnknownSizeFile());

  blob_registry_ptr_.FlushForTesting();
  EXPECT_EQ(0u, mock_blob_registry_.binding_requests.size());
  ASSERT_EQ(1u, mock_blob_registry_.registrations.size());
  const auto& reg = mock_blob_registry_.registrations[0];
  EXPECT_EQ(handle->Uuid(), reg.uuid);
  EXPECT_EQ("", reg.content_type);
  EXPECT_EQ("", reg.content_disposition);
  EXPECT_EQ(0u, reg.elements.size());
}

TEST_F(BlobDataHandleTest, CreateFromEmptyData) {
  String kType = "content/type";

  std::unique_ptr<BlobData> data = BlobData::Create();
  data->SetContentType(kType);

  TestCreateBlob(std::move(data), {});
}

TEST_F(BlobDataHandleTest, CreateFromUUID) {
  String kUuid = CreateCanonicalUUIDString();
  String kType = "content/type";
  uint64_t kSize = 1234;

  scoped_refptr<BlobDataHandle> handle =
      BlobDataHandle::Create(kUuid, kType, kSize);
  EXPECT_EQ(kUuid, handle->Uuid());
  EXPECT_EQ(kType, handle->GetType());
  EXPECT_EQ(kSize, handle->size());
  EXPECT_FALSE(handle->IsSingleUnknownSizeFile());

  blob_registry_ptr_.FlushForTesting();
  EXPECT_EQ(0u, mock_blob_registry_.registrations.size());
  ASSERT_EQ(1u, mock_blob_registry_.binding_requests.size());
  EXPECT_EQ(kUuid, mock_blob_registry_.binding_requests[0].uuid);
}

TEST_F(BlobDataHandleTest, CreateFromEmptyElements) {
  std::unique_ptr<BlobData> data = BlobData::Create();
  data->AppendBytes(small_test_data_.data(), 0);
  data->AppendBlob(empty_blob_, 0, 0);
  data->AppendFile("path", 0, 0, 0.0);
  data->AppendFileSystemURL(NullURL(), 0, 0, 0.0);

  TestCreateBlob(std::move(data), {});
}

TEST_F(BlobDataHandleTest, CreateFromSmallBytes) {
  std::unique_ptr<BlobData> data = BlobData::Create();
  data->AppendBytes(small_test_data_.data(), small_test_data_.size());

  Vector<ExpectedElement> expected_elements;
  expected_elements.push_back(ExpectedElement::EmbeddedBytes(small_test_data_));

  TestCreateBlob(std::move(data), std::move(expected_elements));
}

TEST_F(BlobDataHandleTest, CreateFromLargeBytes) {
  std::unique_ptr<BlobData> data = BlobData::Create();
  data->AppendBytes(large_test_data_.data(), large_test_data_.size());

  Vector<ExpectedElement> expected_elements;
  expected_elements.push_back(ExpectedElement::LargeBytes(large_test_data_));

  TestCreateBlob(std::move(data), std::move(expected_elements));
}

TEST_F(BlobDataHandleTest, CreateFromMergedBytes) {
  std::unique_ptr<BlobData> data = BlobData::Create();
  data->AppendBytes(medium_test_data_.data(), medium_test_data_.size());
  data->AppendBytes(small_test_data_.data(), small_test_data_.size());
  EXPECT_EQ(1u, data->Elements().size());

  Vector<uint8_t> expected_data = medium_test_data_;
  expected_data.AppendVector(small_test_data_);

  Vector<ExpectedElement> expected_elements;
  expected_elements.push_back(
      ExpectedElement::EmbeddedBytes(std::move(expected_data)));

  TestCreateBlob(std::move(data), std::move(expected_elements));
}

TEST_F(BlobDataHandleTest, CreateFromMergedLargeAndSmallBytes) {
  std::unique_ptr<BlobData> data = BlobData::Create();
  data->AppendBytes(large_test_data_.data(), large_test_data_.size());
  data->AppendBytes(small_test_data_.data(), small_test_data_.size());
  EXPECT_EQ(1u, data->Elements().size());

  Vector<uint8_t> expected_data = large_test_data_;
  expected_data.AppendVector(small_test_data_);

  Vector<ExpectedElement> expected_elements;
  expected_elements.push_back(
      ExpectedElement::LargeBytes(std::move(expected_data)));

  TestCreateBlob(std::move(data), std::move(expected_elements));
}

TEST_F(BlobDataHandleTest, CreateFromMergedSmallAndLargeBytes) {
  std::unique_ptr<BlobData> data = BlobData::Create();
  data->AppendBytes(small_test_data_.data(), small_test_data_.size());
  data->AppendBytes(large_test_data_.data(), large_test_data_.size());
  EXPECT_EQ(1u, data->Elements().size());

  Vector<uint8_t> expected_data = small_test_data_;
  expected_data.AppendVector(large_test_data_);

  Vector<ExpectedElement> expected_elements;
  expected_elements.push_back(
      ExpectedElement::LargeBytes(std::move(expected_data)));

  TestCreateBlob(std::move(data), std::move(expected_elements));
}

TEST_F(BlobDataHandleTest, CreateFromFileAndFileSystemURL) {
  double timestamp1 = CurrentTime();
  double timestamp2 = timestamp1 + 1;
  KURL url(NullURL(), "http://example.com/");
  std::unique_ptr<BlobData> data = BlobData::Create();
  data->AppendFile("path", 4, 32, timestamp1);
  data->AppendFileSystemURL(url, 15, 876, timestamp2);

  Vector<ExpectedElement> expected_elements;
  expected_elements.push_back(
      ExpectedElement::File("path", 4, 32, WTF::Time::FromDoubleT(timestamp1)));
  expected_elements.push_back(ExpectedElement::FileFilesystem(
      url, 15, 876, WTF::Time::FromDoubleT(timestamp2)));

  TestCreateBlob(std::move(data), std::move(expected_elements));
}

TEST_F(BlobDataHandleTest, CreateFromFileWithUnknownSize) {
  Vector<ExpectedElement> expected_elements;
  expected_elements.push_back(
      ExpectedElement::File("path", 0, uint64_t(-1), WTF::Time()));

  TestCreateBlob(BlobData::CreateForFileWithUnknownSize("path"),
                 std::move(expected_elements));
}

TEST_F(BlobDataHandleTest, CreateFromFilesystemFileWithUnknownSize) {
  double timestamp = CurrentTime();
  KURL url(NullURL(), "http://example.com/");
  Vector<ExpectedElement> expected_elements;
  expected_elements.push_back(ExpectedElement::FileFilesystem(
      url, 0, uint64_t(-1), WTF::Time::FromDoubleT(timestamp)));

  TestCreateBlob(
      BlobData::CreateForFileSystemURLWithUnknownSize(url, timestamp),
      std::move(expected_elements));
}

TEST_F(BlobDataHandleTest, CreateFromBlob) {
  std::unique_ptr<BlobData> data = BlobData::Create();
  data->AppendBlob(test_blob_, 13, 765);

  Vector<ExpectedElement> expected_elements;
  expected_elements.push_back(ExpectedElement::Blob(test_blob_uuid_, 13, 765));

  TestCreateBlob(std::move(data), std::move(expected_elements));
}

TEST_F(BlobDataHandleTest, CreateFromBlobsAndBytes) {
  std::unique_ptr<BlobData> data = BlobData::Create();
  data->AppendBlob(test_blob_, 10, 10);
  data->AppendBytes(medium_test_data_.data(), medium_test_data_.size());
  data->AppendBlob(test_blob_, 0, 0);
  data->AppendBytes(small_test_data_.data(), small_test_data_.size());
  data->AppendBlob(test_blob_, 0, 10);
  data->AppendBytes(large_test_data_.data(), large_test_data_.size());

  Vector<uint8_t> expected_data = medium_test_data_;
  expected_data.AppendVector(small_test_data_);

  Vector<ExpectedElement> expected_elements;
  expected_elements.push_back(ExpectedElement::Blob(test_blob_uuid_, 10, 10));
  expected_elements.push_back(
      ExpectedElement::EmbeddedBytes(std::move(expected_data)));
  expected_elements.push_back(ExpectedElement::Blob(test_blob_uuid_, 0, 10));
  expected_elements.push_back(ExpectedElement::LargeBytes(large_test_data_));

  TestCreateBlob(std::move(data), std::move(expected_elements));
}

TEST_F(BlobDataHandleTest, CreateFromSmallBytesAfterLargeBytes) {
  std::unique_ptr<BlobData> data = BlobData::Create();
  data->AppendBytes(large_test_data_.data(), large_test_data_.size());
  data->AppendBlob(test_blob_, 0, 10);
  data->AppendBytes(small_test_data_.data(), small_test_data_.size());

  Vector<ExpectedElement> expected_elements;
  expected_elements.push_back(ExpectedElement::LargeBytes(large_test_data_));
  expected_elements.push_back(ExpectedElement::Blob(test_blob_uuid_, 0, 10));
  expected_elements.push_back(ExpectedElement::EmbeddedBytes(small_test_data_));

  TestCreateBlob(std::move(data), std::move(expected_elements));
}

TEST_F(BlobDataHandleTest, CreateFromManyMergedBytes) {
  std::unique_ptr<BlobData> data = BlobData::Create();
  Vector<uint8_t> merged_data;
  while (merged_data.size() <= DataElementBytes::kMaximumEmbeddedDataSize) {
    data->AppendBytes(medium_test_data_.data(), medium_test_data_.size());
    merged_data.AppendVector(medium_test_data_);
  }
  data->AppendBlob(test_blob_, 0, 10);
  data->AppendBytes(medium_test_data_.data(), medium_test_data_.size());

  Vector<ExpectedElement> expected_elements;
  expected_elements.push_back(
      ExpectedElement::LargeBytes(std::move(merged_data)));
  expected_elements.push_back(ExpectedElement::Blob(test_blob_uuid_, 0, 10));
  expected_elements.push_back(
      ExpectedElement::EmbeddedBytes(medium_test_data_));

  TestCreateBlob(std::move(data), std::move(expected_elements));
}

}  // namespace blink
