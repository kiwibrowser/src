// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/appcache/appcache_response.h"

#include <stddef.h>

#include "base/bind.h"
#include "base/bind_helpers.h"
#include "base/compiler_specific.h"
#include "base/location.h"
#include "base/logging.h"
#include "base/numerics/safe_math.h"
#include "base/pickle.h"
#include "base/single_thread_task_runner.h"
#include "base/strings/string_util.h"
#include "base/threading/thread_task_runner_handle.h"
#include "content/browser/appcache/appcache_storage.h"
#include "net/base/completion_callback.h"
#include "net/base/io_buffer.h"
#include "net/base/net_errors.h"
#include "storage/common/storage_histograms.h"

namespace content {

namespace {

// Disk cache entry data indices.
enum { kResponseInfoIndex, kResponseContentIndex, kResponseMetadataIndex };

// An IOBuffer that wraps a pickle's data. Ownership of the
// pickle is transfered to the WrappedPickleIOBuffer object.
class WrappedPickleIOBuffer : public net::WrappedIOBuffer {
 public:
  explicit WrappedPickleIOBuffer(const base::Pickle* pickle)
      : net::WrappedIOBuffer(reinterpret_cast<const char*>(pickle->data())),
        pickle_(pickle) {
    DCHECK(pickle->data());
  }

 private:
  ~WrappedPickleIOBuffer() override {}

