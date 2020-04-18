// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/loader/redirect_to_file_resource_handler.h"

#include <utility>

#include "base/bind.h"
#include "base/logging.h"
#include "base/macros.h"
#include "base/threading/thread_restrictions.h"
#include "content/browser/loader/resource_controller.h"
#include "content/browser/loader/resource_request_info_impl.h"
#include "content/browser/loader/temporary_file_stream.h"
#include "net/base/file_stream.h"
#include "net/base/io_buffer.h"
#include "net/base/mime_sniffer.h"
#include "net/base/net_errors.h"
#include "services/network/public/cpp/resource_response.h"
#include "storage/browser/blob/shareable_file_reference.h"

using storage::ShareableFileReference;

namespace {

// This class is similar to identically named classes in AsyncResourceHandler
// and MimeTypeResourceHandler, but not quite.
// TODO(ncbray): generalize and unify these cases?
// In general, it's a bad idea to point to a subbuffer (particularly with
// GrowableIOBuffer) because the backing IOBuffer may realloc its data.  In this
// particular case we know RedirectToFileResourceHandler will not realloc its
// buffer while a write is occurring, so we should be safe.  This property is
// somewhat fragile, however, and depending on it is dangerous.  A more
// principled approach would require significant refactoring, however, so for
// the moment we're relying on fragile properties.
class DependentIOBufferForRedirectToFile : public net::WrappedIOBuffer {
 public:
  DependentIOBufferForRedirectToFile(net::IOBuffer* backing, char* memory)
      : net::WrappedIOBuffer(memory), backing_(backing) {}

 private:
  ~DependentIOBufferForRedirectToFile() override {}

  scoped_refptr<net::IOBuffer> backing_;
};

}  // namespace

namespace content {

const int RedirectToFileResourceHandler::kInitialReadBufSize = 32768;
const int RedirectToFileResourceHandler::kMaxReadBufSize = 524288;

// A separate IO thread object to manage the lifetime of the net::FileStream and
// the ShareableFileReference. When the handler is destroyed, it asynchronously
// closes net::FileStream after all pending writes complete. Only after the
// stream is closed is the ShareableFileReference released, to ensure the
// temporary is not deleted before it is closed.
class RedirectToFileResourceHandler::Writer {
 public:
  Writer(RedirectToFileResourceHandler* handler,
         std::unique_ptr<net::FileStream> file_stream,
         ShareableFileReference* deletable_file)
      : handler_(handler),
        file_stream_(std::move(file_stream)),
        is_writing_(false),
        deletable_file_(deletable_file) {
    DCHECK(!deletable_file_->path().empty());
  }

  bool is_writing() const { return is_writing_; }
  const base::FilePath& path() const { return deletable_file_->path(); }

  int Write(net::IOBuffer* buf, int buf_len) {
    DCHECK(!is_writing_);
    DCHECK(handler_);
    int result = file_stream_->Write(
        buf, buf_len,
        base::BindOnce(&Writer::DidWriteToFile, base::Unretained(this)));
    if (result == net::ERR_IO_PENDING)
      is_writing_ = true;
    return result;
  }

  void Close() {
    handler_ = nullptr;
    if (!is_writing_)
      CloseAndDelete();
  }

 private:
  // Only DidClose can delete this.
  ~Writer() {
  }

  void DidWriteToFile(int result) {
    DCHECK(is_writing_);
    is_writing_ = false;
    if (handler_) {
      handler_->DidWriteToFile(result);
    } else {
      CloseAndDelete();
    }
  }

  void CloseAndDelete() {
    DCHECK(!is_writing_);
    int result = file_stream_->Close(
        base::BindOnce(&Writer::DidClose, base::Unretained(this)));
    if (result != net::ERR_IO_PENDING)
      DidClose(result);
  }

  void DidClose(int result) {
    delete this;
  }

  RedirectToFileResourceHandler* handler_;

  std::unique_ptr<net::FileStream> file_stream_;
  bool is_writing_;

  // We create a ShareableFileReference that's deletable for the temp file
  // created as a result of the download.
  scoped_refptr<storage::ShareableFileReference> deletable_file_;

