// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_LOADER_REDIRECT_TO_FILE_RESOURCE_HANDLER_H_
#define CONTENT_BROWSER_LOADER_REDIRECT_TO_FILE_RESOURCE_HANDLER_H_

#include <memory>

#include "base/callback.h"
#include "base/compiler_specific.h"
#include "base/files/file.h"
#include "base/files/file_path.h"
#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "base/memory/weak_ptr.h"
#include "content/browser/loader/layered_resource_handler.h"
#include "content/browser/loader/temporary_file_stream.h"
#include "content/common/content_export.h"
#include "net/url_request/url_request.h"
#include "net/url_request/url_request_status.h"
#include "url/gurl.h"

namespace net {
class FileStream;
class GrowableIOBuffer;
}

namespace storage {
class ShareableFileReference;
}

namespace content {

class ResourceController;

// Redirects network data to a file.  This is intended to be layered in front of
// either the AsyncResourceHandler or the SyncResourceHandler.  The downstream
// resource handler does not see OnWillRead or OnReadCompleted calls. Instead,
// the ResourceResponse contains the path to a temporary file and
// OnDataDownloaded is called as the file downloads.
class CONTENT_EXPORT RedirectToFileResourceHandler
    : public LayeredResourceHandler {
 public:
  // Exposed for testing.
  static const int kInitialReadBufSize;
  static const int kMaxReadBufSize;

  typedef base::Callback<void(const CreateTemporaryFileStreamCallback&)>
      CreateTemporaryFileStreamFunction;

  // Create a RedirectToFileResourceHandler for |request| which wraps
  // |next_handler|.
  RedirectToFileResourceHandler(std::unique_ptr<ResourceHandler> next_handler,
                                net::URLRequest* request);
  ~RedirectToFileResourceHandler() override;

  // Replace the CreateTemporaryFileStream implementation with a mocked one for
  // testing purposes. The function should create a net::FileStream and a
  // ShareableFileReference and then asynchronously pass them to the
  // CreateTemporaryFileStreamCallback.
  void SetCreateTemporaryFileStreamFunctionForTesting(
      const CreateTemporaryFileStreamFunction& create_temporary_file_stream);

  // LayeredResourceHandler implementation:
  void OnResponseStarted(
      network::ResourceResponse* response,
      std::unique_ptr<ResourceController> controller) override;
  void OnWillStart(const GURL& url,
                   std::unique_ptr<ResourceController> controller) override;
  void OnWillRead(scoped_refptr<net::IOBuffer>* buf,
                  int* buf_size,
                  std::unique_ptr<ResourceController> controller) override;
  void OnReadCompleted(int bytes_read,
                       std::unique_ptr<ResourceController> controller) override;
  void OnResponseCompleted(
      const net::URLRequestStatus& status,
      std::unique_ptr<ResourceController> controller) override;

  // Returns the size of |buf_|, to make sure it's being increased as expected.
  int GetBufferSizeForTesting() const;

 private:
  void DidCreateTemporaryFile(base::File::Error error_code,
                              std::unique_ptr<net::FileStream> file_stream,
                              storage::ShareableFileReference* deletable_file);

  // Called by RedirectToFileResourceHandler::Writer.
  void DidWriteToFile(int result);

  // Attempts to write more data to the file, if possible. Returns false on
  // error. Returns true if there's already a write in progress, all data was
  // written successfully, or a new write was started that will complete
  // asynchronously. Resumes the request if there's more data to read and more
  // buffer space available.
  bool WriteMore();

  bool BufIsFull() const;

  // If populated, OnResponseStarted completion is pending on file creation.
  scoped_refptr<network::ResourceResponse> response_pending_file_creation_;
  CreateTemporaryFileStreamFunction create_temporary_file_stream_;

  // We allocate a single, fixed-size IO buffer (buf_) used to read from the
  // network (buf_write_pending_ is true while the system is copying data into
  // buf_), and then write this buffer out to disk (write_callback_pending_ is
  // true while writing to disk).  Reading from the network is suspended while
  // the buffer is full (BufIsFull returns true).  The write_cursor_ member
  // tracks the offset into buf_ that we are writing to disk.

  scoped_refptr<net::GrowableIOBuffer> buf_;
  bool buf_write_pending_ = false;
  int write_cursor_ = 0;

  // Helper writer object which maintains references to the net::FileStream and
  // storage::ShareableFileReference. This is maintained separately so that,
  // on Windows, the temporary file isn't deleted until after it is closed.
  class Writer;
  Writer* writer_ = nullptr;

  // |next_buffer_size_| is the size of the buffer to be allocated on the next
  // OnWillRead() call.  We exponentially grow the size of the buffer allocated
  // when our owner fills our buffers. On the first OnWillRead() call, we
  // allocate a buffer of 32k and double it in OnReadCompleted() if the buffer
  // was filled, up to a maximum size of 512k.
  int next_buffer_size_ = kInitialReadBufSize;

  bool completed_during_write_ = false;
  net::URLRequestStatus completed_status_;

  base::WeakPtrFactory<RedirectToFileResourceHandler> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(RedirectToFileResourceHandler);
};

}  // namespace content

#endif  // CONTENT_BROWSER_LOADER_REDIRECT_TO_FILE_RESOURCE_HANDLER_H_
