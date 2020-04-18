// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/loader/upload_data_stream_builder.h"

#include <stdint.h>

#include <limits>
#include <utility>
#include <vector>

#include "base/logging.h"
#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "base/single_thread_task_runner.h"
#include "net/base/elements_upload_data_stream.h"
#include "net/base/upload_bytes_element_reader.h"
#include "net/base/upload_file_element_reader.h"
#include "services/network/public/cpp/resource_request_body.h"
#include "storage/browser/blob/blob_data_handle.h"
#include "storage/browser/blob/blob_reader.h"
#include "storage/browser/blob/blob_storage_context.h"
#include "storage/browser/blob/upload_blob_element_reader.h"

namespace base {
class TaskRunner;
}

namespace disk_cache {
class Entry;
}

namespace content {
namespace {

// A subclass of net::UploadBytesElementReader which owns
// ResourceRequestBody.
class BytesElementReader : public net::UploadBytesElementReader {
 public:
  BytesElementReader(network::ResourceRequestBody* resource_request_body,
                     const network::DataElement& element)
      : net::UploadBytesElementReader(element.bytes(), element.length()),
        resource_request_body_(resource_request_body) {
    DCHECK_EQ(network::DataElement::TYPE_BYTES, element.type());
  }

  ~BytesElementReader() override {}

 private:
  scoped_refptr<network::ResourceRequestBody> resource_request_body_;

  DISALLOW_COPY_AND_ASSIGN(BytesElementReader);
};

// A subclass of net::UploadFileElementReader which owns
// ResourceRequestBody.
// This class is necessary to ensure the BlobData and any attached shareable
// files survive until upload completion.
class FileElementReader : public net::UploadFileElementReader {
 public:
  FileElementReader(network::ResourceRequestBody* resource_request_body,
                    base::TaskRunner* task_runner,
                    const network::DataElement& element)
      : net::UploadFileElementReader(task_runner,
                                     element.path(),
                                     element.offset(),
                                     element.length(),
                                     element.expected_modification_time()),
        resource_request_body_(resource_request_body) {
    DCHECK_EQ(network::DataElement::TYPE_FILE, element.type());
  }

  ~FileElementReader() override {}

 private:
  scoped_refptr<network::ResourceRequestBody> resource_request_body_;

  DISALLOW_COPY_AND_ASSIGN(FileElementReader);
};

}  // namespace

std::unique_ptr<net::UploadDataStream> UploadDataStreamBuilder::Build(
    network::ResourceRequestBody* body,
    storage::BlobStorageContext* blob_context,
    storage::FileSystemContext* file_system_context,
    base::SingleThreadTaskRunner* file_task_runner) {
  std::vector<std::unique_ptr<net::UploadElementReader>> element_readers;
  for (const auto& element : *body->elements()) {
    switch (element.type()) {
      case network::DataElement::TYPE_BYTES:
        element_readers.push_back(
            std::make_unique<BytesElementReader>(body, element));
        break;
      case network::DataElement::TYPE_FILE:
        element_readers.push_back(std::make_unique<FileElementReader>(
            body, file_task_runner, element));
        break;
      case network::DataElement::TYPE_BLOB: {
        DCHECK_EQ(0ul, element.offset());
        std::unique_ptr<storage::BlobDataHandle> handle =
            blob_context->GetBlobDataFromUUID(element.blob_uuid());
        DCHECK(element.length() == std::numeric_limits<uint64_t>::max() ||
               element.length() == handle->size());
        element_readers.push_back(
            std::make_unique<storage::UploadBlobElementReader>(
                std::move(handle)));
        break;
      }
      case network::DataElement::TYPE_RAW_FILE:
      case network::DataElement::TYPE_DATA_PIPE:
      case network::DataElement::TYPE_CHUNKED_DATA_PIPE:
      case network::DataElement::TYPE_UNKNOWN:
        NOTREACHED();
        break;
    }
  }

  return std::make_unique<net::ElementsUploadDataStream>(
      std::move(element_readers), body->identifier());
}

}  // namespace content
