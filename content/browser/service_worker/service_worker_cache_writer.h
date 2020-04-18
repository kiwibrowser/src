// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_SERVICE_WORKER_SERVICE_WORKER_CACHE_WRITER_H_
#define CONTENT_BROWSER_SERVICE_WORKER_SERVICE_WORKER_CACHE_WRITER_H_

#include <stddef.h>

#include <map>
#include <memory>
#include <set>

#include "base/callback.h"
#include "base/memory/weak_ptr.h"
#include "content/common/content_export.h"
#include "net/base/io_buffer.h"
#include "net/base/net_errors.h"

namespace content {

struct HttpResponseInfoIOBuffer;
class ServiceWorkerResponseReader;
class ServiceWorkerResponseWriter;

// This class is responsible for possibly updating the ServiceWorker script
// cache for an installed ServiceWorker main script. If there is no existing
// cache entry, this class always writes supplied data back to the cache; if
// there is an existing cache entry, this class only writes supplied data back
// if there is a cache mismatch.
//
// Note that writes done by this class cannot be "short" - ie, if they succeed,
// they always write all the supplied data back. Therefore completions are
// signalled with net::Error without a count of bytes written.
//
// This class's behavior is modelled as a state machine; see the DoLoop function
// for comments about this.
class CONTENT_EXPORT ServiceWorkerCacheWriter {
 public:
  using OnWriteCompleteCallback = base::OnceCallback<void(net::Error)>;

  // The |compare_reader| may be null, in which case this instance will
  // unconditionally write back data supplied to |MaybeWriteHeaders| and
  // |MaybeWriteData|.
  ServiceWorkerCacheWriter(
      std::unique_ptr<ServiceWorkerResponseReader> compare_reader,
      std::unique_ptr<ServiceWorkerResponseReader> copy_reader,
      std::unique_ptr<ServiceWorkerResponseWriter> writer);

  ~ServiceWorkerCacheWriter();

  // Writes the supplied |headers| back to the cache. Returns ERR_IO_PENDING if
  // the write will complete asynchronously, in which case |callback| will be
  // called when it completes. Otherwise, returns a code other than
  // ERR_IO_PENDING and does not invoke |callback|. Note that this method will
  // not necessarily write data back to the cache if the incoming data is
  // equivalent to the existing cached data. See the source of this function for
  // details about how this function drives the state machine.
  net::Error MaybeWriteHeaders(HttpResponseInfoIOBuffer* headers,
                               OnWriteCompleteCallback callback);

  // Writes the supplied body data |data| back to the cache. Returns
  // ERR_IO_PENDING if the write will complete asynchronously, in which case
  // |callback| will be called when it completes. Otherwise, returns a code
  // other than ERR_IO_PENDING and does not invoke |callback|. Note that this
  // method will not necessarily write data back to the cache if the incoming
  // data is equivalent to the existing cached data. See the source of this
  // function for details about how this function drives the state machine.
  net::Error MaybeWriteData(net::IOBuffer* buf,
                            size_t buf_size,
                            OnWriteCompleteCallback callback);

  // Returns a count of bytes written back to the cache.
  size_t bytes_written() const { return bytes_written_; }
  bool did_replace() const { return did_replace_; }

 private:
  // States for the state machine.
  //
  // The state machine flows roughly like this: if there is no existing cache
  // entry, incoming headers and data are written directly back to the cache
  // ("passthrough mode", the PASSTHROUGH states). If there is an existing cache
  // entry, incoming headers and data are compared to the existing cache entry
  // ("compare mode", the COMPARE states); if at any point the incoming
  // headers/data are not equal to the cached headers/data, this class copies
  // the cached data up to the point where the incoming data and the cached data
  // diverged ("copy mode", the COPY states), then switches to "passthrough
  // mode" to write the remainder of the incoming data. The overall effect is to
  // avoid rewriting the cache entry if the incoming data is identical to the
  // cached data.
  //
  // Note that after a call to MaybeWriteHeaders or MaybeWriteData completes,
  // the machine is always in STATE_DONE, indicating that the call is finished;
  // those methods are responsible for setting a new initial state.
  enum State {
    STATE_START,
    // Control flows linearly through these four states, then loops from
    // READ_DATA_FOR_COMPARE_DONE to READ_DATA_FOR_COMPARE, or exits to
    // READ_HEADERS_FOR_COPY.
    STATE_READ_HEADERS_FOR_COMPARE,
    STATE_READ_HEADERS_FOR_COMPARE_DONE,
    STATE_READ_DATA_FOR_COMPARE,
    STATE_READ_DATA_FOR_COMPARE_DONE,

    // Control flows linearly through these states, with each pass from
    // READ_DATA_FOR_COPY to WRITE_DATA_FOR_COPY_DONE copying one block of data
    // at a time. Control loops from WRITE_DATA_FOR_COPY_DONE back to
    // READ_DATA_FOR_COPY if there is more data to copy, or exits to
    // WRITE_DATA_FOR_PASSTHROUGH.
    STATE_READ_HEADERS_FOR_COPY,
    STATE_READ_HEADERS_FOR_COPY_DONE,
    STATE_WRITE_HEADERS_FOR_COPY,
    STATE_WRITE_HEADERS_FOR_COPY_DONE,
    STATE_READ_DATA_FOR_COPY,
    STATE_READ_DATA_FOR_COPY_DONE,
    STATE_WRITE_DATA_FOR_COPY,
    STATE_WRITE_DATA_FOR_COPY_DONE,