  DISALLOW_COPY_AND_ASSIGN(Writer);
};

RedirectToFileResourceHandler::RedirectToFileResourceHandler(
    std::unique_ptr<ResourceHandler> next_handler,
    net::URLRequest* request)
    : LayeredResourceHandler(request, std::move(next_handler)),
      buf_(new net::GrowableIOBuffer()),
      weak_factory_(this) {}

RedirectToFileResourceHandler::~RedirectToFileResourceHandler() {
  // Orphan the writer to asynchronously close and release the temporary file.
  if (writer_) {
    writer_->Close();
    writer_ = nullptr;
  }
}

void RedirectToFileResourceHandler::
    SetCreateTemporaryFileStreamFunctionForTesting(
        const CreateTemporaryFileStreamFunction& create_temporary_file_stream) {
  create_temporary_file_stream_ = create_temporary_file_stream;
}

void RedirectToFileResourceHandler::OnResponseStarted(
    network::ResourceResponse* response,
    std::unique_ptr<ResourceController> controller) {
  if (!writer_) {
    response_pending_file_creation_ = response;
    HoldController(std::move(controller));
    request()->LogBlockedBy("RedirectToFileResourceHandler");
    return;
  }
  response->head.download_file_path = writer_->path();
  next_handler_->OnResponseStarted(response, std::move(controller));
}

void RedirectToFileResourceHandler::OnWillStart(
    const GURL& url,
    std::unique_ptr<ResourceController> controller) {
  DCHECK(!writer_);

  // Create the file ASAP but don't block.
  if (create_temporary_file_stream_.is_null()) {
    CreateTemporaryFileStream(
        base::Bind(&RedirectToFileResourceHandler::DidCreateTemporaryFile,
                   weak_factory_.GetWeakPtr()));
  } else {
    create_temporary_file_stream_.Run(
        base::Bind(&RedirectToFileResourceHandler::DidCreateTemporaryFile,
                   weak_factory_.GetWeakPtr()));
  }
  next_handler_->OnWillStart(url, std::move(controller));
}

void RedirectToFileResourceHandler::OnWillRead(
    scoped_refptr<net::IOBuffer>* buf,
    int* buf_size,
    std::unique_ptr<ResourceController> controller) {
  if (buf_->capacity() < next_buffer_size_)
    buf_->SetCapacity(next_buffer_size_);

  // We should have paused this network request already if the buffer is full.
  DCHECK(!BufIsFull());

  *buf = buf_.get();
  *buf_size = buf_->RemainingCapacity();

  buf_write_pending_ = true;
  controller->Resume();
}

void RedirectToFileResourceHandler::OnReadCompleted(
    int bytes_read,
    std::unique_ptr<ResourceController> controller) {
  DCHECK(buf_write_pending_);
  buf_write_pending_ = false;

  // We use the buffer's offset field to record the end of the buffer.
  int new_offset = buf_->offset() + bytes_read;
  DCHECK(new_offset <= buf_->capacity());
  buf_->set_offset(new_offset);

  if (buf_->capacity() == bytes_read) {
    // The network layer has saturated our buffer in one read. Next time, we
    // should give it a bigger buffer for it to fill.
    next_buffer_size_ = std::min(next_buffer_size_ * 2, kMaxReadBufSize);
  }

  HoldController(std::move(controller));
  // WriteMore will resume the request if there's more buffer space.
  if (!WriteMore()) {
    CancelWithError(net::ERR_FAILED);
    return;
  }

  if (has_controller())
    request()->LogBlockedBy("RedirectToFileResourceHandler");
}

void RedirectToFileResourceHandler::OnResponseCompleted(
    const net::URLRequestStatus& status,
    std::unique_ptr<ResourceController> controller) {
  if (writer_ && writer_->is_writing()) {
    completed_during_write_ = true;
    completed_status_ = status;
    HoldController(std::move(controller));
    request()->LogBlockedBy("RedirectToFileResourceHandler");
    return;
  }
  next_handler_->OnResponseCompleted(status, std::move(controller));
}

int RedirectToFileResourceHandler::GetBufferSizeForTesting() const {
  return buf_->capacity();
}

void RedirectToFileResourceHandler::DidCreateTemporaryFile(
    base::File::Error error_code,
    std::unique_ptr<net::FileStream> file_stream,
    ShareableFileReference* deletable_file) {
  DCHECK(!writer_);
  if (error_code != base::File::FILE_OK) {
    if (has_controller()) {
      CancelWithError(net::FileErrorToNetError(error_code));
    } else {
      OutOfBandCancel(net::FileErrorToNetError(error_code),
                      true /* tell_renderer */);
    }
    return;
  }

  writer_ = new Writer(this, std::move(file_stream), deletable_file);

  if (response_pending_file_creation_) {
    scoped_refptr<network::ResourceResponse> response =
        std::move(response_pending_file_creation_);
    request()->LogUnblocked();
    OnResponseStarted(response.get(), ReleaseController());
  }
}

void RedirectToFileResourceHandler::DidWriteToFile(int result) {
  bool failed = false;
  if (result > 0) {
    OnDataDownloaded(result);
    write_cursor_ += result;
    // WriteMore will resume the request if the request hasn't completed and
    // there's more buffer space.
    failed = !WriteMore();
  } else {
    failed = true;
  }

  if (failed) {
    DCHECK(!writer_->is_writing());
    // TODO(davidben): Recover the error code from WriteMore or |result|, as
    // appropriate.
    if (completed_during_write_ && completed_status_.is_success()) {
      // If the request successfully completed mid-write, but the write failed,
      // convert the status to a failure for downstream.
      completed_status_ = net::URLRequestStatus(net::URLRequestStatus::CANCELED,
                                                net::ERR_FAILED);
    }
    if (!completed_during_write_) {
      if (has_controller()) {
        // If the write buffer is full, |this| has deferred the request, and
        // can do an in-band cancel.
        CancelWithError(net::ERR_FAILED);
      } else {
        OutOfBandCancel(net::ERR_FAILED, true /* tell_renderer */);
      }
      return;
    }
  }

  if (completed_during_write_ && !writer_->is_writing()) {
    // Resume shutdown now that all data has been written to disk. Note that
    // this should run even in the |failed| case above, otherwise a failed write
    // leaves the handler stuck.
    DCHECK(has_controller());
    request()->LogUnblocked();
    next_handler_->OnResponseCompleted(completed_status_, ReleaseController());
  }
}

bool RedirectToFileResourceHandler::WriteMore() {
  DCHECK(writer_);

  for (;;) {
    if (write_cursor_ == buf_->offset()) {
      // We've caught up to the network load, but it may be in the process of
      // appending more data to the buffer.
      if (!buf_write_pending_) {
        buf_->set_offset(0);
        write_cursor_ = 0;
      }
      break;
    }
    if (writer_->is_writing())
      break;
    DCHECK(write_cursor_ < buf_->offset());

    // Create a temporary buffer pointing to a subsection of the data buffer so
    // that it can be passed to Write.  This code makes some crazy scary
    // assumptions about object lifetimes, thread sharing, and that buf_ will
    // not realloc durring the write due to how the state machine in this class
    // works.
    // Note that buf_ is also shared with the code that writes data into the
    // cache, so modifying it can cause some pretty subtle race conditions:
    // https://code.google.com/p/chromium/issues/detail?id=152076
    // We're using DependentIOBuffer instead of DrainableIOBuffer to dodge some
    // of these issues, for the moment.
    // TODO(ncbray) make this code less crazy scary.
    // Also note that Write may increase the refcount of "wrapped" deep in the
    // bowels of its implementation, the use of scoped_refptr here is not
    // spurious.
    scoped_refptr<DependentIOBufferForRedirectToFile> wrapped =
        new DependentIOBufferForRedirectToFile(
            buf_.get(), buf_->StartOfBuffer() + write_cursor_);
    int write_len = buf_->offset() - write_cursor_;

    int rv = writer_->Write(wrapped.get(), write_len);
    if (rv == net::ERR_IO_PENDING)
      break;
    if (rv <= 0)
      return false;
    OnDataDownloaded(rv);
    write_cursor_ += rv;
  }

  // If the request was defered for a reason other than having been completed,
  // and the buffer has space, resume the request.
  if (has_controller() && !completed_during_write_ && !BufIsFull()) {
    request()->LogUnblocked();
    Resume();
  }
  return true;
}

bool RedirectToFileResourceHandler::BufIsFull() const {
  // This is a hack to workaround MimeTypeResourceHandler's inability to
  // deal with a ResourceHandler that returns a buffer size of less than
  // 2 * net::kMaxBytesToSniff from its OnWillRead method.
  // TODO(darin): Fix this retardation!
  return buf_->RemainingCapacity() <= (2 * net::kMaxBytesToSniff);
}

}  // namespace content
