// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_LOADER_CROSS_SITE_DOCUMENT_RESOURCE_HANDLER_H_
#define CONTENT_BROWSER_LOADER_CROSS_SITE_DOCUMENT_RESOURCE_HANDLER_H_

#include <memory>
#include <vector>

#include "base/gtest_prod_util.h"
#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "content/browser/loader/layered_resource_handler.h"
#include "content/browser/loader/resource_request_info_impl.h"
#include "content/public/browser/resource_request_info.h"
#include "content/public/common/resource_type.h"
#include "services/network/cross_origin_read_blocking.h"

namespace net {
class URLRequest;
}  // namespace net

namespace content {

// A ResourceHandler that prevents the renderer process from receiving network
// responses that contain cross-site documents (HTML, XML, some plain text) or
// similar data that should be opaque (JSON), with appropriate exceptions to
// preserve compatibility.  Other cross-site resources such as scripts, images,
// stylesheets, etc are still allowed.
//
// This handler is not used for navigations, which create a new security context
// based on the origin of the response.  It currently only protects documents
// from sites that require dedicated renderer processes, though it could be
// expanded to apply to all sites.
//
// When a response is blocked, the renderer is sent an empty response body (with
// stripped down set of response headers) instead of seeing a failed request.  A
// failed request would change page- visible behavior (e.g., for a blocked XHR).
// An empty response can generally be consumed by the renderer without noticing
// the difference.
//
// To allow stripping response headers of a blocked response,
// CrossSiteDocumentResourceHandler holds onto ResourceResponse received in
// OnResponseStarted and replays it only after making the final block-or-allow
// decision (this may require sniffing - processing the first few OnWillRead
// and OnReadCompleted calls).  Note that the first OnWillRead is forwarded into
// the downstream handler (to use a single buffer from the downstream handler,
// rather than allocating a separate buffer and copying the data between the
// buffers) and this leads to perturbed order of calls (that is OnWillRead is
// received by the downstream handler *before* OnResponseStarted) - this
// behavior is very similar to what MimeSniffingResourceHandler does.
//
// For more details, see:
// http://chromium.org/developers/design-documents/blocking-cross-site-documents
class CONTENT_EXPORT CrossSiteDocumentResourceHandler
    : public LayeredResourceHandler {
 public:
  CrossSiteDocumentResourceHandler(
      std::unique_ptr<ResourceHandler> next_handler,
      net::URLRequest* request,
      bool is_nocors_plugin_request);
  ~CrossSiteDocumentResourceHandler() override;

  // LayeredResourceHandler overrides:
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

 private:
  FRIEND_TEST_ALL_PREFIXES(CrossSiteDocumentResourceHandlerTest,
                           ResponseBlocking);
  FRIEND_TEST_ALL_PREFIXES(CrossSiteDocumentResourceHandlerTest,
                           OnWillReadDefer);
  FRIEND_TEST_ALL_PREFIXES(CrossSiteDocumentResourceHandlerTest,
                           MimeSnifferInterop);

  // A ResourceController subclass for running deferred operations.
  class Controller;

  // Computes whether this response contains a cross-site document that needs to
  // be blocked from the renderer process.  This is a first approximation based
  // on the headers, and may be revised after some of the data is sniffed.
  bool ShouldBlockBasedOnHeaders(const network::ResourceResponse& response);

  // Once the downstream handler has allocated the buffer for OnWillRead
  // (possibly after deferring), this sets up sniffing into a local buffer.
  // Called by the OnWillReadController.
  void ResumeOnWillRead(scoped_refptr<net::IOBuffer>* buf, int* buf_size);

  // Stops local buffering and optionally copies the data from the
  // |local_buffer_| into the |next_handler_|'s buffer that was returned by the
  // |next_handler_| in response to OnWillRead.
  void StopLocalBuffering(bool copy_data_to_next_handler);

  // Helpers for UMA and UKM logging.
  static void LogBlockedResponseOnUIThread(
      ResourceRequestInfo::WebContentsGetter web_contents_getter,
      bool needed_sniffing,
      network::CrossOriginReadBlocking::MimeType canonical_mime_type,
      ResourceType resource_type,
      int http_response_code,
      int64_t content_length);
  void LogBlockedResponse(ResourceRequestInfoImpl* resource_request_info,
                          int http_response_code);

  // WeakPtrFactory for |next_handler_|.
  base::WeakPtrFactory<ResourceHandler> weak_next_handler_;

  // Temporary storage for response headers, while we haven't yet made the
  // allow-vs-block decisions and haven't yet passed the response headers down
  // to the next handler.
  scoped_refptr<network::ResourceResponse> pending_response_start_;

  // A local buffer for sniffing content and using for throwaway reads.
  // This is not shared with the renderer process.
  scoped_refptr<net::IOBuffer> local_buffer_;

  // The buffer allocated by the next ResourceHandler for reads, which is used
  // if sniffing determines that we should proceed with the response.
  scoped_refptr<net::IOBuffer> next_handler_buffer_;

  // The number of bytes written into |local_buffer_| by previous reads.
  int local_buffer_bytes_read_ = 0;

  // The size of |next_handler_buffer_|.
  int next_handler_buffer_size_ = 0;

  // The helper class that encapsulates the logic for deciding whether to block
  // the response or not.
  std::unique_ptr<network::CrossOriginReadBlocking::ResponseAnalyzer> analyzer_;

  // Indicates whether this request was made by a plugin and was not using CORS.
  // Such requests are exempt from blocking, while other plugin requests must be
  // blocked if the CORS check fails.
  // TODO(creis, nick): Replace this with a plugin process ID check to see if
  // the plugin has universal access.
  bool is_nocors_plugin_request_;

  // Tracks whether OnResponseStarted has been called, to ensure that it happens
  // before OnWillRead and OnReadCompleted.
  bool has_response_started_ = false;

  // Whether this response is a cross-site document that should be blocked,
  // pending the outcome of sniffing the content.  Set in OnResponseStarted and
  // should only be read afterwards.
  bool should_block_based_on_headers_ = false;

  // Whether this response will be allowed through despite being flagged for
  // blocking (via |should_block_based_on_headers_), because sniffing determined
  // it was incorrectly labeled and might be needed for compatibility (e.g.,
  // in case it is Javascript).
  bool allow_based_on_sniffing_ = false;

  // Whether the next ResourceHandler has already been told that the read has
  // completed, and thus it is safe to cancel or detach on the next read.
  bool blocked_read_completed_ = false;

  // The HTTP response code (e.g. 200 or 404) received in response to this
  // resource request.
  int http_response_code_ = 0;

  base::WeakPtrFactory<CrossSiteDocumentResourceHandler> weak_this_;

  DISALLOW_COPY_AND_ASSIGN(CrossSiteDocumentResourceHandler);
};

}  // namespace content

#endif  // CONTENT_BROWSER_LOADER_CROSS_SITE_DOCUMENT_RESOURCE_HANDLER_H_
