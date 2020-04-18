// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/network/public/cpp/data_element.h"

#include <stddef.h>
#include <stdint.h>

#include <algorithm>

#include "base/strings/string_number_conversions.h"

namespace network {

const uint64_t DataElement::kUnknownSize;

DataElement::DataElement()
    : type_(TYPE_UNKNOWN),
      bytes_(NULL),
      offset_(0),
      length_(std::numeric_limits<uint64_t>::max()) {}

DataElement::~DataElement() = default;

DataElement::DataElement(DataElement&& other) = default;
DataElement& DataElement::operator=(DataElement&& other) = default;

void DataElement::SetToFilePathRange(
    const base::FilePath& path,
    uint64_t offset,
    uint64_t length,
    const base::Time& expected_modification_time) {
  type_ = TYPE_FILE;
  path_ = path;
  offset_ = offset;
  length_ = length;
  expected_modification_time_ = expected_modification_time;
}

void DataElement::SetToFileRange(base::File file,
                                 const base::FilePath& path,
                                 uint64_t offset,
                                 uint64_t length,
                                 const base::Time& expected_modification_time) {
  type_ = TYPE_RAW_FILE;
  file_ = std::move(file);
  path_ = path;
  offset_ = offset;
  length_ = length;
  expected_modification_time_ = expected_modification_time;
}

void DataElement::SetToBlobRange(const std::string& blob_uuid,
                                 uint64_t offset,
                                 uint64_t length) {
  type_ = TYPE_BLOB;
  blob_uuid_ = blob_uuid;
  offset_ = offset;
  length_ = length;
}

void DataElement::SetToDataPipe(mojom::DataPipeGetterPtr data_pipe_getter) {
  DCHECK(data_pipe_getter);
  type_ = TYPE_DATA_PIPE;
  data_pipe_getter_ = data_pipe_getter.PassInterface();
}

void DataElement::SetToChunkedDataPipe(
    mojom::ChunkedDataPipeGetterPtr chunked_data_pipe_getter) {
  type_ = TYPE_CHUNKED_DATA_PIPE;
  chunked_data_pipe_getter_ = std::move(chunked_data_pipe_getter);
}

base::File DataElement::ReleaseFile() {
  return std::move(file_);
}

mojom::DataPipeGetterPtrInfo DataElement::ReleaseDataPipeGetter() {
  DCHECK_EQ(TYPE_DATA_PIPE, type_);
  DCHECK(data_pipe_getter_.is_valid());
  return std::move(data_pipe_getter_);
}

mojom::DataPipeGetterPtr DataElement::CloneDataPipeGetter() const {
  DCHECK_EQ(TYPE_DATA_PIPE, type_);
  DCHECK(data_pipe_getter_.is_valid());
  auto* mutable_this = const_cast<DataElement*>(this);
  mojom::DataPipeGetterPtr owned(std::move(mutable_this->data_pipe_getter_));
  mojom::DataPipeGetterPtr clone;
  owned->Clone(MakeRequest(&clone));
  mutable_this->data_pipe_getter_ = owned.PassInterface();
  return clone;
}

mojom::ChunkedDataPipeGetterPtr DataElement::ReleaseChunkedDataPipeGetter() {
  DCHECK_EQ(TYPE_CHUNKED_DATA_PIPE, type_);
  return std::move(chunked_data_pipe_getter_);
}

void PrintTo(const DataElement& x, std::ostream* os) {
  const uint64_t kMaxDataPrintLength = 40;
  *os << "<DataElement>{type: ";
  switch (x.type()) {
    case DataElement::TYPE_BYTES: {
      uint64_t length = std::min(x.length(), kMaxDataPrintLength);
      *os << "TYPE_BYTES, data: ["
          << base::HexEncode(x.bytes(), static_cast<size_t>(length));
      if (length < x.length()) {
        *os << "<...truncated due to length...>";
      }
      *os << "]";
      break;
    }
    case DataElement::TYPE_FILE:
      *os << "TYPE_FILE, path: " << x.path().AsUTF8Unsafe()
          << ", expected_modification_time: " << x.expected_modification_time();
      break;
    case DataElement::TYPE_RAW_FILE:
      *os << "TYPE_RAW_FILE, path: " << x.path().AsUTF8Unsafe()
          << ", expected_modification_time: " << x.expected_modification_time();
      break;
    case DataElement::TYPE_BLOB:
      *os << "TYPE_BLOB, uuid: " << x.blob_uuid();
      break;
    case DataElement::TYPE_DATA_PIPE:
      *os << "TYPE_DATA_PIPE";
      break;
    case DataElement::TYPE_CHUNKED_DATA_PIPE:
      *os << "TYPE_CHUNKED_DATA_PIPE";
      break;
    case DataElement::TYPE_UNKNOWN:
      *os << "TYPE_UNKNOWN";
      break;
  }
  *os << ", length: " << x.length() << ", offset: " << x.offset() << "}";
}

bool operator==(const DataElement& a, const DataElement& b) {
  if (a.type() != b.type() || a.offset() != b.offset() ||
      a.length() != b.length())
    return false;
  switch (a.type()) {
    case DataElement::TYPE_BYTES:
      return memcmp(a.bytes(), b.bytes(), b.length()) == 0;
    case DataElement::TYPE_FILE:
      return a.path() == b.path() &&
             a.expected_modification_time() == b.expected_modification_time();
    case DataElement::TYPE_RAW_FILE:
      return a.path() == b.path() &&
             a.expected_modification_time() == b.expected_modification_time();
    case DataElement::TYPE_BLOB:
      return a.blob_uuid() == b.blob_uuid();
    case DataElement::TYPE_DATA_PIPE:
      return false;
    case DataElement::TYPE_CHUNKED_DATA_PIPE:
      return false;
    case DataElement::TYPE_UNKNOWN:
      NOTREACHED();
      return false;
  }
  return false;
}

bool operator!=(const DataElement& a, const DataElement& b) {
  return !(a == b);
}

}  // namespace network
