// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_LOADER_MIME_SNIFFING_RESOURCE_HANDLER_H_
#define CONTENT_BROWSER_LOADER_MIME_SNIFFING_RESOURCE_HANDLER_H_

#include <string>
#include <vector>

#include "base/auto_reset.h"
#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "content/browser/loader/layered_resource_handler.h"
#include "content/browser/loader/resource_controller.h"
#include "content/common/content_export.h"
#include "content/public/common/request_context_type.h"
#include "ppapi/buildflags/buildflags.h"

namespace net {
class URLRequest;
}

namespace content {
class InterceptingResourceHandler;
class PluginService;
class ResourceController;
class ResourceDispatcherHostImpl;
struct WebPluginInfo;

// ResourceHandler that, if necessary, buffers a response body without passing
// it to the next ResourceHandler until it can perform mime sniffing on it.
//
// Uses the buffer provided by the original event handler for buffering, and
// continues to reuses it until it can determine the MIME type
// subsequent reads until it's done buffering.  As a result, the buffer
// returned by the next ResourceHandler must have a capacity of at least
// net::kMaxBytesToSniff * 2.
//
// Before a request is sent, this ResourceHandler will also set an appropriate
// Accept header on the request based on its ResourceType, if one isn't already
// present.
class CONTENT_EXPORT MimeSniffingResourceHandler
    : public LayeredResourceHandler {
 public:
  MimeSniffingResourceHandler(std::unique_ptr<ResourceHandler> next_handler,
                              ResourceDispatcherHostImpl* host,
                              PluginService* plugin_service,
                              InterceptingResourceHandler* intercepting_handler,
                              net::URLRequest* request,
                              RequestContextType request_context_type);
  ~MimeSniffingResourceHandler() override;

 private:
  class Controller;

  friend class MimeSniffingResourceHandlerTest;
  enum State {
    // Starting state of the MimeSniffingResourceHandler. In this state, it is
    // acting as a blind pass-through ResourceHandler until the response is
    // received.
    STATE_STARTING,

    // In this state, the MimeSniffingResourceHandler is buffering the response
    // data in read_buffer_, waiting to sniff the mime type and make a choice
    // about request interception.
    STATE_BUFFERING,

    // In these states, the MimeSniffingResourceHandler is calling OnWillRead on
    // the downstream ResourceHandler and then waiting for the response.
    STATE_CALLING_ON_WILL_READ,
    STATE_WAITING_FOR_BUFFER,

    // In this state, the MimeSniffingResourceHandler has identified the mime
    // type and made a decision on whether the request should be intercepted or
    // not. It is nows attempting to replay the response to downstream
    // handlers.
    STATE_INTERCEPTION_CHECK_DONE,

    // In this state, the MimeSniffingResourceHandler is replaying the buffered
    // OnResponseStarted event to the downstream ResourceHandlers.
    STATE_REPLAYING_RESPONSE_RECEIVED,

    // In these states, the MimeSniffingResourceHandler is replaying the pair of
    // OnWillRead + OnReadCompleted(0) calls that indicates end of the response
    // body.  See also |need_to_replay_extra_eof_packet_|.
    STATE_REPLAYING_EOF_WILL_READ,
    STATE_REPLAYING_EOF_READ_COMPLETED,

    // In this state, the MimeSniffingResourceHandler is just a blind
    // pass-through
    // ResourceHandler.
    STATE_STREAMING,
  };

  // ResourceHandler implementation:
  void OnWillStart(const GURL&,
                   std::unique_ptr<ResourceController> controller) override;
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

  void ResumeInternal();

  // --------------------------------------------------------------------------
  // The following methods replay the buffered data to the downstream
  // ResourceHandlers. They return false if the request should be cancelled,
  // true otherwise. Each of them will set |defer| to true if the request will
  // proceed to the next stage asynchronously.

  // Used to advance through the states of the state machine.
  void AdvanceState();

  // Intercepts the request as a stream/download if needed.
  void MaybeIntercept();

  // Calls OnWillRead on the downstream handlers.
  void CallOnWillRead();

  // Copies received buffer to parent.
  void BufferReceived();

  // Replays OnResponseStarted on the downstream handlers.
  void ReplayResponseReceived();

  // Replays OnReadCompleted on the downstreams handlers.
  void ReplayReadCompleted();

  // Replays OnWillRead if needed to notify the downstream handler about EOF.
  void ReplayWillReadEof();

  // Replays OnReadCompleted(0) if needed to notify the downstream handler about
  // EOF.
  void ReplayReadCompletedEof();

  // --------------------------------------------------------------------------

  // Checks whether this request should be intercepted as a stream or a
  // download. If this is the case, sets up the new ResourceHandler that will be
  // used for interception.
  //
  // Returns true on synchronous success, false if the operation will need to
  // complete asynchronously or failure. On failure, also cancels the request.
  bool MaybeStartInterception();

  // Determines whether a plugin will handle the current request. Returns false
  // if there is an error and the request should be cancelled and true
  // otherwise. If the request is directed to a plugin, |handled_by_plugin| is
  // set to true.
  //
  // Returns true on synchronous success, false if the operation will need to
  // complete asynchronously or failure. On failure, also cancels the request.
  bool CheckForPluginHandler(bool* handled_by_plugin);

  // Whether this request is allowed to be intercepted as a download or a
  // stream.
  bool CanBeIntercepted();

  // Whether the response we received is not provisional.
  bool CheckResponseIsNotProvisional();

  bool MustDownload();

  // Called on the IO thread once the list of plugins has been loaded.
  void OnPluginsLoaded(const std::vector<WebPluginInfo>& plugins);

  State state_;

  ResourceDispatcherHostImpl* host_;
#if BUILDFLAG(ENABLE_PLUGINS)
  PluginService* plugin_service_;
#endif

  bool must_download_;
  bool must_download_is_set_;

  // Used to buffer the response received until replay.
  scoped_refptr<network::ResourceResponse> response_;
  scoped_refptr<net::IOBuffer> read_buffer_;
  int read_buffer_size_;
  int bytes_read_;
  bool need_to_replay_extra_eof_packet_;

  // Pointers to parent-owned read buffer and its size.  Only used for first
  // OnWillRead call.
  scoped_refptr<net::IOBuffer>* parent_read_buffer_;
  int* parent_read_buffer_size_;

  // The InterceptingResourceHandler that will perform ResourceHandler swap if
  // needed.
  InterceptingResourceHandler* intercepting_handler_;

  RequestContextType request_context_type_;

  // True if current in an AdvanceState loop. Used to prevent re-entrancy and
  // avoid an extra PostTask.
  bool in_state_loop_;
  // Set to true if Resume() is called while |in_state_loop_| is true. When
  // returning to the parent AdvanceState loop, will synchronously advance to
  // the next state when control returns to the AdvanceState loop.
  bool advance_state_;

  base::WeakPtrFactory<MimeSniffingResourceHandler> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(MimeSniffingResourceHandler);
};

}  // namespace content

#endif  // CONTENT_BROWSER_LOADER_MIME_SNIFFING_RESOURCE_HANDLER_H_
