// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_STREAMS_STREAM_H_
#define CONTENT_BROWSER_STREAMS_STREAM_H_

#include <stddef.h>

#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "base/memory/weak_ptr.h"
#include "content/browser/byte_stream.h"
#include "content/common/content_export.h"
#include "url/gurl.h"

namespace net {
class HttpResponseInfo;
class IOBuffer;
}

namespace content {

class StreamHandle;
class StreamHandleImpl;
class StreamMetadata;
class StreamReadObserver;
class StreamRegistry;
class StreamWriteObserver;

// A stream that sends data from an arbitrary source to an internal URL
// that can be read by an internal consumer.  It will continue to pull from the
// original URL as long as there is data available.  It can be read from
// multiple clients, but only one can be reading at a time. This allows a
// reader to consume part of the stream, then pass it along to another client
// to continue processing the stream.
class CONTENT_EXPORT Stream : public base::RefCountedThreadSafe<Stream> {
 public:
  enum StreamState {
    STREAM_HAS_DATA,
    STREAM_COMPLETE,
    STREAM_EMPTY,
    STREAM_ABORTED,
  };

  // Creates a stream.
  //
  // Security origin of Streams is checked in Blink (See BlobRegistry,
  // BlobURL and SecurityOrigin to understand how it works). There's no security
  // origin check in Chromium side for now.
  Stream(StreamRegistry* registry,
         StreamWriteObserver* write_observer,
         const GURL& url);

  // Sets the reader of this stream. Returns true on success, or false if there
  // is already a reader.
  bool SetReadObserver(StreamReadObserver* observer);

  // Removes the read observer.  |observer| must be the current observer.
  void RemoveReadObserver(StreamReadObserver* observer);

  // Removes the write observer.  |observer| must be the current observer.
  void RemoveWriteObserver(StreamWriteObserver* observer);

  // Stops accepting new data, clears all buffer, unregisters this stream from
  // |registry_| and make coming ReadRawData() calls return STREAM_ABORTED.
  void Abort();

  // Passes HTTP response information associated with the response body
  // transferred through this.
  void OnResponseStarted(const net::HttpResponseInfo& response_info);

  // Updates actual counts of bytes transferred by the network.
  void UpdateNetworkStats(int64_t raw_body_bytes, int64_t total_bytes);

  // Adds the data in |buffer| to the stream.  Takes ownership of |buffer|.
  void AddData(scoped_refptr<net::IOBuffer> buffer, size_t size);
  // Adds data of |size| at |data| to the stream. This method creates a copy
  // of the data, and then passes it to |writer_|.
  void AddData(const char* data, size_t size);

  // Flushes contents buffered in the stream to the corresponding reader.
  void Flush();

  // Notifies this stream that it will not be receiving any more data.
  void Finalize(int status);

  // Reads a maximum of |buf_size| from the stream into |buf|.  Sets
  // |*bytes_read| to the number of bytes actually read.
  // Returns STREAM_HAS_DATA if data was read, STREAM_EMPTY if no data was read,
  // and STREAM_COMPLETE if the stream is finalized and all data has been read.
  StreamState ReadRawData(net::IOBuffer* buf, int buf_size, int* bytes_read);

  std::unique_ptr<StreamHandle> CreateHandle();
  void CloseHandle();

  // Returns the status of the stream. This is either an error code that
  // occurred while reading, or the status that was set in Finalize above.
  int GetStatus();

  // Indicates whether there is space in the buffer to add more data.
  bool can_add_data() const { return can_add_data_; }

  const GURL& url() const { return url_; }

  // For StreamRegistry to remember the last memory usage reported to it.
  size_t last_total_buffered_bytes() const {
    return last_total_buffered_bytes_;
  }

  StreamMetadata* metadata() const { return metadata_.get(); }

 private:
  friend class base::RefCountedThreadSafe<Stream>;

  virtual ~Stream();

  void OnSpaceAvailable();
  void OnDataAvailable();

  // Clears |data_| and related variables.
  void ClearBuffer();

  bool can_add_data_;

  GURL url_;

  // Buffer for storing data read from |reader_| but not yet read out from this
  // Stream by ReadRawData() method.
  scoped_refptr<net::IOBuffer> data_;
  // Number of bytes read from |reader_| into |data_| including bytes already
  // read out.
  size_t data_length_;
  // Number of bytes in |data_| that are already read out.
  size_t data_bytes_read_;

  // Last value returned by writer_->TotalBufferedBytes() in AddData(). Stored
  // in order to check memory usage.
  size_t last_total_buffered_bytes_;

  std::unique_ptr<ByteStreamWriter> writer_;
  std::unique_ptr<ByteStreamReader> reader_;

  StreamRegistry* registry_;
  StreamReadObserver* read_observer_;
  StreamWriteObserver* write_observer_;

  StreamHandleImpl* stream_handle_;
  std::unique_ptr<StreamMetadata> metadata_;

  base::WeakPtrFactory<Stream> weak_ptr_factory_;
  DISALLOW_COPY_AND_ASSIGN(Stream);
};

}  // namespace content

#endif  // CONTENT_BROWSER_STREAMS_STREAM_H_
