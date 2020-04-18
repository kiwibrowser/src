// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/network/public/cpp/resource_request_body.h"

namespace network {

ResourceRequestBody::ResourceRequestBody()
    : identifier_(0), contains_sensitive_info_(false) {}

// static
scoped_refptr<ResourceRequestBody> ResourceRequestBody::CreateFromBytes(
    const char* bytes,
    size_t length) {
  scoped_refptr<ResourceRequestBody> result = new ResourceRequestBody();
  result->AppendBytes(bytes, length);
  return result;
}

void ResourceRequestBody::AppendBytes(const char* bytes, int bytes_len) {
  DCHECK(elements_.empty() ||
         elements_.front().type() != DataElement::TYPE_CHUNKED_DATA_PIPE);

  if (bytes_len > 0) {
    elements_.push_back(DataElement());
    elements_.back().SetToBytes(bytes, bytes_len);
  }
}

void ResourceRequestBody::AppendFileRange(
    const base::FilePath& file_path,
    uint64_t offset,
    uint64_t length,
    const base::Time& expected_modification_time) {
  DCHECK(elements_.empty() ||
         elements_.front().type() != DataElement::TYPE_CHUNKED_DATA_PIPE);

  elements_.push_back(DataElement());
  elements_.back().SetToFilePathRange(file_path, offset, length,
                                      expected_modification_time);
}

void ResourceRequestBody::AppendRawFileRange(
    base::File file,
    const base::FilePath& file_path,
    uint64_t offset,
    uint64_t length,
    const base::Time& expected_modification_time) {
  DCHECK(elements_.empty() ||
         elements_.front().type() != DataElement::TYPE_CHUNKED_DATA_PIPE);

  elements_.push_back(DataElement());
  elements_.back().SetToFileRange(std::move(file), file_path, offset, length,
                                  expected_modification_time);
}

void ResourceRequestBody::AppendBlob(const std::string& uuid) {
  AppendBlob(uuid, std::numeric_limits<uint64_t>::max());
}

void ResourceRequestBody::AppendBlob(const std::string& uuid, uint64_t length) {
  DCHECK(elements_.empty() ||
         elements_.front().type() != DataElement::TYPE_CHUNKED_DATA_PIPE);

  elements_.push_back(DataElement());
  elements_.back().SetToBlobRange(uuid, 0 /* offset */, length);
}

void ResourceRequestBody::AppendDataPipe(
    mojom::DataPipeGetterPtr data_pipe_getter) {
  DCHECK(elements_.empty() ||
         elements_.front().type() != DataElement::TYPE_CHUNKED_DATA_PIPE);

  elements_.push_back(DataElement());
  elements_.back().SetToDataPipe(std::move(data_pipe_getter));
}

void ResourceRequestBody::SetToChunkedDataPipe(
    mojom::ChunkedDataPipeGetterPtr chunked_data_pipe_getter) {
  DCHECK(elements_.empty());

  elements_.push_back(DataElement());
  elements_.back().SetToChunkedDataPipe(std::move(chunked_data_pipe_getter));
}

std::vector<base::FilePath> ResourceRequestBody::GetReferencedFiles() const {
  std::vector<base::FilePath> result;
  for (const auto& element : *elements()) {
    if (element.type() == DataElement::TYPE_FILE)
      result.push_back(element.path());
  }
  return result;
}

ResourceRequestBody::~ResourceRequestBody() {}

}  // namespace network
