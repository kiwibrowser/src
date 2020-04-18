// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_LOADER_MOCK_RESOURCE_LOADER_H_
#define CONTENT_BROWSER_LOADER_MOCK_RESOURCE_LOADER_H_

#include <memory>
#include <string>

#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "base/memory/weak_ptr.h"
#include "base/run_loop.h"
#include "base/strings/string_piece.h"
#include "content/browser/loader/resource_controller.h"
#include "content/browser/loader/resource_handler.h"
#include "net/base/io_buffer.h"
#include "net/base/net_errors.h"

class GURL;

namespace net {
struct RedirectInfo;
class URLRequestStatus;
}

namespace network {
struct ResourceResponse;
}

namespace content {

// Class that takes the place of the ResourceLoader for tests. It simplifies
// testing ResourceHandlers by managing callbacks and performing basic sanity
// checks. The test fixture is responsible for advancing states.
class MockResourceLoader : public ResourceHandler::Delegate {
 public:
  explicit MockResourceLoader(ResourceHandler* resource_handler);
  ~MockResourceLoader() override;

  // Idle means the ResourceHandler is waiting for the next call from the
  // TestFixture/ResourceLoader, CALLBACK_PENDING means that the ResourceHandler
  // will resume the request at some future point to resume the request, and
  // CANCELED means the ResourceHandler cancelled the request.
  enum class Status {
    // The MockLoader is waiting for more data from hte test fixture.
    IDLE,
    // The MockLoader is currently in the middle of a call to a handler. Will
    // switch to CALLBACK_PENDING if the handler defers handling the request.
    CALLING_HANDLER,
    // The MockLoader is waiting for a callback from the ResourceHandler.
    CALLBACK_PENDING,
    // The request was cancelled.
    CANCELED,
  };

  // These all run the corresponding methods on ResourceHandler, along with
  // basic sanity checks for the behavior of the handler. Each check returns the
  // current status of the ResourceLoader.
  Status OnWillStart(const GURL& url);
  Status OnRequestRedirected(const net::RedirectInfo& redirect_info,
                             scoped_refptr<network::ResourceResponse> response);
  Status OnResponseStarted(scoped_refptr<network::ResourceResponse> response);
  Status OnWillRead();
  Status OnReadCompleted(base::StringPiece bytes);
  Status OnResponseCompleted(const net::URLRequestStatus& status);

  // Simulates an out-of-band cancel from some source other than the
  // ResourceHandler |this| was created with (like another ResourceHandler). The
  // difference between this and OnResponseCompleted() is that this has fewer
  // sanity checks to validate the cancel was in-band.
  Status OnResponseCompletedFromExternalOutOfBandCancel(
      const net::URLRequestStatus& url_request_status);

  // Waits until status() is IDLE or CANCELED.  If that's already the case, does
  // nothing.
  void WaitUntilIdleOrCanceled();

  Status status() const { return status_; }

  // Network error passed to the first CancelWithError() / Cancel() call, which
  // is the one the real code uses in the case of multiple cancels.
  int error_code() const { return error_code_; }

  // Returns IOBuffer returned by last call to OnWillRead. The IOBuffer is
  // release on read complete, on response complete, and on cancel.
  net::IOBuffer* io_buffer() const { return io_buffer_.get(); }
  int io_buffer_size() const { return io_buffer_size_; }

 private:
  class TestResourceController;

  // ResourceHandler::Delegate implementation:
  void OutOfBandCancel(int error_code, bool tell_renderer) override;

  void OnCancel(int error_code);
  void OnResume();

  ResourceHandler* const resource_handler_;

  Status status_ = Status::IDLE;
  int error_code_ = net::OK;
  bool canceled_out_of_band_ = false;

  scoped_refptr<net::IOBuffer> io_buffer_;
  int io_buffer_size_ = 0;

  // True if waiting to receive a buffer due to an OnWillRead call. This does
  // not affect behavior; it's only used for DCHECKing.
  bool waiting_on_buffer_ = false;

  std::unique_ptr<base::RunLoop> canceled_or_idle_run_loop_;

  base::WeakPtrFactory<MockResourceLoader> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(MockResourceLoader);
};

}  // namespace content

#endif  // CONTENT_BROWSER_LOADER_TEST_RESOURCE_HANDLER_WRAPPER_H_
