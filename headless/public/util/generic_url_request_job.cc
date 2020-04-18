// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "headless/public/util/generic_url_request_job.h"

#include <string.h>
#include <algorithm>

#include "base/logging.h"
#include "base/run_loop.h"
#include "content/public/browser/devtools_agent_host.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/resource_request_info.h"
#include "content/public/browser/web_contents.h"
#include "headless/lib/browser/headless_browser_context_impl.h"
#include "headless/public/headless_browser_context.h"
#include "headless/public/util/url_request_dispatcher.h"
#include "net/base/io_buffer.h"
#include "net/base/load_flags.h"
#include "net/base/net_errors.h"
#include "net/base/registry_controlled_domains/registry_controlled_domain.h"
#include "net/base/upload_bytes_element_reader.h"
#include "net/cookies/canonical_cookie.h"
#include "net/cookies/cookie_store.h"
#include "net/http/http_response_headers.h"
#include "net/http/http_util.h"
#include "net/url_request/http_user_agent_settings.h"
#include "net/url_request/url_request_context.h"

namespace headless {

GenericURLRequestJob::GenericURLRequestJob(
    net::URLRequest* request,
    net::NetworkDelegate* network_delegate,
    URLRequestDispatcher* url_request_dispatcher,
    std::unique_ptr<URLFetcher> url_fetcher,
    Delegate* delegate,
    HeadlessBrowserContext* headless_browser_context)
    : ManagedDispatchURLRequestJob(request,
                                   network_delegate,
                                   url_request_dispatcher),
      url_fetcher_(std::move(url_fetcher)),
      origin_task_runner_(base::ThreadTaskRunnerHandle::Get()),
      delegate_(delegate),
      headless_browser_context_(headless_browser_context),
      request_resource_info_(
          content::ResourceRequestInfo::ForRequest(request_)),
      weak_factory_(this) {}

GenericURLRequestJob::~GenericURLRequestJob() {
  DCHECK(origin_task_runner_->RunsTasksInCurrentSequence());
}

void GenericURLRequestJob::SetExtraRequestHeaders(
    const net::HttpRequestHeaders& headers) {
  DCHECK(origin_task_runner_->RunsTasksInCurrentSequence());
  extra_request_headers_ = headers;

  if (!request_->referrer().empty()) {
    extra_request_headers_.SetHeader(net::HttpRequestHeaders::kReferer,
                                     request_->referrer());
  }

  const net::HttpUserAgentSettings* user_agent_settings =
      request()->context()->http_user_agent_settings();
  // If set the |user_agent_settings| accept language is a fallback.
  if (user_agent_settings) {
    extra_request_headers_.SetHeaderIfMissing(
        net::HttpRequestHeaders::kAcceptLanguage,
        user_agent_settings->GetAcceptLanguage());
  }
}

void GenericURLRequestJob::Start() {
  PrepareCookies(request_->url(), request_->method(),
                 url::Origin::Create(request_->site_for_cookies()));
}

void GenericURLRequestJob::PrepareCookies(const GURL& rewritten_url,
                                          const std::string& method,
                                          const url::Origin& site_for_cookies) {
  DCHECK(origin_task_runner_->RunsTasksInCurrentSequence());
  net::CookieStore* cookie_store = request_->context()->cookie_store();
  net::CookieOptions options;
  options.set_include_httponly();

  // See net::URLRequestHttpJob::AddCookieHeaderAndStart().
  url::Origin requested_origin = url::Origin::Create(rewritten_url);
  if (net::registry_controlled_domains::SameDomainOrHost(
          requested_origin, site_for_cookies,
          net::registry_controlled_domains::INCLUDE_PRIVATE_REGISTRIES)) {
    if (net::registry_controlled_domains::SameDomainOrHost(
            requested_origin, request_->initiator(),
            net::registry_controlled_domains::INCLUDE_PRIVATE_REGISTRIES)) {
      options.set_same_site_cookie_mode(
          net::CookieOptions::SameSiteCookieMode::INCLUDE_STRICT_AND_LAX);
    } else if (net::HttpUtil::IsMethodSafe(request_->method())) {
      options.set_same_site_cookie_mode(
          net::CookieOptions::SameSiteCookieMode::INCLUDE_LAX);
    }
  }

  cookie_store->GetCookieListWithOptionsAsync(
      rewritten_url, options,
      base::BindOnce(&GenericURLRequestJob::OnCookiesAvailable,
                     weak_factory_.GetWeakPtr(), rewritten_url, method));
}

void GenericURLRequestJob::OnCookiesAvailable(
    const GURL& rewritten_url,
    const std::string& method,
    const net::CookieList& cookie_list) {
  DCHECK(origin_task_runner_->RunsTasksInCurrentSequence());
  // TODO(alexclarke): Set user agent.
  // Pass cookies, the referrer and any extra headers into the fetch request.
  std::string cookie = net::CanonicalCookie::BuildCookieLine(cookie_list);
  if (!cookie.empty())
    extra_request_headers_.SetHeader(net::HttpRequestHeaders::kCookie, cookie);

  url_fetcher_->StartFetch(this, this);
}

void GenericURLRequestJob::OnFetchStartError(net::Error error) {
  DCHECK(origin_task_runner_->RunsTasksInCurrentSequence());
  DispatchStartError(error);
  delegate_->OnResourceLoadFailed(this, error);
}

void GenericURLRequestJob::OnFetchComplete(
    const GURL& final_url,
    scoped_refptr<net::HttpResponseHeaders> response_headers,
    const char* body,
    size_t body_size,
    scoped_refptr<net::IOBufferWithSize> metadata,
    const net::LoadTimingInfo& load_timing_info,
    size_t total_received_bytes) {
  DCHECK(origin_task_runner_->RunsTasksInCurrentSequence());
  response_headers_ = response_headers;
  body_ = body;
  body_size_ = body_size;
  load_timing_info_ = load_timing_info;
  total_received_bytes_ = total_received_bytes;
  metadata_ = metadata;

  // Save any cookies from the response.
  if (!(request_->load_flags() & net::LOAD_DO_NOT_SAVE_COOKIES) &&
      request_->context()->cookie_store()) {
    net::CookieOptions options;
    options.set_include_httponly();

    base::Time response_date;
    if (!response_headers_->GetDateValue(&response_date))
      response_date = base::Time();
    options.set_server_time(response_date);

    // Set all cookies, without waiting for them to be set. Any subsequent read
    // will see the combined result of all cookie operation.
    const base::StringPiece name("Set-Cookie");
    std::string cookie_line;
    size_t iter = 0;
    while (response_headers_->EnumerateHeader(&iter, name, &cookie_line)) {
      std::unique_ptr<net::CanonicalCookie> cookie =
          net::CanonicalCookie::Create(final_url, cookie_line,
                                       base::Time::Now(), options);
      if (!cookie || !CanSetCookie(*cookie, &options))
        continue;
      request_->context()->cookie_store()->SetCanonicalCookieAsync(
          std::move(cookie), final_url.SchemeIsCryptographic(),
          !options.exclude_httponly(), net::CookieStore::SetCookiesCallback());
    }
  }

  DispatchHeadersComplete();

  delegate_->OnResourceLoadComplete(this, final_url, response_headers_, body_,
                                    body_size_);
}

int GenericURLRequestJob::ReadRawData(net::IOBuffer* buf, int buf_size) {
  DCHECK(origin_task_runner_->RunsTasksInCurrentSequence());
  // TODO(skyostil): Implement ranged fetches.
  // TODO(alexclarke): Add coverage for all the cases below.
  size_t bytes_available = body_size_ - read_offset_;
  size_t bytes_to_copy =
      std::min(static_cast<size_t>(buf_size), bytes_available);
  if (bytes_to_copy) {
    std::memcpy(buf->data(), &body_[read_offset_], bytes_to_copy);
    read_offset_ += bytes_to_copy;
  }
  return bytes_to_copy;
}

void GenericURLRequestJob::GetResponseInfo(net::HttpResponseInfo* info) {
  info->headers = response_headers_;
  info->metadata = metadata_;

  // Important we need to set this so we can detect if a navigation request got
  // canceled by DevTools.
  info->network_accessed = true;
}

bool GenericURLRequestJob::GetMimeType(std::string* mime_type) const {
  if (!response_headers_)
    return false;
  return response_headers_->GetMimeType(mime_type);
}

bool GenericURLRequestJob::GetCharset(std::string* charset) {
  if (!response_headers_)
    return false;
  return response_headers_->GetCharset(charset);
}

void GenericURLRequestJob::GetLoadTimingInfo(
    net::LoadTimingInfo* load_timing_info) const {
  *load_timing_info = load_timing_info_;
}

int64_t GenericURLRequestJob::GetTotalReceivedBytes() const {
  return total_received_bytes_;
}

uint64_t GenericURLRequestJob::GenericURLRequestJob::GetRequestId() const {
  return request_->identifier() +
         (static_cast<uint64_t>(request_->url_chain().size()) << 32);
}

const std::string& GenericURLRequestJob::GetMethod() const {
  return request_->method();
}

const GURL& GenericURLRequestJob::GetURL() const {
  return request_->url();
}

int GenericURLRequestJob::GetPriority() const {
  return request_->priority();
}

const net::URLRequest* GenericURLRequestJob::GetURLRequest() const {
  return request_;
}

const net::HttpRequestHeaders& GenericURLRequestJob::GetHttpRequestHeaders()
    const {
  return extra_request_headers_;
}

std::string GenericURLRequestJob::GetDevToolsFrameId() const {
  // TODO(alexclarke): When available get the new DevTools token directly from
  // |request_resource_info_| and remove the machinery filling these maps.

  // URLRequestUserData will be set for all renderer initiated resource
  // requests, but not for browser side navigations.
  int render_process_id;
  int render_frame_routing_id;
  if (content::ResourceRequestInfo::GetRenderFrameForRequest(
          request_, &render_process_id, &render_frame_routing_id) &&
      render_process_id != -1) {
    DCHECK(headless_browser_context_);
    const base::UnguessableToken* devtools_frame_token =
        static_cast<HeadlessBrowserContextImpl*>(headless_browser_context_)
            ->GetDevToolsFrameToken(render_process_id, render_frame_routing_id);
    if (!devtools_frame_token)
      return "";
    return devtools_frame_token->ToString();
  }

  // ResourceRequestInfo::GetFrameTreeNodeId is only set for browser side
  // navigations.
  if (request_resource_info_) {
    const base::UnguessableToken* devtools_frame_token =
        static_cast<HeadlessBrowserContextImpl*>(headless_browser_context_)
            ->GetDevToolsFrameTokenForFrameTreeNodeId(
                request_resource_info_->GetFrameTreeNodeId());
    if (!devtools_frame_token)
      return "";
    return devtools_frame_token->ToString();
  }

  // This should only happen in tests.
  return "";
}

std::string GenericURLRequestJob::GetDevToolsAgentHostId() const {
  return content::DevToolsAgentHost::GetOrCreateFor(
             request_resource_info_->GetWebContentsGetterForRequest().Run())
      ->GetId();
}

Request::ResourceType GenericURLRequestJob::GetResourceType() const {
  // This should only happen in some tests.
  if (!request_resource_info_)
    return Request::ResourceType::MAIN_FRAME;

  switch (request_resource_info_->GetResourceType()) {
    case content::RESOURCE_TYPE_MAIN_FRAME:
      return Request::ResourceType::MAIN_FRAME;
    case content::RESOURCE_TYPE_SUB_FRAME:
      return Request::ResourceType::SUB_FRAME;
    case content::RESOURCE_TYPE_STYLESHEET:
      return Request::ResourceType::STYLESHEET;
    case content::RESOURCE_TYPE_SCRIPT:
      return Request::ResourceType::SCRIPT;
    case content::RESOURCE_TYPE_IMAGE:
      return Request::ResourceType::IMAGE;
    case content::RESOURCE_TYPE_FONT_RESOURCE:
      return Request::ResourceType::FONT_RESOURCE;
    case content::RESOURCE_TYPE_SUB_RESOURCE:
      return Request::ResourceType::SUB_RESOURCE;
    case content::RESOURCE_TYPE_OBJECT:
      return Request::ResourceType::OBJECT;
    case content::RESOURCE_TYPE_MEDIA:
      return Request::ResourceType::MEDIA;
    case content::RESOURCE_TYPE_WORKER:
      return Request::ResourceType::WORKER;
    case content::RESOURCE_TYPE_SHARED_WORKER:
      return Request::ResourceType::SHARED_WORKER;
    case content::RESOURCE_TYPE_PREFETCH:
      return Request::ResourceType::PREFETCH;
    case content::RESOURCE_TYPE_FAVICON:
      return Request::ResourceType::FAVICON;
    case content::RESOURCE_TYPE_XHR:
      return Request::ResourceType::XHR;
    case content::RESOURCE_TYPE_PING:
      return Request::ResourceType::PING;
    case content::RESOURCE_TYPE_SERVICE_WORKER:
      return Request::ResourceType::SERVICE_WORKER;
    case content::RESOURCE_TYPE_CSP_REPORT:
      return Request::ResourceType::CSP_REPORT;
    case content::RESOURCE_TYPE_PLUGIN_RESOURCE:
      return Request::ResourceType::PLUGIN_RESOURCE;
    default:
      NOTREACHED() << "Unrecognized resource type";
      return Request::ResourceType::MAIN_FRAME;
  }
}

bool GenericURLRequestJob::IsAsync() const {
  // In some tests |request_resource_info_| is null.
  if (request_resource_info_)
    return request_resource_info_->IsAsync();
  return true;
}

bool GenericURLRequestJob::IsBrowserSideFetch() const {
  if (!request_resource_info_)
    return false;
  return request_resource_info_->GetFrameTreeNodeId() != -1;
}

bool GenericURLRequestJob::HasUserGesture() const {
  if (!request_resource_info_)
    return false;
  return request_resource_info_->HasUserGesture();
}

namespace {
void CompletionCallback(int* dest, base::Closure* quit_closure, int value) {
  *dest = value;
  quit_closure->Run();
}

bool ReadAndAppendBytes(const std::unique_ptr<net::UploadElementReader>& reader,
                        std::string& post_data) {
  // TODO(alexclarke): Consider changing the interface of
  // GenericURLRequestJob::GetPostData to use a callback which would let us
  // avoid the nested run loops below.

  // Read the POST bytes.
  uint64_t size_to_stop_at = post_data.size() + reader->GetContentLength();
  const size_t block_size = 1024;
  scoped_refptr<net::IOBuffer> read_buffer(new net::IOBuffer(block_size));
  while (post_data.size() < size_to_stop_at) {
    base::Closure quit_closure;
    int bytes_read = reader->Read(
        read_buffer.get(), block_size,
        base::BindOnce(&CompletionCallback, &bytes_read, &quit_closure));

    if (bytes_read == net::ERR_IO_PENDING) {
      base::RunLoop nested_run_loop(base::RunLoop::Type::kNestableTasksAllowed);
      quit_closure = nested_run_loop.QuitClosure();
      nested_run_loop.Run();
    }

    // Bail out if an error occured.
    if (bytes_read < 0)
      return false;

    post_data.append(read_buffer->data(), bytes_read);
  }
  return true;
}
}  // namespace

const std::vector<std::unique_ptr<net::UploadElementReader>>*
GenericURLRequestJob::GetInitializedReaders() const {
  if (!request_->has_upload())
    return nullptr;

  const net::UploadDataStream* stream = request_->get_upload();
  if (!stream->GetElementReaders())
    return nullptr;

  if (stream->GetElementReaders()->size() == 0)
    return nullptr;

  const std::vector<std::unique_ptr<net::UploadElementReader>>* readers =
      stream->GetElementReaders();

  // Initialize the readers, this necessary because some of them will segfault
  // if you try to call any method without initializing first.
  for (size_t i = 0; i < readers->size(); ++i) {
    const std::unique_ptr<net::UploadElementReader>& reader = (*readers)[i];
    if (!reader)
      continue;

    base::Closure quit_closure;
    int init_result = reader->Init(
        base::BindOnce(&CompletionCallback, &init_result, &quit_closure));
    if (init_result == net::ERR_IO_PENDING) {
      base::RunLoop nested_run_loop(base::RunLoop::Type::kNestableTasksAllowed);
      quit_closure = nested_run_loop.QuitClosure();
      nested_run_loop.Run();
    }

    if (init_result != net::OK)
      return nullptr;
  }

  return readers;
}

std::string GenericURLRequestJob::GetPostData() const {
  const std::vector<std::unique_ptr<net::UploadElementReader>>* readers =
      GetInitializedReaders();

  if (!readers)
    return "";

  uint64_t total_content_length = 0;
  for (size_t i = 0; i < readers->size(); ++i) {
    const std::unique_ptr<net::UploadElementReader>& reader = (*readers)[i];
    total_content_length += reader->GetContentLength();
  }
  std::string post_data;
  post_data.reserve(total_content_length);

  for (size_t i = 0; i < readers->size(); ++i) {
    const std::unique_ptr<net::UploadElementReader>& reader = (*readers)[i];
    // If |reader| is actually an UploadBytesElementReader we can get the data
    // directly.
    const net::UploadBytesElementReader* bytes_reader = reader->AsBytesReader();
    if (bytes_reader) {
      post_data.append(bytes_reader->bytes(), bytes_reader->length());
    } else {
      if (!ReadAndAppendBytes(reader, post_data))
        break;  // Bail out if something went wrong.
    }
  }

  return post_data;
}

uint64_t GenericURLRequestJob::GetPostDataSize() const {
  const std::vector<std::unique_ptr<net::UploadElementReader>>* readers =
      GetInitializedReaders();

  if (!readers)
    return 0;

  uint64_t total_content_length = 0;
  for (size_t i = 0; i < readers->size(); ++i) {
    const std::unique_ptr<net::UploadElementReader>& reader = (*readers)[i];
    if (reader)
      total_content_length += reader->GetContentLength();
  }
  return total_content_length;
}

}  // namespace headless
