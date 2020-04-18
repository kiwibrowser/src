// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// This is the browser side of the resource dispatcher, it receives requests
// from the RenderProcessHosts, and dispatches them to URLRequests. It then
// fowards the messages from the URLRequests back to the correct process for
// handling.
//
// See http://dev.chromium.org/developers/design-documents/multi-process-resource-loading

#ifndef CONTENT_BROWSER_LOADER_RESOURCE_HANDLER_H_
#define CONTENT_BROWSER_LOADER_RESOURCE_HANDLER_H_

#include <memory>
#include <string>

#include "base/compiler_specific.h"
#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "base/sequence_checker.h"
#include "content/browser/loader/resource_controller.h"
#include "content/common/content_export.h"

class GURL;

namespace net {
class IOBuffer;
class URLRequest;
class URLRequestStatus;
struct RedirectInfo;
}  // namespace net

namespace network {
struct ResourceResponse;
}

namespace content {
class ResourceMessageFilter;
class ResourceRequestInfoImpl;

// The resource dispatcher host uses this interface to process network events
// for an URLRequest instance.  A ResourceHandler's lifetime is bound to its
// associated URLRequest.
//
// No ResourceHandler method other than OnWillRead will ever be called
// synchronously when it calls into the ResourceController passed in to it,
// either to resume or cancel the request.
class CONTENT_EXPORT ResourceHandler {
 public:
  virtual ~ResourceHandler();

  // Used to allow a ResourceHandler to cancel the request out of band, when it
  // may not have a ResourceController.
  class CONTENT_EXPORT Delegate {
   protected:
    Delegate();
    virtual ~Delegate();

   private:
    friend class ResourceHandler;

    // Cancels the request when the class does not currently have ownership of
    // the ResourceController.
    // |error_code| indicates the reason for the cancellation, and
    // |tell_renderer| whether the renderer needs to be informed of the
    // cancellation.
    virtual void OutOfBandCancel(int error_code, bool tell_renderer) = 0;

    // Pauses/resumes reading response body if the resource is fetched from
    // network. They could be no-ops if the resource is not fetched from
    // network.
    //
    // It is allowed to call these methods before response body is available, or
    // while the request is deferred.
    virtual void PauseReadingBodyFromNet();
    virtual void ResumeReadingBodyFromNet();

    DISALLOW_COPY_AND_ASSIGN(Delegate);
  };

  // Sets the ResourceHandler::Delegate, which handles out-of-band cancellation.
  virtual void SetDelegate(Delegate* delegate);

  // The request was redirected to a new URL.  The request will not continue
  // until one of |controller|'s resume or cancellation methods is invoked.
  // |response| may be destroyed as soon as the method returns, so if the
  // ResourceHandler wants to continue to use it, it must maintain a reference
  // to it.
  virtual void OnRequestRedirected(
      const net::RedirectInfo& redirect_info,
      network::ResourceResponse* response,
      std::unique_ptr<ResourceController> controller) = 0;

  // Response headers and metadata are available.  The request will not continue
  // until one of |controller|'s resume or cancellation methods is invoked.
  // |response| may be destroyed as soon as the method returns, so if the
  // ResourceHandler wants to continue to use it, it must maintain a reference
  // to it.
  virtual void OnResponseStarted(
      network::ResourceResponse* response,
      std::unique_ptr<ResourceController> controller) = 0;

  // Called before the net::URLRequest (whose url is |url|) is to be started.
  // The request will not continue until one of |controller|'s resume or
  // cancellation methods is invoked.
  virtual void OnWillStart(const GURL& url,
                           std::unique_ptr<ResourceController> controller) = 0;

  // Data will be read for the response.  Upon success, this method places the
  // size and address of the buffer where the data is to be written in its
  // out-params.  This call will be followed by either OnReadCompleted (on
  // successful read or EOF) or OnResponseCompleted (on error).  If
  // OnReadCompleted is called, the buffer may be recycled.  Otherwise, it may
  // not be recycled and may potentially outlive the handler.
  //
  // Unlike other methods, may be called synchronously on Resume, for
  // performance reasons.
  //
  // The request will not continue until one of |controller|'s resume or
  // cancellation methods is invoked.
  virtual void OnWillRead(scoped_refptr<net::IOBuffer>* buf,
                          int* buf_size,
                          std::unique_ptr<ResourceController> controller) = 0;

  // Data (*bytes_read bytes) was written into the buffer provided by
  // OnWillRead.  The request will not continue until one of |controller|'s
  // resume or cancellation methods is invoked.  A zero |bytes_read| signals
  // that no further data will be received.
  virtual void OnReadCompleted(
      int bytes_read,
      std::unique_ptr<ResourceController> controller) = 0;

  // The response is complete.  The final response status is given.  The request
  // will not be deleted until controller's Resume() method is invoked. It is
  // illegal to use its cancellation methods.
  virtual void OnResponseCompleted(
      const net::URLRequestStatus& status,
      std::unique_ptr<ResourceController> controller) = 0;

  // This notification is synthesized by the RedirectToFileResourceHandler
  // to indicate progress of 'download_to_file' requests. OnReadCompleted
  // calls are consumed by the RedirectToFileResourceHandler and replaced
  // with OnDataDownloaded calls.
  virtual void OnDataDownloaded(int bytes_downloaded) = 0;

 protected:
  explicit ResourceHandler(net::URLRequest* request);

  // Convenience methods for managing a ResourceHandler's controller in the
  // async completion case. These ensure that the controller is nullptr after
  // being invoked, which allows for DCHECKing on it and better crashes on
  // calling into deleted objects.

  // Passes ownership of |controller| to |this|. Nothing is done with the
  // |controller| automatically.
  void HoldController(std::unique_ptr<ResourceController> controller);

  // Returns ownership of the ResourceController previously passed in to
  // HoldController.
  std::unique_ptr<ResourceController> ReleaseController();

  bool has_controller() const { return !!controller_; }

  // These call the corresponding methods on the ResourceController previously
  // passed to HoldController and then destroy it.
  void Resume();
  void Cancel();
  void CancelWithError(int error_code);

  // Cancels the request when the class does not currently have ownership of the
  // ResourceController.
  void OutOfBandCancel(int error_code, bool tell_renderer);
  void PauseReadingBodyFromNet();
  void ResumeReadingBodyFromNet();

  net::URLRequest* request() const { return request_; }

  // Convenience functions.
  static void GetNumericArg(const std::string& name, int* result);
  ResourceRequestInfoImpl* GetRequestInfo() const;
  int GetRequestID() const;
  ResourceMessageFilter* GetFilter() const;

  Delegate* delegate() { return delegate_; }

 private:
  net::URLRequest* request_;
  Delegate* delegate_;
  std::unique_ptr<ResourceController> controller_;

  SEQUENCE_CHECKER(sequence_checker_);

  DISALLOW_COPY_AND_ASSIGN(ResourceHandler);
};

}  // namespace content

#endif  // CONTENT_BROWSER_LOADER_RESOURCE_HANDLER_H_