  std::unique_ptr<const base::Pickle> pickle_;
};

}  // anon namespace


// AppCacheResponseInfo ----------------------------------------------

AppCacheResponseInfo::AppCacheResponseInfo(AppCacheStorage* storage,
                                           const GURL& manifest_url,
                                           int64_t response_id,
                                           net::HttpResponseInfo* http_info,
                                           int64_t response_data_size)
    : manifest_url_(manifest_url),
      response_id_(response_id),
      http_response_info_(http_info),
      response_data_size_(response_data_size),
      storage_(storage) {
  DCHECK(http_info);
  DCHECK(response_id != kAppCacheNoResponseId);
  storage_->working_set()->AddResponseInfo(this);
}

AppCacheResponseInfo::~AppCacheResponseInfo() {
  storage_->working_set()->RemoveResponseInfo(this);
}

// HttpResponseInfoIOBuffer ------------------------------------------

HttpResponseInfoIOBuffer::HttpResponseInfoIOBuffer()
    : response_data_size(kUnkownResponseDataSize) {}

HttpResponseInfoIOBuffer::HttpResponseInfoIOBuffer(net::HttpResponseInfo* info)
    : http_info(info), response_data_size(kUnkownResponseDataSize) {}

HttpResponseInfoIOBuffer::~HttpResponseInfoIOBuffer() {}

// AppCacheDiskCacheInterface ----------------------------------------

AppCacheDiskCacheInterface::AppCacheDiskCacheInterface(const char* uma_name)
    : uma_name_(uma_name), weak_factory_(this) {}

base::WeakPtr<AppCacheDiskCacheInterface>
AppCacheDiskCacheInterface::GetWeakPtr() {
  return weak_factory_.GetWeakPtr();
}

AppCacheDiskCacheInterface::~AppCacheDiskCacheInterface() {}

// AppCacheResponseIO ----------------------------------------------

AppCacheResponseIO::AppCacheResponseIO(
    int64_t response_id,
    const base::WeakPtr<AppCacheDiskCacheInterface>& disk_cache)
    : response_id_(response_id),
      disk_cache_(disk_cache),
      entry_(nullptr),
      buffer_len_(0),
      weak_factory_(this) {}

AppCacheResponseIO::~AppCacheResponseIO() {
  if (entry_)
    entry_->Close();
}

void AppCacheResponseIO::ScheduleIOCompletionCallback(int result) {
  base::ThreadTaskRunnerHandle::Get()->PostTask(
      FROM_HERE, base::BindOnce(&AppCacheResponseIO::OnIOComplete,
                                weak_factory_.GetWeakPtr(), result));
}

void AppCacheResponseIO::InvokeUserCompletionCallback(int result) {
  // Clear the user callback and buffers prior to invoking the callback
  // so the caller can schedule additional operations in the callback.
  buffer_ = nullptr;
  info_buffer_ = nullptr;
  OnceCompletionCallback cb = std::move(callback_);
  callback_.Reset();
  std::move(cb).Run(result);
}

void AppCacheResponseIO::ReadRaw(int index, int offset,
                                 net::IOBuffer* buf, int buf_len) {
  DCHECK(entry_);
  int rv = entry_->Read(
      index, offset, buf, buf_len,
      base::Bind(&AppCacheResponseIO::OnRawIOComplete,
                 weak_factory_.GetWeakPtr()));
  if (rv != net::ERR_IO_PENDING)
    ScheduleIOCompletionCallback(rv);
}

void AppCacheResponseIO::WriteRaw(int index, int offset,
                                 net::IOBuffer* buf, int buf_len) {
  DCHECK(entry_);
  int rv = entry_->Write(
      index, offset, buf, buf_len,
      base::Bind(&AppCacheResponseIO::OnRawIOComplete,
                 weak_factory_.GetWeakPtr()));
  if (rv != net::ERR_IO_PENDING)
    ScheduleIOCompletionCallback(rv);
}

void AppCacheResponseIO::OnRawIOComplete(int result) {
  DCHECK_NE(net::ERR_IO_PENDING, result);
  OnIOComplete(result);
}

void AppCacheResponseIO::OpenEntryIfNeeded() {
  int rv;
  AppCacheDiskCacheInterface::Entry** entry_ptr = nullptr;
  if (entry_) {
    rv = net::OK;
  } else if (!disk_cache_) {
    rv = net::ERR_FAILED;
  } else {
    entry_ptr = new AppCacheDiskCacheInterface::Entry*;
    open_callback_ =
        base::Bind(&AppCacheResponseIO::OpenEntryCallback,
                   weak_factory_.GetWeakPtr(), base::Owned(entry_ptr));
    rv = disk_cache_->OpenEntry(response_id_, entry_ptr, open_callback_);
  }

  if (rv != net::ERR_IO_PENDING)
    OpenEntryCallback(entry_ptr, rv);
}

void AppCacheResponseIO::OpenEntryCallback(
    AppCacheDiskCacheInterface::Entry** entry, int rv) {
  DCHECK(info_buffer_.get() || buffer_.get());

  if (!open_callback_.is_null()) {
    if (rv == net::OK) {
      DCHECK(entry);
      entry_ = *entry;
    }
    open_callback_.Reset();
  }
  OnOpenEntryComplete();
}


// AppCacheResponseReader ----------------------------------------------

AppCacheResponseReader::AppCacheResponseReader(
    int64_t response_id,
    const base::WeakPtr<AppCacheDiskCacheInterface>& disk_cache)
    : AppCacheResponseIO(response_id, disk_cache),
      range_offset_(0),
      range_length_(std::numeric_limits<int32_t>::max()),
      read_position_(0),
      reading_metadata_size_(0),
      weak_factory_(this) {}

AppCacheResponseReader::~AppCacheResponseReader() {
}

void AppCacheResponseReader::ReadInfo(HttpResponseInfoIOBuffer* info_buf,
                                      OnceCompletionCallback callback) {
  DCHECK(!callback.is_null());
  DCHECK(!IsReadPending());
  DCHECK(info_buf);
  DCHECK(!info_buf->http_info.get());
  DCHECK(!buffer_.get());
  DCHECK(!info_buffer_.get());

  info_buffer_ = info_buf;
  callback_ = std::move(callback);  // cleared on completion
  OpenEntryIfNeeded();
}

void AppCacheResponseReader::ContinueReadInfo() {
  int size = entry_->GetSize(kResponseInfoIndex);
  if (size <= 0) {
    ScheduleIOCompletionCallback(net::ERR_CACHE_MISS);
    return;
  }

  buffer_ = new net::IOBuffer(size);
  ReadRaw(kResponseInfoIndex, 0, buffer_.get(), size);
}

void AppCacheResponseReader::ReadData(net::IOBuffer* buf,
                                      int buf_len,
                                      OnceCompletionCallback callback) {
  DCHECK(!callback.is_null());
  DCHECK(!IsReadPending());
  DCHECK(buf);
  DCHECK(buf_len >= 0);
  DCHECK(!buffer_.get());
  DCHECK(!info_buffer_.get());

  buffer_ = buf;
  buffer_len_ = buf_len;
  callback_ = std::move(callback);  // cleared on completion
  OpenEntryIfNeeded();
}

void AppCacheResponseReader::ContinueReadData() {
  if (read_position_ + buffer_len_ > range_length_) {
    // TODO(michaeln): What about integer overflows?
    DCHECK(range_length_ >= read_position_);
    buffer_len_ = range_length_ - read_position_;
  }
  ReadRaw(kResponseContentIndex,
          range_offset_ + read_position_,
          buffer_.get(),
          buffer_len_);
}

void AppCacheResponseReader::SetReadRange(int offset, int length) {
  DCHECK(!IsReadPending() && !read_position_);
  range_offset_ = offset;
  range_length_ = length;
}

void AppCacheResponseReader::OnIOComplete(int result) {
  if (result >= 0) {
    if (reading_metadata_size_) {
      DCHECK(reading_metadata_size_ == result);
      DCHECK(info_buffer_->http_info->metadata);
      reading_metadata_size_ = 0;
    } else if (info_buffer_.get()) {
      // Deserialize the http info structure, ensuring we got headers.
      base::Pickle pickle(buffer_->data(), result);
      std::unique_ptr<net::HttpResponseInfo> info(new net::HttpResponseInfo);
      bool response_truncated = false;
      if (!info->InitFromPickle(pickle, &response_truncated) ||
          !info->headers.get()) {
        InvokeUserCompletionCallback(net::ERR_FAILED);
        return;
      }
      DCHECK(!response_truncated);
      info_buffer_->http_info.reset(info.release());

      // Also return the size of the response body
      DCHECK(entry_);
      info_buffer_->response_data_size =
          entry_->GetSize(kResponseContentIndex);

      int64_t metadata_size = entry_->GetSize(kResponseMetadataIndex);
      if (metadata_size > 0) {
        reading_metadata_size_ = metadata_size;
        info_buffer_->http_info->metadata = new net::IOBufferWithSize(
            base::checked_cast<size_t>(metadata_size));
        ReadRaw(kResponseMetadataIndex, 0,
                info_buffer_->http_info->metadata.get(), metadata_size);
        return;
      }
    } else {
      read_position_ += result;
    }
  }
  if (result > 0 && disk_cache_)
    storage::RecordBytesRead(disk_cache_->uma_name(), result);
  InvokeUserCompletionCallback(result);
  // Note: |this| may have been deleted by the completion callback.
}

void AppCacheResponseReader::OnOpenEntryComplete() {
  if (!entry_)  {
    ScheduleIOCompletionCallback(net::ERR_CACHE_MISS);
    return;
  }
  if (info_buffer_.get())
    ContinueReadInfo();
  else
    ContinueReadData();
}

// AppCacheResponseWriter ----------------------------------------------

AppCacheResponseWriter::AppCacheResponseWriter(
    int64_t response_id,
    const base::WeakPtr<AppCacheDiskCacheInterface>& disk_cache)
    : AppCacheResponseIO(response_id, disk_cache),
      info_size_(0),
      write_position_(0),
      write_amount_(0),
      creation_phase_(INITIAL_ATTEMPT),
      weak_factory_(this) {}

AppCacheResponseWriter::~AppCacheResponseWriter() {
}

void AppCacheResponseWriter::WriteInfo(HttpResponseInfoIOBuffer* info_buf,
                                       OnceCompletionCallback callback) {
  DCHECK(!callback.is_null());
  DCHECK(!IsWritePending());
  DCHECK(info_buf);
  DCHECK(info_buf->http_info.get());
  DCHECK(!buffer_.get());
  DCHECK(!info_buffer_.get());
  DCHECK(info_buf->http_info->headers.get());

  info_buffer_ = info_buf;
  callback_ = std::move(callback);  // cleared on completion
  CreateEntryIfNeededAndContinue();
}

void AppCacheResponseWriter::ContinueWriteInfo() {
  if (!entry_) {
    ScheduleIOCompletionCallback(net::ERR_FAILED);
    return;
  }

  const bool kSkipTransientHeaders = true;
  const bool kTruncated = false;
  base::Pickle* pickle = new base::Pickle;
  info_buffer_->http_info->Persist(pickle, kSkipTransientHeaders, kTruncated);
  write_amount_ = static_cast<int>(pickle->size());
  buffer_ = new WrappedPickleIOBuffer(pickle);  // takes ownership of pickle
  WriteRaw(kResponseInfoIndex, 0, buffer_.get(), write_amount_);
}

void AppCacheResponseWriter::WriteData(net::IOBuffer* buf,
                                       int buf_len,
                                       OnceCompletionCallback callback) {
  DCHECK(!callback.is_null());
  DCHECK(!IsWritePending());
  DCHECK(buf);
  DCHECK(buf_len >= 0);
  DCHECK(!buffer_.get());
  DCHECK(!info_buffer_.get());

  buffer_ = buf;
  write_amount_ = buf_len;
  callback_ = std::move(callback);  // cleared on completion
  CreateEntryIfNeededAndContinue();
}

void AppCacheResponseWriter::ContinueWriteData() {
  if (!entry_) {
    ScheduleIOCompletionCallback(net::ERR_FAILED);
    return;
  }
  WriteRaw(
      kResponseContentIndex, write_position_, buffer_.get(), write_amount_);
}

void AppCacheResponseWriter::OnIOComplete(int result) {
  if (result >= 0) {
    DCHECK(write_amount_ == result);
    if (!info_buffer_.get())
      write_position_ += result;
    else
      info_size_ = result;
  }
  if (result > 0 && disk_cache_)
    storage::RecordBytesWritten(disk_cache_->uma_name(), result);
  InvokeUserCompletionCallback(result);
  // Note: |this| may have been deleted by the completion callback.
}

void AppCacheResponseWriter::CreateEntryIfNeededAndContinue() {
  int rv;
  AppCacheDiskCacheInterface::Entry** entry_ptr = nullptr;
  if (entry_) {
    creation_phase_ = NO_ATTEMPT;
    rv = net::OK;
  } else if (!disk_cache_) {
    creation_phase_ = NO_ATTEMPT;
    rv = net::ERR_FAILED;
  } else {
    creation_phase_ = INITIAL_ATTEMPT;
    entry_ptr = new AppCacheDiskCacheInterface::Entry*;
    create_callback_ =
        base::Bind(&AppCacheResponseWriter::OnCreateEntryComplete,
                   weak_factory_.GetWeakPtr(), base::Owned(entry_ptr));
    rv = disk_cache_->CreateEntry(response_id_, entry_ptr, create_callback_);
  }
  if (rv != net::ERR_IO_PENDING)
    OnCreateEntryComplete(entry_ptr, rv);
}

void AppCacheResponseWriter::OnCreateEntryComplete(
    AppCacheDiskCacheInterface::Entry** entry, int rv) {
  DCHECK(info_buffer_.get() || buffer_.get());

  if (!disk_cache_) {
    ScheduleIOCompletionCallback(net::ERR_FAILED);
    return;
  } else if (creation_phase_ == INITIAL_ATTEMPT) {
    if (rv != net::OK) {
      // We may try to overwrite existing entries.
      creation_phase_ = DOOM_EXISTING;
      rv = disk_cache_->DoomEntry(response_id_, create_callback_);
      if (rv != net::ERR_IO_PENDING)
        OnCreateEntryComplete(nullptr, rv);
      return;
    }
  } else if (creation_phase_ == DOOM_EXISTING) {
    creation_phase_ = SECOND_ATTEMPT;
    AppCacheDiskCacheInterface::Entry** entry_ptr =
        new AppCacheDiskCacheInterface::Entry*;
    create_callback_ =
        base::Bind(&AppCacheResponseWriter::OnCreateEntryComplete,
                   weak_factory_.GetWeakPtr(), base::Owned(entry_ptr));
    rv = disk_cache_->CreateEntry(response_id_, entry_ptr, create_callback_);
    if (rv != net::ERR_IO_PENDING)
      OnCreateEntryComplete(entry_ptr, rv);
    return;
  }

  if (!create_callback_.is_null()) {
    if (rv == net::OK)
      entry_ = *entry;

    create_callback_.Reset();
  }

  if (info_buffer_.get())
    ContinueWriteInfo();
  else
    ContinueWriteData();
}

// AppCacheResponseMetadataWriter ----------------------------------------------

AppCacheResponseMetadataWriter::AppCacheResponseMetadataWriter(
    int64_t response_id,
    const base::WeakPtr<AppCacheDiskCacheInterface>& disk_cache)
    : AppCacheResponseIO(response_id, disk_cache),
      write_amount_(0),
      weak_factory_(this) {}

AppCacheResponseMetadataWriter::~AppCacheResponseMetadataWriter() {
}

void AppCacheResponseMetadataWriter::WriteMetadata(
    net::IOBuffer* buf,
    int buf_len,
    const net::CompletionCallback& callback) {
  DCHECK(!callback.is_null());
  DCHECK(!IsIOPending());
  DCHECK(buf);
  DCHECK(buf_len >= 0);
  DCHECK(!buffer_.get());

  buffer_ = buf;
  write_amount_ = buf_len;
  callback_ = callback;  // cleared on completion
  OpenEntryIfNeeded();
}

void AppCacheResponseMetadataWriter::OnOpenEntryComplete() {
  if (!entry_) {
    ScheduleIOCompletionCallback(net::ERR_FAILED);
    return;
  }
  WriteRaw(kResponseMetadataIndex, 0, buffer_.get(), write_amount_);
}

void AppCacheResponseMetadataWriter::OnIOComplete(int result) {
  DCHECK(result < 0 || write_amount_ == result);
  if (result > 0 && disk_cache_)
    storage::RecordBytesWritten(disk_cache_->uma_name(), result);
  InvokeUserCompletionCallback(result);
  // Note: |this| may have been deleted by the completion callback.
}

}  // namespace content
