// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/service_worker/service_worker_cache_writer.h"

#include <algorithm>
#include <string>

#include "content/browser/appcache/appcache_response.h"
#include "content/browser/service_worker/service_worker_disk_cache.h"
#include "content/browser/service_worker/service_worker_storage.h"

namespace {

const size_t kCopyBufferSize = 16 * 1024;

// Shim class used to turn always-async functions into async-or-result
// functions. See the comments below near ReadInfoHelper.
class AsyncOnlyCompletionCallbackAdaptor
    : public base::RefCounted<AsyncOnlyCompletionCallbackAdaptor> {
 public:
  explicit AsyncOnlyCompletionCallbackAdaptor(
      const net::CompletionCallback& callback)
      : async_(false), result_(net::ERR_IO_PENDING), callback_(callback) {}

  void set_async(bool async) { async_ = async; }
  bool async() { return async_; }
  int result() { return result_; }

  void WrappedCallback(int result) {
    result_ = result;
    if (async_)
      callback_.Run(result);
  }

 private:
  friend class base::RefCounted<AsyncOnlyCompletionCallbackAdaptor>;
  virtual ~AsyncOnlyCompletionCallbackAdaptor() {}

  bool async_;
  int result_;
  net::CompletionCallback callback_;
};

}  // namespace