    // Control flows linearly through these states, with a loop between
    // WRITE_DATA_FOR_PASSTHROUGH and WRITE_DATA_FOR_PASSTHROUGH_DONE.
    STATE_WRITE_HEADERS_FOR_PASSTHROUGH,
    STATE_WRITE_HEADERS_FOR_PASSTHROUGH_DONE,
    STATE_WRITE_DATA_FOR_PASSTHROUGH,
    STATE_WRITE_DATA_FOR_PASSTHROUGH_DONE,

    // This state means "done with the current call; ready for another one."
    STATE_DONE,
  };

  // Drives this class's state machine. This function steps the state machine
  // until one of:
  //   a) One of the state functions returns an error
  //   b) The state machine reaches STATE_DONE
  // A successful value (net::OK or greater) indicates that the requested
  // operation completed synchronously. A return value of ERR_IO_PENDING
  // indicates that some step had to submit asynchronous IO for later
  // completion, and the state machine will resume running (via AsyncDoLoop)
  // when that asynchronous IO completes. Any other return value indicates that
  // the requested operation failed synchronously.
  int DoLoop(int result);

  // State handlers. See function comments in the corresponding source file for
  // details on these.
  int DoStart(int result);
  int DoReadHeadersForCompare(int result);
  int DoReadHeadersForCompareDone(int result);
  int DoReadDataForCompare(int result);
  int DoReadDataForCompareDone(int result);
  int DoReadHeadersForCopy(int result);
  int DoReadHeadersForCopyDone(int result);
  int DoWriteHeadersForCopy(int result);
  int DoWriteHeadersForCopyDone(int result);
  int DoReadDataForCopy(int result);
  int DoReadDataForCopyDone(int result);
  int DoWriteDataForCopy(int result);
  int DoWriteDataForCopyDone(int result);
  int DoWriteHeadersForPassthrough(int result);
  int DoWriteHeadersForPassthroughDone(int result);
  int DoWriteDataForPassthrough(int result);
  int DoWriteDataForPassthroughDone(int result);
  int DoDone(int result);

  // Wrappers for asynchronous calls. These are responsible for scheduling a
  // callback to drive the state machine if needed. These either:
  //   a) Return ERR_IO_PENDING, and schedule a callback to run the state
  //      machine's Run() later, or
  //   b) Return some other value and do not schedule a callback.
  int ReadInfoHelper(const std::unique_ptr<ServiceWorkerResponseReader>& reader,
                     HttpResponseInfoIOBuffer* buf);
  int ReadDataHelper(const std::unique_ptr<ServiceWorkerResponseReader>& reader,
                     net::IOBuffer* buf,
                     int buf_len);
  int WriteInfoHelper(
      const std::unique_ptr<ServiceWorkerResponseWriter>& writer,
      HttpResponseInfoIOBuffer* buf);
  int WriteDataHelper(
      const std::unique_ptr<ServiceWorkerResponseWriter>& writer,
      net::IOBuffer* buf,
      int buf_len);

  // Callback used by the above helpers for their IO operations. This is only
  // run when those IO operations complete asynchronously, in which case it
  // invokes the synchronous DoLoop function and runs the client callback (the
  // one passed into MaybeWriteData/MaybeWriteHeaders) if that invocation
  // of DoLoop completes synchronously.
  void AsyncDoLoop(int result);

  State state_;
  // Note that this variable is only used for assertions; it reflects "state !=
  // DONE && not in synchronous DoLoop".
  bool io_pending_;
  bool comparing_;

  scoped_refptr<HttpResponseInfoIOBuffer> headers_to_read_;
  scoped_refptr<HttpResponseInfoIOBuffer> headers_to_write_;
  scoped_refptr<net::IOBuffer> data_to_read_;
  int len_to_read_;
  scoped_refptr<net::IOBuffer> data_to_copy_;
  scoped_refptr<net::IOBuffer> data_to_write_;
  int len_to_write_;
  OnWriteCompleteCallback pending_callback_;

  size_t cached_length_;

  size_t bytes_compared_;
  size_t bytes_copied_;
  size_t bytes_written_;

  bool did_replace_;

  size_t compare_offset_;

  std::unique_ptr<ServiceWorkerResponseReader> compare_reader_;
  std::unique_ptr<ServiceWorkerResponseReader> copy_reader_;
  std::unique_ptr<ServiceWorkerResponseWriter> writer_;
  base::WeakPtrFactory<ServiceWorkerCacheWriter> weak_factory_;
};

}  // namespace content

#endif  // CONTENT_BROWSER_SERVICE_WORKER_SERVICE_WORKER_CACHE_WRITER_H_
