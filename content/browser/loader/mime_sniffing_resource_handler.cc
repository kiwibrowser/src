// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/loader/mime_sniffing_resource_handler.h"

#include <utility>
#include <vector>

#include "base/bind.h"
#include "base/feature_list.h"
#include "base/location.h"
#include "base/logging.h"
#include "base/single_thread_task_runner.h"
#include "base/strings/string_util.h"
#include "base/threading/thread_task_runner_handle.h"
#include "components/download/public/common/download_item.h"
#include "components/download/public/common/download_stats.h"
#include "components/download/public/common/download_url_parameters.h"
#include "content/browser/download/download_resource_handler.h"
#include "content/browser/loader/intercepting_resource_handler.h"
#include "content/browser/loader/resource_controller.h"
#include "content/browser/loader/resource_dispatcher_host_impl.h"
#include "content/browser/loader/resource_request_info_impl.h"
#include "content/browser/loader/stream_resource_handler.h"
#include "content/browser/web_package/signed_exchange_utils.h"
#include "content/browser/web_package/web_package_request_handler.h"
#include "content/public/browser/content_browser_client.h"
#include "content/public/browser/plugin_service.h"
#include "content/public/browser/resource_context.h"
#include "content/public/browser/resource_dispatcher_host_delegate.h"
#include "content/public/common/content_client.h"
#include "content/public/common/content_features.h"
#include "content/public/common/webplugininfo.h"
#include "net/base/io_buffer.h"
#include "net/base/mime_sniffer.h"
#include "net/base/mime_util.h"
#include "net/http/http_content_disposition.h"
#include "net/http/http_response_headers.h"
#include "net/url_request/url_request.h"
#include "ppapi/buildflags/buildflags.h"
#include "services/network/loader_util.h"
#include "services/network/public/cpp/resource_response.h"
#include "third_party/blink/public/common/mime_util/mime_util.h"
#include "third_party/blink/public/common/origin_trials/trial_token_validator.h"
#include "url/origin.h"

namespace content {

namespace {

// Used to write into an existing IOBuffer at a given offset. This is
// very similar to DependentIOBufferForRedirectToFile and
// DependentIOBufferForAsyncLoading but not identical.
class DependentIOBufferForMimeSniffing : public net::WrappedIOBuffer {
 public:
  DependentIOBufferForMimeSniffing(net::IOBuffer* buf, int offset)
      : net::WrappedIOBuffer(buf->data() + offset), buf_(buf) {}

 private:
  ~DependentIOBufferForMimeSniffing() override {}

  scoped_refptr<net::IOBuffer> buf_;
};

}  // namespace

class MimeSniffingResourceHandler::Controller : public ResourceController {
 public:
  explicit Controller(MimeSniffingResourceHandler* mime_handler)
      : mime_handler_(mime_handler) {}

  void Resume() override {
    MarkAsUsed();
    mime_handler_->ResumeInternal();
  }

  void Cancel() override {
    MarkAsUsed();
    mime_handler_->Cancel();
  }

  void CancelWithError(int error_code) override {
    MarkAsUsed();
    mime_handler_->CancelWithError(error_code);
  }

 private:
  void MarkAsUsed() {
#if DCHECK_IS_ON()
    DCHECK(!used_);
    used_ = true;
#endif
  }

#if DCHECK_IS_ON()
  bool used_ = false;
#endif

  MimeSniffingResourceHandler* mime_handler_;

