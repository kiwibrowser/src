// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_LOADER_INTERCEPTING_RESOURCE_HANDLER_H_
#define CONTENT_BROWSER_LOADER_INTERCEPTING_RESOURCE_HANDLER_H_

#include <memory>
#include <string>

#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "base/memory/weak_ptr.h"
#include "content/browser/loader/layered_resource_handler.h"
#include "content/browser/loader/resource_controller.h"
#include "content/browser/loader/resource_handler.h"
#include "content/common/content_export.h"
#include "net/base/io_buffer.h"
#include "net/url_request/url_request_status.h"

namespace net {
class URLRequest;
}

namespace content {

class ResourceController;

// ResourceHandler that initiates special handling of the response if needed,
// based on the response's MIME type (starts downloads, sends data to some
// plugin types via a special channel).
//
// An InterceptingResourceHandler holds two handlers (|next_handler| and
// |new_handler|). It assumes the following:
//  - OnResponseStarted on |next_handler| never sets |*defer|.
//  - OnResponseCompleted on |next_handler| never sets |*defer|.
class CONTENT_EXPORT InterceptingResourceHandler
    : public LayeredResourceHandler {
 public:
  InterceptingResourceHandler(std::unique_ptr<ResourceHandler> next_handler,
                              net::URLRequest* request);
  ~InterceptingResourceHandler() override;

  // ResourceHandler implementation:
  void OnResponseStarted(
      network::ResourceResponse* response,
      std::unique_ptr<ResourceController> controller) override;
  void OnWillRead(scoped_refptr<net::IOBuffer>* buf,
                  int* buf_size,
                  std::unique_ptr<ResourceController> controller) override;
  void OnReadCompleted(int bytes_read,
                       std::unique_ptr<ResourceController> controller) override;
  void OnResponseCompleted(
      const net::URLRequestStatus& status,
      std::unique_ptr<ResourceController> controller) override;

  // Replaces the next handler with |new_handler|, sending
  // |payload_for_old_handler| to the old handler. Must be called after
  // OnWillStart and OnRequestRedirected and before OnResponseStarted. One
  // OnWillRead call is permitted beforehand. |new_handler|'s OnWillStart and
  // OnRequestRedirected methods will not be called.
  void UseNewHandler(std::unique_ptr<ResourceHandler> new_handler,
                     const std::string& payload_for_old_handler);

  // Used in tests.
  ResourceHandler* new_handler_for_testing() const {
    return new_handler_.get();
  }

 private:
  // ResourceController subclass that calls into the InterceptingResourceHandler
  // on cancel/resume.
  class Controller;

  enum class State {
    // The InterceptingResourceHandler is waiting for the mime type of the
    // response to be identified, to check if the next handler should be
    // replaced with an appropriate one.
    STARTING,

    // The InterceptingResourceHandler is starting the process of swapping
    // handlers.
    SWAPPING_HANDLERS,

    // States where the InterceptingResourceHandler passes the initial
    // OnWillRead call to the old handler, and then waits for the resulting
    // buffer read buffer.
    SENDING_ON_WILL_READ_TO_OLD_HANDLER,
    WAITING_FOR_OLD_HANDLERS_BUFFER,

    // The InterceptingResourceHandler is sending the payload given via
    // UseNewHandler to the old handler. The first state starts retrieving a
    // buffer from the old handler, the second state copies as much of the data
    // as possible to the received buffer and passes it to the old handler.
    SENDING_PAYLOAD_TO_OLD_HANDLER,
    RECEIVING_BUFFER_FROM_OLD_HANDLER,

    // The InterceptingResourcHandler is calling the new handler's
    // OnResponseStarted method and waiting for its completion via Resume().
    // After completion, the InterceptingResourceHandler will transition to
    // SENDING_ON_RESPONSE_STARTED_TO_NEW_HANDLER on success.
    SENDING_ON_WILL_START_TO_NEW_HANDLER,

    // The InterceptingResourcHandler is calling the new handler's
    // OnResponseStarted method and waiting for its completion via Resume().
    // After completion, the InterceptingResourceHandler will transition to
    // WAITING_FOR_ON_READ_COMPLETED on success.
    SENDING_ON_RESPONSE_STARTED_TO_NEW_HANDLER,

    // The InterceptingResourcHandler is waiting for OnReadCompleted to be
    // called.
    WAITING_FOR_ON_READ_COMPLETED,

    // The two phases of uploading previously received data stored in
    // |first_read_buffer_double_| to the new handler, which is now stored in
    // |next_handler_|. The first state gets a buffer to write to, and the next
    // copies all the data it can to that buffer.
    SENDING_BUFFER_TO_NEW_HANDLER,
    SENDING_BUFFER_TO_NEW_HANDLER_WAITING_FOR_BUFFER,

    // The InterceptingResourceHandler has replaced its next ResourceHandler if
    // needed, and has ensured the buffered read data was properly transmitted
    // to the new ResourceHandler. The InterceptingResourceHandler now acts as
    // a pass-through ResourceHandler.
    PASS_THROUGH,
  };

  // Runs necessary operations depending on |state_|.
  void DoLoop();

  void ResumeInternal();

  void SendOnWillReadToOldHandler();
  void OnBufferReceived();
  void SendOnResponseStartedToOldHandler();
  void SendPayloadToOldHandler();
  void ReceivedBufferFromOldHandler();
  void SendFirstReadBufferToNewHandler();
  void SendOnResponseStartedToNewHandler();
  void ReceivedBufferFromNewHandler();

  State state_ = State::STARTING;

  std::unique_ptr<ResourceHandler> new_handler_;
  std::string payload_for_old_handler_;
  size_t payload_bytes_written_ = 0;

  // Result of the first read, that may have to be passed to an alternate
  // ResourceHandler instead of the original ResourceHandler.
  scoped_refptr<net::IOBuffer> first_read_buffer_;
  // Instead of |first_read_buffer_|, this handler creates a new IOBuffer with
  // the same size and return it to the client.
  scoped_refptr<net::IOBuffer> first_read_buffer_double_;
  int first_read_buffer_size_ = 0;
  int first_read_buffer_bytes_read_ = 0;
  int first_read_buffer_bytes_written_ = 0;

  // Information about the new handler's buffer while copying data from
  // |first_read_buffer_double_| to the new handler's buffer.
  // Note that when these are used, the old handler has been destroyed, and
  // |next_handler_| is now the new one.
  scoped_refptr<net::IOBuffer> new_handler_read_buffer_;
  int new_handler_read_buffer_size_ = 0;

  // Pointers to parent-owned read buffer and its size.  Only used for first
  // OnWillRead call.
  scoped_refptr<net::IOBuffer>* parent_read_buffer_ = nullptr;
  int* parent_read_buffer_size_ = nullptr;

  scoped_refptr<network::ResourceResponse> response_;

  // Next two values are used to handle synchronous Resume calls without a
  // PostTask.

  // True if the code is currently within a DoLoop.
  bool in_do_loop_ = false;
  // True if the request was resumed while |in_do_loop_| was true;
  bool advance_to_next_state_ = false;

  base::WeakPtrFactory<InterceptingResourceHandler> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(InterceptingResourceHandler);
};

}  // namespace content

#endif  // CONTENT_BROWSER_LOADER_INTERCEPTING_RESOURCE_HANDLER_H_
