// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/platform/image-decoders/segment_reader.h"

#include "base/memory/scoped_refptr.h"
#include "third_party/blink/renderer/platform/shared_buffer.h"
#include "third_party/blink/renderer/platform/wtf/assertions.h"
#include "third_party/blink/renderer/platform/wtf/noncopyable.h"
#include "third_party/blink/renderer/platform/wtf/threading_primitives.h"
#include "third_party/skia/include/core/SkData.h"
#include "third_party/skia/include/core/SkRWBuffer.h"

namespace blink {

// SharedBufferSegmentReader ---------------------------------------------------

// Interface for ImageDecoder to read a SharedBuffer.
class SharedBufferSegmentReader final : public SegmentReader {
  WTF_MAKE_NONCOPYABLE(SharedBufferSegmentReader);

 public:
  SharedBufferSegmentReader(scoped_refptr<SharedBuffer>);
  size_t size() const override;
  size_t GetSomeData(const char*& data, size_t position) const override;
  sk_sp<SkData> GetAsSkData() const override;

 private:
  scoped_refptr<SharedBuffer> shared_buffer_;
};

SharedBufferSegmentReader::SharedBufferSegmentReader(
    scoped_refptr<SharedBuffer> buffer)
    : shared_buffer_(std::move(buffer)) {}

size_t SharedBufferSegmentReader::size() const {
  return shared_buffer_->size();
}

size_t SharedBufferSegmentReader::GetSomeData(const char*& data,
                                              size_t position) const {
  return shared_buffer_->GetSomeData(data, position);
}

sk_sp<SkData> SharedBufferSegmentReader::GetAsSkData() const {
  return shared_buffer_->GetAsSkData();
}

// DataSegmentReader -----------------------------------------------------------

// Interface for ImageDecoder to read an SkData.
class DataSegmentReader final : public SegmentReader {
  WTF_MAKE_NONCOPYABLE(DataSegmentReader);

 public:
  DataSegmentReader(sk_sp<SkData>);
  size_t size() const override;
  size_t GetSomeData(const char*& data, size_t position) const override;
  sk_sp<SkData> GetAsSkData() const override;

 private:
  sk_sp<SkData> data_;
};

DataSegmentReader::DataSegmentReader(sk_sp<SkData> data)
    : data_(std::move(data)) {}

size_t DataSegmentReader::size() const {
  return data_->size();
}

size_t DataSegmentReader::GetSomeData(const char*& data,
                                      size_t position) const {
  if (position >= data_->size())
    return 0;

  data = reinterpret_cast<const char*>(data_->bytes() + position);
  return data_->size() - position;
}

sk_sp<SkData> DataSegmentReader::GetAsSkData() const {
  return data_;
}

// ROBufferSegmentReader -------------------------------------------------------

class ROBufferSegmentReader final : public SegmentReader {
  WTF_MAKE_NONCOPYABLE(ROBufferSegmentReader);

 public:
  ROBufferSegmentReader(sk_sp<SkROBuffer>);

  size_t size() const override;
  size_t GetSomeData(const char*& data, size_t position) const override;
  sk_sp<SkData> GetAsSkData() const override;

 private:
  sk_sp<SkROBuffer> ro_buffer_;
  // Protects access to mutable fields.
  mutable Mutex read_mutex_;
  // Position of the first char in the current block of iter_.
  mutable size_t position_of_block_;
  mutable SkROBuffer::Iter iter_;
};

ROBufferSegmentReader::ROBufferSegmentReader(sk_sp<SkROBuffer> buffer)
    : ro_buffer_(std::move(buffer)),
      position_of_block_(0),
      iter_(ro_buffer_.get()) {}

size_t ROBufferSegmentReader::size() const {
  return ro_buffer_ ? ro_buffer_->size() : 0;
}

size_t ROBufferSegmentReader::GetSomeData(const char*& data,
                                          size_t position) const {
  if (!ro_buffer_)
    return 0;

  MutexLocker lock(read_mutex_);

  if (position < position_of_block_) {
    // SkROBuffer::Iter only iterates forwards. Start from the beginning.
    iter_.reset(ro_buffer_.get());
    position_of_block_ = 0;
  }

  for (size_t size_of_block = iter_.size(); size_of_block != 0;
       position_of_block_ += size_of_block, size_of_block = iter_.size()) {
    DCHECK_LE(position_of_block_, position);

    if (position_of_block_ + size_of_block > position) {
      // |position| is in this block.
      const size_t position_in_block = position - position_of_block_;
      data = static_cast<const char*>(iter_.data()) + position_in_block;
      return size_of_block - position_in_block;
    }

    // Move to next block.
    if (!iter_.next()) {
      // Reset to the beginning, so future calls can succeed.
      iter_.reset(ro_buffer_.get());
      position_of_block_ = 0;
      return 0;
    }
  }

  return 0;
}

static void UnrefROBuffer(const void* ptr, void* context) {
  static_cast<SkROBuffer*>(context)->unref();
}

sk_sp<SkData> ROBufferSegmentReader::GetAsSkData() const {
  if (!ro_buffer_)
    return nullptr;

  // Check to see if the data is already contiguous.
  SkROBuffer::Iter iter(ro_buffer_.get());
  const bool multiple_blocks = iter.next();
  iter.reset(ro_buffer_.get());

  if (!multiple_blocks) {
    // Contiguous data. No need to copy.
    ro_buffer_->ref();
    return SkData::MakeWithProc(iter.data(), iter.size(), &UnrefROBuffer,
                                ro_buffer_.get());
  }

  sk_sp<SkData> data = SkData::MakeUninitialized(ro_buffer_->size());
  char* dst = static_cast<char*>(data->writable_data());
  do {
    size_t size = iter.size();
    memcpy(dst, iter.data(), size);
    dst += size;
  } while (iter.next());
  return data;
}

// SegmentReader ---------------------------------------------------------------

scoped_refptr<SegmentReader> SegmentReader::CreateFromSharedBuffer(
    scoped_refptr<SharedBuffer> buffer) {
  return base::AdoptRef(new SharedBufferSegmentReader(std::move(buffer)));
}

scoped_refptr<SegmentReader> SegmentReader::CreateFromSkData(
    sk_sp<SkData> data) {
  return base::AdoptRef(new DataSegmentReader(std::move(data)));
}

scoped_refptr<SegmentReader> SegmentReader::CreateFromSkROBuffer(
    sk_sp<SkROBuffer> buffer) {
  return base::AdoptRef(new ROBufferSegmentReader(std::move(buffer)));
}

}  // namespace blink
