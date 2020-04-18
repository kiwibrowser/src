// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "mojo/public/cpp/base/big_buffer.h"

#include "base/logging.h"

namespace mojo_base {

namespace internal {

BigBufferSharedMemoryRegion::BigBufferSharedMemoryRegion() = default;

BigBufferSharedMemoryRegion::BigBufferSharedMemoryRegion(
    mojo::ScopedSharedBufferHandle buffer_handle,
    size_t size)
    : size_(size),
      buffer_handle_(std::move(buffer_handle)),
      buffer_mapping_(buffer_handle_->Map(size)) {}

BigBufferSharedMemoryRegion::BigBufferSharedMemoryRegion(
    BigBufferSharedMemoryRegion&& other) = default;

BigBufferSharedMemoryRegion::~BigBufferSharedMemoryRegion() = default;

BigBufferSharedMemoryRegion& BigBufferSharedMemoryRegion::operator=(
    BigBufferSharedMemoryRegion&& other) = default;

mojo::ScopedSharedBufferHandle BigBufferSharedMemoryRegion::TakeBufferHandle() {
  DCHECK(buffer_handle_.is_valid());
  buffer_mapping_.reset();
  return std::move(buffer_handle_);
}

}  // namespace internal

// static
constexpr size_t BigBuffer::kMaxInlineBytes;

BigBuffer::BigBuffer() : storage_type_(StorageType::kBytes) {}

BigBuffer::BigBuffer(BigBuffer&& other) = default;

BigBuffer::BigBuffer(base::span<const uint8_t> data) {
  *this = BigBufferView::ToBigBuffer(BigBufferView(data));
}

BigBuffer::BigBuffer(const std::vector<uint8_t>& data)
    : BigBuffer(base::make_span(data)) {}

BigBuffer::BigBuffer(internal::BigBufferSharedMemoryRegion shared_memory)
    : storage_type_(StorageType::kSharedMemory),
      shared_memory_(std::move(shared_memory)) {}

BigBuffer::~BigBuffer() = default;

BigBuffer& BigBuffer::operator=(BigBuffer&& other) = default;

uint8_t* BigBuffer::data() {
  return const_cast<uint8_t*>(const_cast<const BigBuffer*>(this)->data());
}

const uint8_t* BigBuffer::data() const {
  switch (storage_type_) {
    case StorageType::kBytes:
      return bytes_.data();
    case StorageType::kSharedMemory:
      DCHECK(shared_memory_->buffer_mapping_);
      return static_cast<const uint8_t*>(
          const_cast<const void*>(shared_memory_->buffer_mapping_.get()));
    default:
      NOTREACHED();
      return nullptr;
  }
}

size_t BigBuffer::size() const {
  switch (storage_type_) {
    case StorageType::kBytes:
      return bytes_.size();
    case StorageType::kSharedMemory:
      return shared_memory_->size();
    default:
      NOTREACHED();
      return 0;
  }
}

BigBufferView::BigBufferView() = default;

BigBufferView::BigBufferView(BigBufferView&& other) = default;

BigBufferView::BigBufferView(base::span<const uint8_t> bytes) {
  if (bytes.size() > BigBuffer::kMaxInlineBytes) {
    auto buffer = mojo::SharedBufferHandle::Create(bytes.size());
    if (buffer.is_valid()) {
      storage_type_ = BigBuffer::StorageType::kSharedMemory;
      shared_memory_.emplace(std::move(buffer), bytes.size());
      std::copy(bytes.begin(), bytes.end(),
                static_cast<uint8_t*>(shared_memory_->buffer_mapping_.get()));
      return;
    }
  }

  // Either the data is small enough or shared memory allocation failed. Either
  // way we fall back to directly referencing the input bytes.
  storage_type_ = BigBuffer::StorageType::kBytes;
  bytes_ = bytes;
}

BigBufferView::~BigBufferView() = default;

BigBufferView& BigBufferView::operator=(BigBufferView&& other) = default;

void BigBufferView::SetBytes(base::span<const uint8_t> bytes) {
  DCHECK(bytes_.empty());
  DCHECK(!shared_memory_);
  storage_type_ = BigBuffer::StorageType::kBytes;
  bytes_ = bytes;
}

void BigBufferView::SetSharedMemory(
    internal::BigBufferSharedMemoryRegion shared_memory) {
  DCHECK(bytes_.empty());
  DCHECK(!shared_memory_);
  storage_type_ = BigBuffer::StorageType::kSharedMemory;
  shared_memory_ = std::move(shared_memory);
}

base::span<const uint8_t> BigBufferView::data() const {
  if (storage_type_ == BigBuffer::StorageType::kBytes) {
    return bytes_;
  } else {
    DCHECK(shared_memory_.has_value());
    return base::make_span(static_cast<const uint8_t*>(const_cast<const void*>(
                               shared_memory_->memory())),
                           shared_memory_->size());
  }
}

// static
BigBuffer BigBufferView::ToBigBuffer(BigBufferView view) {
  BigBuffer buffer;
  buffer.storage_type_ = view.storage_type_;
  if (view.storage_type_ == BigBuffer::StorageType::kBytes) {
    std::copy(view.bytes_.begin(), view.bytes_.end(),
              std::back_inserter(buffer.bytes_));
  } else {
    buffer.shared_memory_ = std::move(*view.shared_memory_);
  }
  return buffer;
}

}  // namespace mojo_base