namespace content {

int ServiceWorkerCacheWriter::DoLoop(int status) {
  do {
    switch (state_) {
      case STATE_START:
        status = DoStart(status);
        break;
      case STATE_READ_HEADERS_FOR_COMPARE:
        status = DoReadHeadersForCompare(status);
        break;
      case STATE_READ_HEADERS_FOR_COMPARE_DONE:
        status = DoReadHeadersForCompareDone(status);
        break;
      case STATE_READ_DATA_FOR_COMPARE:
        status = DoReadDataForCompare(status);
        break;
      case STATE_READ_DATA_FOR_COMPARE_DONE:
        status = DoReadDataForCompareDone(status);
        break;
      case STATE_READ_HEADERS_FOR_COPY:
        status = DoReadHeadersForCopy(status);
        break;
      case STATE_READ_HEADERS_FOR_COPY_DONE:
        status = DoReadHeadersForCopyDone(status);
        break;
      case STATE_READ_DATA_FOR_COPY:
        status = DoReadDataForCopy(status);
        break;
      case STATE_READ_DATA_FOR_COPY_DONE:
        status = DoReadDataForCopyDone(status);
        break;
      case STATE_WRITE_HEADERS_FOR_PASSTHROUGH:
        status = DoWriteHeadersForPassthrough(status);
        break;
      case STATE_WRITE_HEADERS_FOR_PASSTHROUGH_DONE:
        status = DoWriteHeadersForPassthroughDone(status);
        break;
      case STATE_WRITE_DATA_FOR_PASSTHROUGH:
        status = DoWriteDataForPassthrough(status);
        break;
      case STATE_WRITE_DATA_FOR_PASSTHROUGH_DONE:
        status = DoWriteDataForPassthroughDone(status);
        break;
      case STATE_WRITE_HEADERS_FOR_COPY:
        status = DoWriteHeadersForCopy(status);
        break;
      case STATE_WRITE_HEADERS_FOR_COPY_DONE:
        status = DoWriteHeadersForCopyDone(status);
        break;
      case STATE_WRITE_DATA_FOR_COPY:
        status = DoWriteDataForCopy(status);
        break;
      case STATE_WRITE_DATA_FOR_COPY_DONE:
        status = DoWriteDataForCopyDone(status);
        break;
      case STATE_DONE:
        status = DoDone(status);
        break;
      default:
        NOTREACHED() << "Unknown state in DoLoop";
        state_ = STATE_DONE;
        break;
    }
  } while (status != net::ERR_IO_PENDING && state_ != STATE_DONE);
  io_pending_ = (status == net::ERR_IO_PENDING);
  return status;
}

ServiceWorkerCacheWriter::ServiceWorkerCacheWriter(
    std::unique_ptr<ServiceWorkerResponseReader> compare_reader,
    std::unique_ptr<ServiceWorkerResponseReader> copy_reader,
    std::unique_ptr<ServiceWorkerResponseWriter> writer)
    : state_(STATE_START),
      io_pending_(false),
      comparing_(false),
      did_replace_(false),
      compare_reader_(std::move(compare_reader)),
      copy_reader_(std::move(copy_reader)),
      writer_(std::move(writer)),
      weak_factory_(this) {}

ServiceWorkerCacheWriter::~ServiceWorkerCacheWriter() {}

net::Error ServiceWorkerCacheWriter::MaybeWriteHeaders(
    HttpResponseInfoIOBuffer* headers,
    OnWriteCompleteCallback callback) {
  DCHECK(!io_pending_);

  headers_to_write_ = headers;
  pending_callback_ = std::move(callback);
  DCHECK_EQ(state_, STATE_START);
  int result = DoLoop(net::OK);

  // Synchronous errors and successes always go to STATE_DONE.
  if (result != net::ERR_IO_PENDING)
    DCHECK_EQ(state_, STATE_DONE);

  // ERR_IO_PENDING has to have one of the STATE_*_DONE states as the next state
  // (not STATE_DONE itself).
  if (result == net::ERR_IO_PENDING) {
    DCHECK(state_ == STATE_READ_HEADERS_FOR_COMPARE_DONE ||
           state_ == STATE_WRITE_HEADERS_FOR_COPY_DONE ||
           state_ == STATE_WRITE_HEADERS_FOR_PASSTHROUGH_DONE)
        << "Unexpected state: " << state_;
    io_pending_ = true;
  }

  return result >= 0 ? net::OK : static_cast<net::Error>(result);
}

net::Error ServiceWorkerCacheWriter::MaybeWriteData(
    net::IOBuffer* buf,
    size_t buf_size,
    OnWriteCompleteCallback callback) {
  DCHECK(!io_pending_);

  data_to_write_ = buf;
  len_to_write_ = buf_size;
  pending_callback_ = std::move(callback);

  if (comparing_)
    state_ = STATE_READ_DATA_FOR_COMPARE;
  else
    state_ = STATE_WRITE_DATA_FOR_PASSTHROUGH;

  int result = DoLoop(net::OK);

  // Synchronous completions are always STATE_DONE.
  if (result != net::ERR_IO_PENDING)
    DCHECK_EQ(state_, STATE_DONE);

  // Asynchronous completion means the state machine must be waiting in one of
  // the Done states for an IO operation to complete:
  if (result == net::ERR_IO_PENDING) {
    // Note that STATE_READ_HEADERS_FOR_COMPARE_DONE is excluded because the
    // headers are compared in MaybeWriteHeaders, not here, and
    // STATE_WRITE_HEADERS_FOR_PASSTHROUGH_DONE is excluded because that write
    // is done by MaybeWriteHeaders.
    DCHECK(state_ == STATE_READ_DATA_FOR_COMPARE_DONE ||
           state_ == STATE_READ_HEADERS_FOR_COPY_DONE ||
           state_ == STATE_READ_DATA_FOR_COPY_DONE ||
           state_ == STATE_WRITE_HEADERS_FOR_COPY_DONE ||
           state_ == STATE_WRITE_DATA_FOR_COPY_DONE ||
           state_ == STATE_WRITE_DATA_FOR_PASSTHROUGH_DONE)
        << "Unexpected state: " << state_;
  }

  return result >= 0 ? net::OK : static_cast<net::Error>(result);
}

int ServiceWorkerCacheWriter::DoStart(int result) {
  bytes_written_ = 0;
  if (compare_reader_) {
    state_ = STATE_READ_HEADERS_FOR_COMPARE;
    comparing_ = true;
  } else {
    // No existing reader, just write the headers back directly.
    state_ = STATE_WRITE_HEADERS_FOR_PASSTHROUGH;
    comparing_ = false;
  }
  return net::OK;
}

int ServiceWorkerCacheWriter::DoReadHeadersForCompare(int result) {
  DCHECK_GE(result, 0);
  DCHECK(headers_to_write_);

  headers_to_read_ = new HttpResponseInfoIOBuffer;
  state_ = STATE_READ_HEADERS_FOR_COMPARE_DONE;
  return ReadInfoHelper(compare_reader_, headers_to_read_.get());
}

int ServiceWorkerCacheWriter::DoReadHeadersForCompareDone(int result) {
  if (result < 0) {
    state_ = STATE_DONE;
    return result;
  }
  cached_length_ = headers_to_read_->response_data_size;
  bytes_compared_ = 0;
  state_ = STATE_DONE;
  return net::OK;
}

int ServiceWorkerCacheWriter::DoReadDataForCompare(int result) {
  DCHECK_GE(result, 0);
  DCHECK(data_to_write_);

  data_to_read_ = new net::IOBuffer(len_to_write_);
  len_to_read_ = len_to_write_;
  state_ = STATE_READ_DATA_FOR_COMPARE_DONE;
  compare_offset_ = 0;
  // If this was an EOF, don't issue a read.
  if (len_to_write_ > 0)
    result = ReadDataHelper(compare_reader_, data_to_read_.get(), len_to_read_);
  return result;
}

int ServiceWorkerCacheWriter::DoReadDataForCompareDone(int result) {
  DCHECK(data_to_read_);
  DCHECK(data_to_write_);
  DCHECK_EQ(len_to_read_, len_to_write_);

  if (result < 0) {
    state_ = STATE_DONE;
    return result;
  }

  DCHECK_LE(result + compare_offset_, static_cast<size_t>(len_to_write_));

  // Premature EOF while reading the service worker script cache data to
  // compare. Fail the comparison.
  if (result == 0 && len_to_write_ != 0) {
    comparing_ = false;
    state_ = STATE_READ_HEADERS_FOR_COPY;
    return net::OK;
  }

  // Compare the data from the ServiceWorker script cache to the data from the
  // network.
  if (memcmp(data_to_read_->data(), data_to_write_->data() + compare_offset_,
             result)) {
    // Data mismatched. This method already validated that all the bytes through
    // |bytes_compared_| were identical, so copy the first |bytes_compared_|
    // over, then start writing network data back after the changed point.
    comparing_ = false;
    state_ = STATE_READ_HEADERS_FOR_COPY;
    return net::OK;
  }

  compare_offset_ += result;

  // This is a little bit tricky. It is possible that not enough data was read
  // to finish comparing the entire block of data from the network (which is
  // kept in len_to_write_), so this method may need to issue another read and
  // return to this state.
  //
  // Compare isn't complete yet. Issue another read for the remaining data. Note
  // that this reuses the same IOBuffer.
  if (compare_offset_ < static_cast<size_t>(len_to_read_)) {
    state_ = STATE_READ_DATA_FOR_COMPARE_DONE;
    return ReadDataHelper(compare_reader_, data_to_read_.get(),
                          len_to_read_ - compare_offset_);
  }

  // Cached entry is longer than the network entry but the prefix matches. Copy
  // just the prefix.
  if (len_to_read_ == 0 && bytes_compared_ + compare_offset_ < cached_length_) {
    comparing_ = false;
    state_ = STATE_READ_HEADERS_FOR_COPY;
    return net::OK;
  }

  // bytes_compared_ only gets incremented when a full block is compared, to
  // avoid having to use only parts of the buffered network data.
  bytes_compared_ += result;
  state_ = STATE_DONE;
  return net::OK;
}

int ServiceWorkerCacheWriter::DoReadHeadersForCopy(int result) {
  DCHECK_GE(result, 0);
  DCHECK(copy_reader_);
  bytes_copied_ = 0;
  headers_to_read_ = new HttpResponseInfoIOBuffer;
  data_to_copy_ = new net::IOBuffer(kCopyBufferSize);
  state_ = STATE_READ_HEADERS_FOR_COPY_DONE;
  return ReadInfoHelper(copy_reader_, headers_to_read_.get());
}

int ServiceWorkerCacheWriter::DoReadHeadersForCopyDone(int result) {
  if (result < 0) {
    state_ = STATE_DONE;
    return result;
  }
  state_ = STATE_WRITE_HEADERS_FOR_COPY;
  return net::OK;
}

// Write the just-read headers back to the cache.
// Note that this *discards* the read headers and replaces them with the net
// headers.
int ServiceWorkerCacheWriter::DoWriteHeadersForCopy(int result) {
  DCHECK_GE(result, 0);
  DCHECK(writer_);
  state_ = STATE_WRITE_HEADERS_FOR_COPY_DONE;
  return WriteInfoHelper(writer_, headers_to_write_.get());
}

int ServiceWorkerCacheWriter::DoWriteHeadersForCopyDone(int result) {
  if (result < 0) {
    state_ = STATE_DONE;
    return result;
  }
  state_ = STATE_READ_DATA_FOR_COPY;
  return net::OK;
}

int ServiceWorkerCacheWriter::DoReadDataForCopy(int result) {
  DCHECK_GE(result, 0);
  size_t to_read = std::min(kCopyBufferSize, bytes_compared_ - bytes_copied_);
  // At this point, all compared bytes have been read. Currently
  // |data_to_write_| and |len_to_write_| hold the chunk of network input that
  // caused the comparison failure, so those need to be written back and this
  // object needs to go into passthrough mode.
  if (to_read == 0) {
    state_ = STATE_WRITE_DATA_FOR_PASSTHROUGH;
    return net::OK;
  }
  state_ = STATE_READ_DATA_FOR_COPY_DONE;
  return ReadDataHelper(copy_reader_, data_to_copy_.get(), to_read);
}

int ServiceWorkerCacheWriter::DoReadDataForCopyDone(int result) {
  if (result < 0) {
    state_ = STATE_DONE;
    return result;
  }
  state_ = STATE_WRITE_DATA_FOR_COPY;
  return result;
}

int ServiceWorkerCacheWriter::DoWriteDataForCopy(int result) {
  state_ = STATE_WRITE_DATA_FOR_COPY_DONE;
  DCHECK_GT(result, 0);
  return WriteDataHelper(writer_, data_to_copy_.get(), result);
}

int ServiceWorkerCacheWriter::DoWriteDataForCopyDone(int result) {
  if (result < 0) {
    state_ = STATE_DONE;
    return result;
  }
  bytes_written_ += result;
  bytes_copied_ += result;
  state_ = STATE_READ_DATA_FOR_COPY;
  return result;
}

int ServiceWorkerCacheWriter::DoWriteHeadersForPassthrough(int result) {
  DCHECK_GE(result, 0);
  DCHECK(writer_);
  state_ = STATE_WRITE_HEADERS_FOR_PASSTHROUGH_DONE;
  return WriteInfoHelper(writer_, headers_to_write_.get());
}

int ServiceWorkerCacheWriter::DoWriteHeadersForPassthroughDone(int result) {
  state_ = STATE_DONE;
  return result;
}

int ServiceWorkerCacheWriter::DoWriteDataForPassthrough(int result) {
  DCHECK_GE(result, 0);
  state_ = STATE_WRITE_DATA_FOR_PASSTHROUGH_DONE;
  if (len_to_write_ > 0)
    result = WriteDataHelper(writer_, data_to_write_.get(), len_to_write_);
  return result;
}

int ServiceWorkerCacheWriter::DoWriteDataForPassthroughDone(int result) {
  if (result < 0) {
    state_ = STATE_DONE;
    return result;
  }
  bytes_written_ += result;
  state_ = STATE_DONE;
  return net::OK;
}

int ServiceWorkerCacheWriter::DoDone(int result) {
  state_ = STATE_DONE;
  return result;
}

// These helpers adapt the AppCache "always use the callback" pattern to the
// //net "only use the callback for async" pattern using
// AsyncCompletionCallbackAdaptor.
//
// Specifically, these methods return result codes directly for synchronous
// completions, and only run their callback (which is AsyncDoLoop) for
// asynchronous completions.

int ServiceWorkerCacheWriter::ReadInfoHelper(
    const std::unique_ptr<ServiceWorkerResponseReader>& reader,
    HttpResponseInfoIOBuffer* buf) {
  net::CompletionCallback run_callback = base::Bind(
      &ServiceWorkerCacheWriter::AsyncDoLoop, weak_factory_.GetWeakPtr());
  scoped_refptr<AsyncOnlyCompletionCallbackAdaptor> adaptor(
      new AsyncOnlyCompletionCallbackAdaptor(std::move(run_callback)));
  reader->ReadInfo(
      buf, base::BindOnce(&AsyncOnlyCompletionCallbackAdaptor::WrappedCallback,
                          adaptor));
  adaptor->set_async(true);
  return adaptor->result();
}

int ServiceWorkerCacheWriter::ReadDataHelper(
    const std::unique_ptr<ServiceWorkerResponseReader>& reader,
    net::IOBuffer* buf,
    int buf_len) {
  net::CompletionCallback run_callback = base::Bind(
      &ServiceWorkerCacheWriter::AsyncDoLoop, weak_factory_.GetWeakPtr());
  scoped_refptr<AsyncOnlyCompletionCallbackAdaptor> adaptor(
      new AsyncOnlyCompletionCallbackAdaptor(std::move(run_callback)));
  reader->ReadData(
      buf, buf_len,
      base::BindOnce(&AsyncOnlyCompletionCallbackAdaptor::WrappedCallback,
                     adaptor));
  adaptor->set_async(true);
  return adaptor->result();
}

int ServiceWorkerCacheWriter::WriteInfoHelper(
    const std::unique_ptr<ServiceWorkerResponseWriter>& writer,
    HttpResponseInfoIOBuffer* buf) {
  did_replace_ = true;
  net::CompletionCallback run_callback = base::Bind(
      &ServiceWorkerCacheWriter::AsyncDoLoop, weak_factory_.GetWeakPtr());
  scoped_refptr<AsyncOnlyCompletionCallbackAdaptor> adaptor(
      new AsyncOnlyCompletionCallbackAdaptor(std::move(run_callback)));
  writer->WriteInfo(
      buf, base::BindOnce(&AsyncOnlyCompletionCallbackAdaptor::WrappedCallback,
                          adaptor));
  adaptor->set_async(true);
  return adaptor->result();
}

int ServiceWorkerCacheWriter::WriteDataHelper(
    const std::unique_ptr<ServiceWorkerResponseWriter>& writer,
    net::IOBuffer* buf,
    int buf_len) {
  net::CompletionCallback run_callback = base::Bind(
      &ServiceWorkerCacheWriter::AsyncDoLoop, weak_factory_.GetWeakPtr());
  scoped_refptr<AsyncOnlyCompletionCallbackAdaptor> adaptor(
      new AsyncOnlyCompletionCallbackAdaptor(std::move(run_callback)));
  writer->WriteData(
      buf, buf_len,
      base::BindOnce(&AsyncOnlyCompletionCallbackAdaptor::WrappedCallback,
                     adaptor));
  adaptor->set_async(true);
  return adaptor->result();
}

void ServiceWorkerCacheWriter::AsyncDoLoop(int result) {
  result = DoLoop(result);
  // If the result is ERR_IO_PENDING, the pending callback will be run by a
  // later invocation of AsyncDoLoop.
  if (result != net::ERR_IO_PENDING) {
    OnWriteCompleteCallback callback = std::move(pending_callback_);
    pending_callback_.Reset();
    net::Error error = result >= 0 ? net::OK : static_cast<net::Error>(result);
    io_pending_ = false;
    std::move(callback).Run(error);
  }
}

}  // namespace content