  DISALLOW_COPY_AND_ASSIGN(Controller);
};

MimeSniffingResourceHandler::MimeSniffingResourceHandler(
    std::unique_ptr<ResourceHandler> next_handler,
    ResourceDispatcherHostImpl* host,
    PluginService* plugin_service,
    InterceptingResourceHandler* intercepting_handler,
    net::URLRequest* request,
    RequestContextType request_context_type)
    : LayeredResourceHandler(request, std::move(next_handler)),
      state_(STATE_STARTING),
      host_(host),
#if BUILDFLAG(ENABLE_PLUGINS)
      plugin_service_(plugin_service),
#endif
      must_download_(false),
      must_download_is_set_(false),
      read_buffer_size_(0),
      bytes_read_(0),
      need_to_replay_extra_eof_packet_(false),
      parent_read_buffer_(nullptr),
      parent_read_buffer_size_(nullptr),
      intercepting_handler_(intercepting_handler),
      request_context_type_(request_context_type),
      in_state_loop_(false),
      advance_state_(false),
      weak_ptr_factory_(this) {
}

MimeSniffingResourceHandler::~MimeSniffingResourceHandler() {}

void MimeSniffingResourceHandler::OnWillStart(
    const GURL& url,
    std::unique_ptr<ResourceController> controller) {
  DCHECK(!has_controller());

  next_handler_->OnWillStart(url, std::move(controller));
}

void MimeSniffingResourceHandler::OnResponseStarted(
    network::ResourceResponse* response,
    std::unique_ptr<ResourceController> controller) {
  DCHECK_EQ(STATE_STARTING, state_);
  DCHECK(!has_controller());

  response_ = response;

  state_ = STATE_BUFFERING;
  // A 304 response should not contain a Content-Type header (RFC 7232 section
  // 4.1). The following code may incorrectly attempt to add a Content-Type to
  // the response, and so must be skipped for 304 responses.
  if (!(response_->head.headers.get() &&
        response_->head.headers->response_code() == 304)) {
    // MIME sniffing should be disabled for a request initiated by fetch().
    if (request_context_type_ != REQUEST_CONTEXT_TYPE_FETCH &&
        network::ShouldSniffContent(request(), response_.get())) {
      controller->Resume();
      return;
    }

    if (response_->head.mime_type.empty()) {
      // Ugg.  The server told us not to sniff the content but didn't give us a
      // mime type.  What's a browser to do?  Turns out, we're supposed to
      // treat the response as "text/plain".  This is the most secure option.
      response_->head.mime_type.assign("text/plain");
    }

    // Treat feed types as text/plain.
    if (response_->head.mime_type == "application/rss+xml" ||
        response_->head.mime_type == "application/atom+xml") {
      response_->head.mime_type.assign("text/plain");
    }
  }

  HoldController(std::move(controller));
  AdvanceState();
}

void MimeSniffingResourceHandler::OnWillRead(
    scoped_refptr<net::IOBuffer>* buf,
    int* buf_size,
    std::unique_ptr<ResourceController> controller) {
  DCHECK(buf);
  DCHECK(buf_size);
  DCHECK(!parent_read_buffer_);
  DCHECK(!parent_read_buffer_size_);

  if (state_ == STATE_STREAMING) {
    next_handler_->OnWillRead(buf, buf_size, std::move(controller));
    return;
  }

  DCHECK_EQ(State::STATE_BUFFERING, state_);

  if (read_buffer_.get()) {
    CHECK_LT(bytes_read_, read_buffer_size_);
    *buf =
        new DependentIOBufferForMimeSniffing(read_buffer_.get(), bytes_read_);
    *buf_size = read_buffer_size_ - bytes_read_;
    controller->Resume();
    return;
  }

  DCHECK(!read_buffer_size_);

  parent_read_buffer_ = buf;
  parent_read_buffer_size_ = buf_size;

  HoldController(std::move(controller));

  // Have to go through AdvanceState here so that if OnWillRead completes
  // synchronously, won't post a task.
  state_ = State::STATE_CALLING_ON_WILL_READ;
  AdvanceState();
}

void MimeSniffingResourceHandler::OnReadCompleted(
    int bytes_read,
    std::unique_ptr<ResourceController> controller) {
  DCHECK(!has_controller());

  if (state_ == STATE_STREAMING) {
    next_handler_->OnReadCompleted(bytes_read, std::move(controller));
    return;
  }

  DCHECK_EQ(state_, STATE_BUFFERING);
  bytes_read_ += bytes_read;

  const std::string& type_hint = response_->head.mime_type;

  std::string new_type;
  bool made_final_decision = net::SniffMimeType(
      read_buffer_->data(), bytes_read_, request()->url(), type_hint,
      GetContentClient()->browser()->ForceSniffingFileUrlsForHtml()
          ? net::ForceSniffFileUrlsForHtml::kEnabled
          : net::ForceSniffFileUrlsForHtml::kDisabled,
      &new_type);

  // SniffMimeType() returns false if there is not enough data to determine
  // the mime type. However, even if it returns false, it returns a new type
  // that is probably better than the current one.
  response_->head.mime_type.assign(new_type);

  if (!made_final_decision && (bytes_read > 0)) {
    controller->Resume();
    return;
  }

  // After getting a 0-sized, eof-indicating packet when buffering, the packet
  // (i.e. the OnReadCompleted(0) call) needs to be replayed against the
  // downstream handler (unless replaying the buffered data will act as one if
  // |bytes_read_ == 0| - the first part of the condition below).
  need_to_replay_extra_eof_packet_ = (bytes_read_ != 0) && (bytes_read == 0);

  HoldController(std::move(controller));
  AdvanceState();
}

void MimeSniffingResourceHandler::OnResponseCompleted(
    const net::URLRequestStatus& status,
    std::unique_ptr<ResourceController> resource_controller) {
  // Upon completion, act like a pass-through handler in case the downstream
  // handler defers OnResponseCompleted.
  state_ = STATE_STREAMING;

  next_handler_->OnResponseCompleted(status, std::move(resource_controller));
}

void MimeSniffingResourceHandler::ResumeInternal() {
  DCHECK_NE(state_, STATE_BUFFERING);
  DCHECK(!advance_state_);

  // If no information is currently being transmitted to downstream handlers,
  // they should not attempt to resume the request.
  if (state_ == STATE_BUFFERING) {
    NOTREACHED();
    return;
  }

  if (in_state_loop_) {
    advance_state_ = true;
    return;
  }

  // Otherwise proceed with the replay of the response. If it is successful,
  // it will resume the request. Posted as a task to avoid re-entrancy into
  // the calling class.
  base::ThreadTaskRunnerHandle::Get()->PostTask(
      FROM_HERE, base::BindOnce(&MimeSniffingResourceHandler::AdvanceState,
                                weak_ptr_factory_.GetWeakPtr()));
}

void MimeSniffingResourceHandler::AdvanceState() {
  DCHECK(!in_state_loop_);
  DCHECK(!advance_state_);

  base::AutoReset<bool> auto_in_state_loop(&in_state_loop_, true);
  advance_state_ = true;
  while (advance_state_) {
    advance_state_ = false;

    switch (state_) {
      case STATE_BUFFERING:
        MaybeIntercept();
        break;
      case STATE_CALLING_ON_WILL_READ:
        CallOnWillRead();
        break;
      case STATE_WAITING_FOR_BUFFER:
        BufferReceived();
        break;
      case STATE_INTERCEPTION_CHECK_DONE:
        ReplayResponseReceived();
        break;
      case STATE_REPLAYING_RESPONSE_RECEIVED:
        ReplayReadCompleted();
        break;
      case STATE_REPLAYING_EOF_WILL_READ:
        ReplayWillReadEof();
        break;
      case STATE_REPLAYING_EOF_READ_COMPLETED:
        ReplayReadCompletedEof();
        break;
      case STATE_STARTING:
      case STATE_STREAMING:
        Resume();
        return;
      default:
        NOTREACHED();
        break;
    }
  }

  DCHECK(in_state_loop_);
  in_state_loop_ = false;
}

void MimeSniffingResourceHandler::MaybeIntercept() {
  DCHECK_EQ(STATE_BUFFERING, state_);
  // If a request that can be intercepted failed the check for interception
  // step, it should be canceled.
  if (!MaybeStartInterception())
    return;

  state_ = STATE_INTERCEPTION_CHECK_DONE;
  ResumeInternal();
}

void MimeSniffingResourceHandler::CallOnWillRead() {
  DCHECK_EQ(STATE_CALLING_ON_WILL_READ, state_);

  state_ = STATE_WAITING_FOR_BUFFER;
  next_handler_->OnWillRead(&read_buffer_, &read_buffer_size_,
                            std::make_unique<Controller>(this));
}

void MimeSniffingResourceHandler::BufferReceived() {
  DCHECK_EQ(STATE_WAITING_FOR_BUFFER, state_);

  DCHECK(read_buffer_);
  DCHECK(parent_read_buffer_);
  DCHECK(parent_read_buffer_size_);
  DCHECK_GE(read_buffer_size_, net::kMaxBytesToSniff * 2);

  *parent_read_buffer_ = read_buffer_;
  *parent_read_buffer_size_ = read_buffer_size_;

  parent_read_buffer_ = nullptr;
  parent_read_buffer_size_ = nullptr;

  state_ = State::STATE_BUFFERING;
  Resume();
}

void MimeSniffingResourceHandler::ReplayResponseReceived() {
  DCHECK_EQ(STATE_INTERCEPTION_CHECK_DONE, state_);
  state_ = STATE_REPLAYING_RESPONSE_RECEIVED;
  next_handler_->OnResponseStarted(response_.get(),
                                   std::make_unique<Controller>(this));
}

void MimeSniffingResourceHandler::ReplayReadCompleted() {
  DCHECK_EQ(STATE_REPLAYING_RESPONSE_RECEIVED, state_);

  if (need_to_replay_extra_eof_packet_)
    state_ = STATE_REPLAYING_EOF_WILL_READ;
  else
    state_ = STATE_STREAMING;

  if (!read_buffer_.get()) {
    ResumeInternal();
    return;
  }

  int bytes_read = bytes_read_;

  read_buffer_ = nullptr;
  read_buffer_size_ = 0;
  bytes_read_ = 0;

  next_handler_->OnReadCompleted(bytes_read,
                                 std::make_unique<Controller>(this));
}

void MimeSniffingResourceHandler::ReplayWillReadEof() {
  DCHECK_EQ(STATE_REPLAYING_EOF_WILL_READ, state_);

  state_ = STATE_REPLAYING_EOF_READ_COMPLETED;
  DCHECK(!read_buffer_);
  DCHECK_EQ(0, read_buffer_size_);
  DCHECK_EQ(0, bytes_read_);
  next_handler_->OnWillRead(&read_buffer_, &read_buffer_size_,
                            std::make_unique<Controller>(this));
}

void MimeSniffingResourceHandler::ReplayReadCompletedEof() {
  DCHECK_EQ(STATE_REPLAYING_EOF_READ_COMPLETED, state_);

  state_ = STATE_STREAMING;
  read_buffer_ = nullptr;
  read_buffer_size_ = 0;
  bytes_read_ = 0;
  next_handler_->OnReadCompleted(0, std::make_unique<Controller>(this));
}

bool MimeSniffingResourceHandler::MaybeStartInterception() {
  if (!CanBeIntercepted())
    return true;

  DCHECK(!response_->head.mime_type.empty());

  ResourceRequestInfoImpl* info = GetRequestInfo();
  const std::string& mime_type = response_->head.mime_type;

  // Allow requests for object/embed tags to be intercepted as streams.
  if (info->GetResourceType() == content::RESOURCE_TYPE_OBJECT) {
    DCHECK(!info->allow_download());

    bool handled_by_plugin;
    if (!CheckForPluginHandler(&handled_by_plugin))
      return false;
    if (handled_by_plugin)
      return true;
  }

  if (!info->allow_download())
    return true;

  // info->allow_download() == true implies
  // info->GetResourceType() == RESOURCE_TYPE_MAIN_FRAME or
  // info->GetResourceType() == RESOURCE_TYPE_SUB_FRAME.
  DCHECK(info->GetResourceType() == RESOURCE_TYPE_MAIN_FRAME ||
         info->GetResourceType() == RESOURCE_TYPE_SUB_FRAME);

  bool must_download = MustDownload();
  if (!must_download) {
    if (blink::IsSupportedMimeType(mime_type))
      return true;
    if (signed_exchange_utils::ShouldHandleAsSignedHTTPExchange(
            request()->url(), response_->head)) {
      return true;
    }
    bool handled_by_plugin;
    if (!CheckForPluginHandler(&handled_by_plugin))
      return false;
    if (handled_by_plugin)
      return true;
  }

  // This request is a download.

  if (!CheckResponseIsNotProvisional())
    return false;

  info->set_is_download(true);
  std::unique_ptr<ResourceHandler> handler(
      host_->CreateResourceHandlerForDownload(request(),
                                              true,  // is_content_initiated
                                              must_download,
                                              false /* is_new_request */));
  intercepting_handler_->UseNewHandler(std::move(handler), std::string());
  return true;
}

bool MimeSniffingResourceHandler::CheckForPluginHandler(
    bool* handled_by_plugin) {
  *handled_by_plugin = false;
#if BUILDFLAG(ENABLE_PLUGINS)
  ResourceRequestInfoImpl* info = GetRequestInfo();
  bool allow_wildcard = false;
  bool stale;
  WebPluginInfo plugin;
  bool has_plugin = plugin_service_->GetPluginInfo(
      info->GetChildID(), info->GetRenderFrameID(), info->GetContext(),
      request()->url(), url::Origin(), response_->head.mime_type,
      allow_wildcard, &stale, &plugin, nullptr);

  if (stale) {
    // Refresh the plugins asynchronously.
    plugin_service_->GetPlugins(
        base::BindOnce(&MimeSniffingResourceHandler::OnPluginsLoaded,
                       weak_ptr_factory_.GetWeakPtr()));
    request()->LogBlockedBy("MimeSniffingResourceHandler");
    // Will complete asynchronously.
    return false;
  }

  if (has_plugin && plugin.type != WebPluginInfo::PLUGIN_TYPE_BROWSER_PLUGIN) {
    *handled_by_plugin = true;
    return true;
  }

  // Attempt to intercept the request as a stream.
  std::string payload;
  std::unique_ptr<ResourceHandler> handler(
      host_->MaybeInterceptAsStream(request(), response_.get(), &payload));
  if (handler) {
    if (!CheckResponseIsNotProvisional())
      return false;
    *handled_by_plugin = true;
    intercepting_handler_->UseNewHandler(std::move(handler), payload);
  }
#endif
  return true;
}

bool MimeSniffingResourceHandler::CanBeIntercepted() {
  if (response_->head.headers.get() &&
      response_->head.headers->response_code() == 304) {
    return false;
  }

  return true;
}

bool MimeSniffingResourceHandler::CheckResponseIsNotProvisional() {
  if (!response_->head.headers.get() ||
      response_->head.headers->response_code() / 100 == 2) {
    return true;
  }

  // The response code indicates that this is an error page, but we don't
  // know how to display the content.  We follow Firefox here and show our
  // own error page instead of intercepting the request as a stream or a
  // download.
  // TODO(abarth): We should abstract the response_code test, but this kind
  // of check is scattered throughout our codebase.
  CancelWithError(net::ERR_INVALID_RESPONSE);
  return false;
}

bool MimeSniffingResourceHandler::MustDownload() {
  if (must_download_is_set_)
    return must_download_;

  must_download_is_set_ = true;

  std::string disposition;
  request()->GetResponseHeaderByName("content-disposition", &disposition);
  if (!disposition.empty() &&
      net::HttpContentDisposition(disposition, std::string()).is_attachment()) {
    must_download_ = true;
  } else if (GetContentClient()->browser()->ShouldForceDownloadResource(
                 request()->url(), response_->head.mime_type)) {
    must_download_ = true;
  } else if (request()->url().SchemeIsHTTPOrHTTPS() &&
             // The MHTML mime type should be same as the one we check in
             // Blink's DocumentLoader.
             (response_->head.mime_type == "multipart/related" ||
              response_->head.mime_type == "message/rfc822")) {
    // It is OK to load the saved offline copy, in MHTML format.
    const ResourceRequestInfo* info =
        ResourceRequestInfo::ForRequest(request());
    must_download_ =
        !GetContentClient()->browser()->AllowRenderingMhtmlOverHttp(
            info->GetNavigationUIData());
  } else {
    must_download_ = false;
  }

  return must_download_;
}

void MimeSniffingResourceHandler::OnPluginsLoaded(
    const std::vector<WebPluginInfo>& plugins) {
  // No longer blocking on the plugins being loaded.
  request()->LogUnblocked();
  if (state_ == STATE_BUFFERING)
    AdvanceState();
}

}  // namespace content
