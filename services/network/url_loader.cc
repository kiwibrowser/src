// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/network/url_loader.h"

#include <limits>
#include <string>
#include <utility>
#include <vector>

#include "base/files/file.h"
#include "base/memory/weak_ptr.h"
#include "base/metrics/histogram_macros.h"
#include "base/task_scheduler/post_task.h"
#include "base/threading/thread_task_runner_handle.h"
#include "base/time/time.h"
#include "mojo/public/cpp/system/simple_watcher.h"
#include "net/base/elements_upload_data_stream.h"
#include "net/base/mime_sniffer.h"
#include "net/base/upload_bytes_element_reader.h"
#include "net/base/upload_file_element_reader.h"
#include "net/cert/symantec_certs.h"
#include "net/ssl/client_cert_store.h"
#include "net/ssl/ssl_private_key.h"
#include "net/url_request/url_request_context.h"
#include "services/network/chunked_data_pipe_upload_data_stream.h"
#include "services/network/data_pipe_element_reader.h"
#include "services/network/loader_util.h"
#include "services/network/network_usage_accumulator.h"
#include "services/network/public/cpp/features.h"
#include "services/network/public/cpp/net_adapters.h"
#include "services/network/public/cpp/resource_request.h"
#include "services/network/public/cpp/resource_response.h"
#include "services/network/public/mojom/url_loader_factory.mojom.h"
#include "services/network/resource_scheduler_client.h"

namespace network {

namespace {
constexpr size_t kDefaultAllocationSize = 512 * 1024;

// TODO: this duplicates some of PopulateResourceResponse in
// content/browser/loader/resource_loader.cc
void PopulateResourceResponse(net::URLRequest* request,
                              bool is_load_timing_enabled,
                              bool include_ssl_info,
                              ResourceResponse* response) {
  response->head.request_time = request->request_time();
  response->head.response_time = request->response_time();
  response->head.headers = request->response_headers();
  request->GetCharset(&response->head.charset);
  response->head.content_length = request->GetExpectedContentSize();
  request->GetMimeType(&response->head.mime_type);
  net::HttpResponseInfo response_info = request->response_info();
  response->head.was_fetched_via_spdy = response_info.was_fetched_via_spdy;
  response->head.was_alpn_negotiated = response_info.was_alpn_negotiated;
  response->head.alpn_negotiated_protocol =
      response_info.alpn_negotiated_protocol;
  response->head.connection_info = response_info.connection_info;
  response->head.socket_address = response_info.socket_address;
  response->head.was_fetched_via_proxy = request->was_fetched_via_proxy();
  response->head.network_accessed = response_info.network_accessed;

  response->head.effective_connection_type =
      net::EFFECTIVE_CONNECTION_TYPE_UNKNOWN;

  if (is_load_timing_enabled)
    request->GetLoadTimingInfo(&response->head.load_timing);

  if (request->ssl_info().cert.get()) {
    response->head.ct_policy_compliance =
        request->ssl_info().ct_policy_compliance;
    response->head.is_legacy_symantec_cert =
        (!net::IsCertStatusError(response->head.cert_status) ||
         net::IsCertStatusMinorError(response->head.cert_status)) &&
        net::IsLegacySymantecCert(request->ssl_info().public_key_hashes);
    response->head.cert_status = request->ssl_info().cert_status;

    if (include_ssl_info)
      response->head.ssl_info = request->ssl_info();
  }

  response->head.request_start = request->creation_time();
  response->head.response_start = base::TimeTicks::Now();
  response->head.encoded_data_length = request->GetTotalReceivedBytes();
}

// A subclass of net::UploadBytesElementReader which owns
// ResourceRequestBody.
class BytesElementReader : public net::UploadBytesElementReader {
 public:
  BytesElementReader(ResourceRequestBody* resource_request_body,
                     const DataElement& element)
      : net::UploadBytesElementReader(element.bytes(), element.length()),
        resource_request_body_(resource_request_body) {
    DCHECK_EQ(DataElement::TYPE_BYTES, element.type());
  }

  ~BytesElementReader() override {}

 private:
  scoped_refptr<ResourceRequestBody> resource_request_body_;

  DISALLOW_COPY_AND_ASSIGN(BytesElementReader);
};

// A subclass of net::UploadFileElementReader which owns
// ResourceRequestBody.
// This class is necessary to ensure the BlobData and any attached shareable
// files survive until upload completion.
class FileElementReader : public net::UploadFileElementReader {
 public:
  FileElementReader(ResourceRequestBody* resource_request_body,
                    base::TaskRunner* task_runner,
                    const DataElement& element)
      : net::UploadFileElementReader(task_runner,
                                     element.path(),
                                     element.offset(),
                                     element.length(),
                                     element.expected_modification_time()),
        resource_request_body_(resource_request_body) {
    DCHECK_EQ(DataElement::TYPE_FILE, element.type());
  }

  ~FileElementReader() override {}

 private:
  scoped_refptr<ResourceRequestBody> resource_request_body_;

  DISALLOW_COPY_AND_ASSIGN(FileElementReader);
};

class RawFileElementReader : public net::UploadFileElementReader {
 public:
  RawFileElementReader(ResourceRequestBody* resource_request_body,
                       base::TaskRunner* task_runner,
                       const DataElement& element)
      : net::UploadFileElementReader(
            task_runner,
            // TODO(mmenke): Is duplicating this necessary?
            element.file().Duplicate(),
            element.path(),
            element.offset(),
            element.length(),
            element.expected_modification_time()),
        resource_request_body_(resource_request_body) {
    DCHECK_EQ(DataElement::TYPE_RAW_FILE, element.type());
  }

  ~RawFileElementReader() override {}

 private:
  scoped_refptr<ResourceRequestBody> resource_request_body_;

  DISALLOW_COPY_AND_ASSIGN(RawFileElementReader);
};

// TODO: copied from content/browser/loader/upload_data_stream_builder.cc.
std::unique_ptr<net::UploadDataStream> CreateUploadDataStream(
    ResourceRequestBody* body,
    base::SequencedTaskRunner* file_task_runner) {
  // In the case of a chunked upload, there will just be one element.
  if (body->elements()->size() == 1 &&
      body->elements()->begin()->type() ==
          DataElement::TYPE_CHUNKED_DATA_PIPE) {
    return std::make_unique<network::ChunkedDataPipeUploadDataStream>(
        body, const_cast<DataElement&>(body->elements()->front())
                  .ReleaseChunkedDataPipeGetter());
  }

  std::vector<std::unique_ptr<net::UploadElementReader>> element_readers;
  for (const auto& element : *body->elements()) {
    switch (element.type()) {
      case DataElement::TYPE_BYTES:
        element_readers.push_back(
            std::make_unique<BytesElementReader>(body, element));
        break;
      case DataElement::TYPE_FILE:
        element_readers.push_back(std::make_unique<FileElementReader>(
            body, file_task_runner, element));
        break;
      case DataElement::TYPE_RAW_FILE:
        element_readers.push_back(std::make_unique<RawFileElementReader>(
            body, file_task_runner, element));
        break;
      case DataElement::TYPE_BLOB: {
        CHECK(false) << "Network service always uses DATA_PIPE for blobs.";
        break;
      }
      case DataElement::TYPE_DATA_PIPE: {
        element_readers.push_back(std::make_unique<DataPipeElementReader>(
            body, element.CloneDataPipeGetter()));
        break;
      }
      case DataElement::TYPE_CHUNKED_DATA_PIPE: {
        // This shouldn't happen, as the traits logic should ensure that if
        // there's a chunked pipe, there's one and only one element.
        NOTREACHED();
        break;
      }
      case DataElement::TYPE_UNKNOWN:
        NOTREACHED();
        break;
    }
  }

  return std::make_unique<net::ElementsUploadDataStream>(
      std::move(element_readers), body->identifier());
}

class SSLPrivateKeyInternal : public net::SSLPrivateKey {
 public:
  SSLPrivateKeyInternal(const std::vector<uint16_t>& algorithm_perferences,
                        mojom::SSLPrivateKeyPtr ssl_private_key)
      : algorithm_perferences_(algorithm_perferences),
        ssl_private_key_(std::move(ssl_private_key)) {
    ssl_private_key_.set_connection_error_handler(
        base::BindOnce(&SSLPrivateKeyInternal::HandleSSLPrivateKeyError, this));
  }

  // net::SSLPrivateKey:
  std::vector<uint16_t> GetAlgorithmPreferences() override {
    return algorithm_perferences_;
  }

  void Sign(uint16_t algorithm,
            base::span<const uint8_t> input,
            net::SSLPrivateKey::SignCallback callback) override {
    std::vector<uint8_t> input_vector(input.begin(), input.end());
    if (ssl_private_key_.encountered_error()) {
      base::ThreadTaskRunnerHandle::Get()->PostTask(
          FROM_HERE,
          base::BindOnce(std::move(callback),
                         net::ERR_SSL_CLIENT_AUTH_CERT_NO_PRIVATE_KEY,
                         input_vector));
      return;
    }

    ssl_private_key_->Sign(algorithm, input_vector,
                           base::BindOnce(&SSLPrivateKeyInternal::Callback,
                                          this, std::move(callback)));
  }

 private:
  ~SSLPrivateKeyInternal() override = default;

  void HandleSSLPrivateKeyError() { ssl_private_key_.reset(); }

  void Callback(net::SSLPrivateKey::SignCallback callback,
                int32_t net_error,
                const std::vector<uint8_t>& input) {
    DCHECK_LE(net_error, 0);
    DCHECK_NE(net_error, net::ERR_IO_PENDING);
    std::move(callback).Run(static_cast<net::Error>(net_error), input);
  }

  std::vector<uint16_t> algorithm_perferences_;
  mojom::SSLPrivateKeyPtr ssl_private_key_;

  DISALLOW_COPY_AND_ASSIGN(SSLPrivateKeyInternal);
};

}  // namespace

URLLoader::URLLoader(
    net::URLRequestContext* url_request_context,
    mojom::NetworkServiceClient* network_service_client,
    DeleteCallback delete_callback,
    mojom::URLLoaderRequest url_loader_request,
    int32_t options,
    const ResourceRequest& request,
    bool report_raw_headers,
    mojom::URLLoaderClientPtr url_loader_client,
    const net::NetworkTrafficAnnotationTag& traffic_annotation,
    const mojom::URLLoaderFactoryParams* factory_params,
    uint32_t request_id,
    scoped_refptr<ResourceSchedulerClient> resource_scheduler_client,
    base::WeakPtr<KeepaliveStatisticsRecorder> keepalive_statistics_recorder,
    base::WeakPtr<NetworkUsageAccumulator> network_usage_accumulator)
    : url_request_context_(url_request_context),
      network_service_client_(network_service_client),
      delete_callback_(std::move(delete_callback)),
      options_(options),
      resource_type_(request.resource_type),
      is_load_timing_enabled_(request.enable_load_timing),
      factory_params_(std::move(factory_params)),
      render_frame_id_(request.render_frame_id),
      request_id_(request_id),
      keepalive_(request.keepalive),
      do_not_prompt_for_login_(request.do_not_prompt_for_login),
      binding_(this, std::move(url_loader_request)),
      auth_challenge_responder_binding_(this),
      url_loader_client_(std::move(url_loader_client)),
      writable_handle_watcher_(FROM_HERE,
                               mojo::SimpleWatcher::ArmingPolicy::MANUAL,
                               base::SequencedTaskRunnerHandle::Get()),
      peer_closed_handle_watcher_(FROM_HERE,
                                  mojo::SimpleWatcher::ArmingPolicy::MANUAL,
                                  base::SequencedTaskRunnerHandle::Get()),
      report_raw_headers_(report_raw_headers),
      resource_scheduler_client_(std::move(resource_scheduler_client)),
      keepalive_statistics_recorder_(std::move(keepalive_statistics_recorder)),
      network_usage_accumulator_(std::move(network_usage_accumulator)),
      first_auth_attempt_(true),
      weak_ptr_factory_(this) {
  DCHECK(delete_callback_);
  if (!base::FeatureList::IsEnabled(features::kNetworkService)) {
    CHECK(!url_loader_client_.internal_state()
                    ->handle()
                    .QuerySignalsState()
                    .peer_remote())
        << "URLLoader must not be used by the renderer when network service is "
        << "disabled, as that skips security checks in ResourceDispatcherHost. "
        << "The only acceptable usage is the browser using SimpleURLLoader.";
  }
  if (report_raw_headers_) {
    options_ |= mojom::kURLLoadOptionSendSSLInfoWithResponse |
                mojom::kURLLoadOptionSendSSLInfoForCertificateError;
  }
  binding_.set_connection_error_handler(
      base::BindOnce(&URLLoader::OnConnectionError, base::Unretained(this)));

  url_request_ = url_request_context_->CreateRequest(
      GURL(request.url), request.priority, this, traffic_annotation);
  url_request_->set_method(request.method);
  url_request_->set_site_for_cookies(request.site_for_cookies);
  url_request_->set_attach_same_site_cookies(request.attach_same_site_cookies);
  url_request_->SetReferrer(ComputeReferrer(request.referrer));
  url_request_->set_referrer_policy(request.referrer_policy);
  url_request_->SetExtraRequestHeaders(request.headers);

  // Resolve elements from request_body and prepare upload data.
  if (request.request_body.get()) {
    scoped_refptr<base::SequencedTaskRunner> task_runner =
        base::CreateSequencedTaskRunnerWithTraits(
            {base::MayBlock(), base::TaskPriority::USER_VISIBLE});
    url_request_->set_upload(
        CreateUploadDataStream(request.request_body.get(), task_runner.get()));

    if (request.enable_upload_progress) {
      upload_progress_tracker_ = std::make_unique<UploadProgressTracker>(
          FROM_HERE,
          base::BindRepeating(&URLLoader::SendUploadProgress,
                              base::Unretained(this)),
          url_request_.get());
    }
  }

  url_request_->set_initiator(request.request_initiator);

  if (request.update_first_party_url_on_redirect) {
    url_request_->set_first_party_url_policy(
        net::URLRequest::UPDATE_FIRST_PARTY_URL_ON_REDIRECT);
  }

  url_request_->SetLoadFlags(request.load_flags);
  if (report_raw_headers_) {
    url_request_->SetRequestHeadersCallback(
        base::Bind(&net::HttpRawRequestHeaders::Assign,
                   base::Unretained(&raw_request_headers_)));
    url_request_->SetResponseHeadersCallback(
        base::Bind(&URLLoader::SetRawResponseHeaders, base::Unretained(this)));
  }

  if (keepalive_ && keepalive_statistics_recorder_)
    keepalive_statistics_recorder_->OnLoadStarted(factory_params_->process_id);

  bool defer = false;
  if (resource_scheduler_client_) {
    resource_scheduler_request_handle_ =
        resource_scheduler_client_->ScheduleRequest(
            !(options_ & network::mojom::kURLLoadOptionSynchronous),
            url_request_.get());
    resource_scheduler_request_handle_->set_resume_callback(
        base::BindRepeating(&URLLoader::ResumeStart, base::Unretained(this)));
    resource_scheduler_request_handle_->WillStartRequest(&defer);
  }
  if (defer)
    url_request_->LogBlockedBy("ResourceScheduler");
  else
    url_request_->Start();
}

URLLoader::~URLLoader() {
  RecordBodyReadFromNetBeforePausedIfNeeded();

  if (keepalive_ && keepalive_statistics_recorder_)
    keepalive_statistics_recorder_->OnLoadFinished(factory_params_->process_id);
}

void URLLoader::FollowRedirect(
    const base::Optional<net::HttpRequestHeaders>& modified_request_headers) {
  DCHECK(!modified_request_headers.has_value()) << "Redirect with modified "
                                                   "headers was not supported "
                                                   "yet. crbug.com/845683";
  if (!url_request_) {
    NotifyCompleted(net::ERR_UNEXPECTED);
    // |this| may have been deleted.
    return;
  }

  url_request_->FollowDeferredRedirect();
}

void URLLoader::ProceedWithResponse() {
  NOTREACHED();
}

void URLLoader::SetPriority(net::RequestPriority priority,
                            int32_t intra_priority_value) {
  if (url_request_ && resource_scheduler_client_) {
    resource_scheduler_client_->ReprioritizeRequest(
        url_request_.get(), priority, intra_priority_value);
  }
}

void URLLoader::PauseReadingBodyFromNet() {
  DVLOG(1) << "URLLoader pauses fetching response body for "
           << (url_request_ ? url_request_->original_url().spec()
                            : "a URL that has completed loading or failed.");

  if (!url_request_)
    return;

  // Please note that we pause reading body in all cases. Even if the URL
  // request indicates that the response was cached, there could still be
  // network activity involved. For example, the response was only partially
  // cached.
  //
  // On the other hand, we only report BodyReadFromNetBeforePaused histogram
  // when we are sure that the response body hasn't been read from cache. This
  // avoids polluting the histogram data with data points from cached responses.
  should_pause_reading_body_ = true;

  // If the data pipe has been set up and the request is in IO pending state,
  // there is a pending read for the response body.
  if (HasDataPipe() && url_request_->status().is_io_pending()) {
    update_body_read_before_paused_ = true;
  } else {
    body_read_before_paused_ = url_request_->GetRawBodyBytes();
  }
}

void URLLoader::ResumeReadingBodyFromNet() {
  DVLOG(1) << "URLLoader resumes fetching response body for "
           << (url_request_ ? url_request_->original_url().spec()
                            : "a URL that has completed loading or failed.");
  should_pause_reading_body_ = false;

  if (paused_reading_body_) {
    paused_reading_body_ = false;
    ReadMore();
  }
}

void URLLoader::OnReceivedRedirect(net::URLRequest* url_request,
                                   const net::RedirectInfo& redirect_info,
                                   bool* defer_redirect) {
  DCHECK(url_request == url_request_.get());
  DCHECK(url_request->status().is_success());

  // Send the redirect response to the client, allowing them to inspect it and
  // optionally follow the redirect.
  *defer_redirect = true;

  scoped_refptr<ResourceResponse> response = new ResourceResponse();
  PopulateResourceResponse(
      url_request_.get(), is_load_timing_enabled_,
      options_ & mojom::kURLLoadOptionSendSSLInfoWithResponse, response.get());
  if (report_raw_headers_) {
    response->head.raw_request_response_info = BuildRawRequestResponseInfo(
        *url_request_, raw_request_headers_, raw_response_headers_.get());
    raw_request_headers_ = net::HttpRawRequestHeaders();
    raw_response_headers_ = nullptr;
  }
  url_loader_client_->OnReceiveRedirect(redirect_info, response->head);
}

void URLLoader::OnAuthRequired(net::URLRequest* unused,
                               net::AuthChallengeInfo* auth_info) {
  if (!network_service_client_) {
    OnAuthCredentials(base::nullopt);
    return;
  }

  if (do_not_prompt_for_login_) {
    OnAuthCredentials(base::nullopt);
    return;
  }

  network::mojom::AuthChallengeResponderPtr auth_challenge_responder;
  auto request = mojo::MakeRequest(&auth_challenge_responder);
  DCHECK(!auth_challenge_responder_binding_.is_bound());
  auth_challenge_responder_binding_.Bind(std::move(request));
  auth_challenge_responder_binding_.set_connection_error_handler(
      base::BindOnce(&URLLoader::DeleteSelf, base::Unretained(this)));
  network_service_client_->OnAuthRequired(
      factory_params_->process_id, render_frame_id_, request_id_,
      url_request_->url(), url_request_->site_for_cookies(),
      first_auth_attempt_, auth_info, resource_type_,
      std::move(auth_challenge_responder));

  first_auth_attempt_ = false;
}

void URLLoader::OnCertificateRequested(net::URLRequest* unused,
                                       net::SSLCertRequestInfo* cert_info) {
  if (!network_service_client_) {
    OnCertificateRequestedResponse(nullptr, std::vector<uint16_t>(), nullptr,
                                   true /* cancel_certificate_selection */);
    return;
  }

  network_service_client_->OnCertificateRequested(
      factory_params_->process_id, render_frame_id_, request_id_, cert_info,
      base::BindOnce(&URLLoader::OnCertificateRequestedResponse,
                     weak_ptr_factory_.GetWeakPtr()));
}

void URLLoader::OnSSLCertificateError(net::URLRequest* request,
                                      const net::SSLInfo& ssl_info,
                                      bool fatal) {
  if (!network_service_client_) {
    OnSSLCertificateErrorResponse(ssl_info, net::ERR_INSECURE_RESPONSE);
    return;
  }
  network_service_client_->OnSSLCertificateError(
      factory_params_->process_id, render_frame_id_, request_id_,
      resource_type_, url_request_->url(), ssl_info, fatal,
      base::Bind(&URLLoader::OnSSLCertificateErrorResponse,
                 weak_ptr_factory_.GetWeakPtr(), ssl_info));
}

void URLLoader::ResumeStart() {
  url_request_->LogUnblocked();
  url_request_->Start();
}

void URLLoader::OnResponseStarted(net::URLRequest* url_request, int net_error) {
  DCHECK(url_request == url_request_.get());

  if (net_error != net::OK) {
    NotifyCompleted(net_error);
    // |this| may have been deleted.
    return;
  }

  // Do not account header bytes when reporting received body bytes to client.
  reported_total_encoded_bytes_ = url_request_->GetTotalReceivedBytes();

  if (resource_scheduler_client_ && url_request->was_fetched_via_proxy() &&
      url_request->was_fetched_via_spdy() &&
      url_request->url().SchemeIs(url::kHttpScheme)) {
    resource_scheduler_client_->OnReceivedSpdyProxiedHttpResponse();
  }

  if (upload_progress_tracker_) {
    upload_progress_tracker_->OnUploadCompleted();
    upload_progress_tracker_ = nullptr;
  }

  response_ = new ResourceResponse();
  PopulateResourceResponse(
      url_request_.get(), is_load_timing_enabled_,
      options_ & mojom::kURLLoadOptionSendSSLInfoWithResponse, response_.get());
  if (report_raw_headers_) {
    response_->head.raw_request_response_info = BuildRawRequestResponseInfo(
        *url_request_, raw_request_headers_, raw_response_headers_.get());
    raw_request_headers_ = net::HttpRawRequestHeaders();
    raw_response_headers_ = nullptr;
  }

  mojo::DataPipe data_pipe(kDefaultAllocationSize);
  response_body_stream_ = std::move(data_pipe.producer_handle);
  consumer_handle_ = std::move(data_pipe.consumer_handle);
  peer_closed_handle_watcher_.Watch(
      response_body_stream_.get(), MOJO_HANDLE_SIGNAL_PEER_CLOSED,
      base::Bind(&URLLoader::OnResponseBodyStreamConsumerClosed,
                 base::Unretained(this)));
  peer_closed_handle_watcher_.ArmOrNotify();

  writable_handle_watcher_.Watch(
      response_body_stream_.get(), MOJO_HANDLE_SIGNAL_WRITABLE,
      base::Bind(&URLLoader::OnResponseBodyStreamReady,
                 base::Unretained(this)));

  if (!(options_ & mojom::kURLLoadOptionSniffMimeType) ||
      !ShouldSniffContent(url_request_.get(), response_.get()))
    SendResponseToClient();

  // Start reading...
  ReadMore();
}

void URLLoader::ReadMore() {
  // Once the MIME type is sniffed, all data is sent as soon as it is read from
  // the network.
  DCHECK(consumer_handle_.is_valid() || !pending_write_);

  if (should_pause_reading_body_) {
    paused_reading_body_ = true;
    return;
  }

  if (!pending_write_.get()) {
    // TODO: we should use the abstractions in MojoAsyncResourceHandler.
    DCHECK_EQ(0u, pending_write_buffer_offset_);
    MojoResult result = NetToMojoPendingBuffer::BeginWrite(
        &response_body_stream_, &pending_write_, &pending_write_buffer_size_);
    if (result != MOJO_RESULT_OK && result != MOJO_RESULT_SHOULD_WAIT) {
      // The response body stream is in a bad state. Bail.
      NotifyCompleted(net::ERR_FAILED);
      return;
    }

    DCHECK_GT(static_cast<uint32_t>(std::numeric_limits<int>::max()),
              pending_write_buffer_size_);
    if (consumer_handle_.is_valid()) {
      DCHECK_GE(pending_write_buffer_size_,
                static_cast<uint32_t>(net::kMaxBytesToSniff));
    }
    if (result == MOJO_RESULT_SHOULD_WAIT) {
      // The pipe is full. We need to wait for it to have more space.
      writable_handle_watcher_.ArmOrNotify();
      return;
    }
  }

  auto buf = base::MakeRefCounted<NetToMojoIOBuffer>(
      pending_write_.get(), pending_write_buffer_offset_);
  int bytes_read;
  url_request_->Read(buf.get(),
                     static_cast<int>(pending_write_buffer_size_ -
                                      pending_write_buffer_offset_),
                     &bytes_read);
  if (url_request_->status().is_io_pending()) {
    // Wait for OnReadCompleted.
  } else {
    DidRead(bytes_read, true);
    // |this| may have been deleted.
  }
}

void URLLoader::DidRead(int num_bytes, bool completed_synchronously) {
  if (num_bytes > 0) {
    pending_write_buffer_offset_ += num_bytes;

    // Only notify client of download progress in case DevTools are attached
    // and we're done sniffing and started sending response.
    if (report_raw_headers_ && !consumer_handle_.is_valid()) {
      int64_t total_encoded_bytes = url_request_->GetTotalReceivedBytes();
      int64_t delta = total_encoded_bytes - reported_total_encoded_bytes_;
      DCHECK_LE(0, delta);
      if (delta)
        url_loader_client_->OnTransferSizeUpdated(delta);
      reported_total_encoded_bytes_ = total_encoded_bytes;
    }
  }
  if (update_body_read_before_paused_) {
    update_body_read_before_paused_ = false;
    body_read_before_paused_ = url_request_->GetRawBodyBytes();
  }

  bool complete_read = true;
  if (consumer_handle_.is_valid()) {
    const std::string& type_hint = response_->head.mime_type;
    std::string new_type;
    bool made_final_decision = net::SniffMimeType(
        pending_write_->buffer(), pending_write_buffer_offset_,
        url_request_->url(), type_hint,
        net::ForceSniffFileUrlsForHtml::kDisabled, &new_type);
    // SniffMimeType() returns false if there is not enough data to determine
    // the mime type. However, even if it returns false, it returns a new type
    // that is probably better than the current one.
    response_->head.mime_type.assign(new_type);

    if (made_final_decision) {
      SendResponseToClient();
    } else {
      complete_read = false;
    }
  }

  if (!url_request_->status().is_success() || num_bytes == 0) {
    CompletePendingWrite();
    NotifyCompleted(url_request_->status().ToNetError());
    // |this| will have been deleted.
    return;
  }

  if (complete_read) {
    CompletePendingWrite();
  }
  if (completed_synchronously) {
    base::ThreadTaskRunnerHandle::Get()->PostTask(
        FROM_HERE,
        base::BindOnce(&URLLoader::ReadMore, weak_ptr_factory_.GetWeakPtr()));
  } else {
    ReadMore();
  }
}

void URLLoader::OnReadCompleted(net::URLRequest* url_request, int bytes_read) {
  DCHECK(url_request == url_request_.get());

  DidRead(bytes_read, false);
  // |this| may have been deleted.
}

net::LoadState URLLoader::GetLoadStateForTesting() const {
  if (!url_request_)
    return net::LOAD_STATE_IDLE;
  return url_request_->GetLoadState().state;
}

void URLLoader::OnAuthCredentials(
    const base::Optional<net::AuthCredentials>& credentials) {
  auth_challenge_responder_binding_.Close();

  if (!url_request_)
    return;

  if (!credentials.has_value()) {
    url_request_->CancelAuth();
  } else {
    url_request_->SetAuth(credentials.value());
  }
}

void URLLoader::NotifyCompleted(int error_code) {
  // Ensure sending the final upload progress message here, since
  // OnResponseCompleted can be called without OnResponseStarted on cancellation
  // or error cases.
  if (upload_progress_tracker_) {
    upload_progress_tracker_->OnUploadCompleted();
    upload_progress_tracker_ = nullptr;
  }

  if (consumer_handle_.is_valid())
    SendResponseToClient();

  URLLoaderCompletionStatus status;
  status.error_code = error_code;
  if (error_code == net::ERR_QUIC_PROTOCOL_ERROR) {
    net::NetErrorDetails details;
    url_request_->PopulateNetErrorDetails(&details);
    status.extended_error_code = details.quic_connection_error;
  }
  status.exists_in_cache = url_request_->response_info().was_cached;
  status.completion_time = base::TimeTicks::Now();
  status.encoded_data_length = url_request_->GetTotalReceivedBytes();
  status.encoded_body_length = url_request_->GetRawBodyBytes();
  status.decoded_body_length = total_written_bytes_;

  if ((options_ & mojom::kURLLoadOptionSendSSLInfoForCertificateError) &&
      net::IsCertStatusError(url_request_->ssl_info().cert_status) &&
      !net::IsCertStatusMinorError(url_request_->ssl_info().cert_status)) {
    status.ssl_info = url_request_->ssl_info();
  }

  if (network_usage_accumulator_) {
    network_usage_accumulator_->OnBytesTransferred(
        factory_params_->process_id, render_frame_id_,
        url_request_->GetTotalReceivedBytes(),
        url_request_->GetTotalSentBytes());
  }

  url_loader_client_->OnComplete(status);
  DeleteSelf();
}

void URLLoader::OnConnectionError() {
  NotifyCompleted(net::ERR_FAILED);
}

void URLLoader::OnResponseBodyStreamConsumerClosed(MojoResult result) {
  NotifyCompleted(net::ERR_FAILED);
}

void URLLoader::OnResponseBodyStreamReady(MojoResult result) {
  if (result != MOJO_RESULT_OK) {
    NotifyCompleted(net::ERR_FAILED);
    return;
  }

  ReadMore();
}

void URLLoader::DeleteSelf() {
  std::move(delete_callback_).Run(this);
}

void URLLoader::SendResponseToClient() {
  mojom::DownloadedTempFilePtr downloaded_file_ptr;
  url_loader_client_->OnReceiveResponse(response_->head,
                                        std::move(downloaded_file_ptr));

  net::IOBufferWithSize* metadata =
      url_request_->response_info().metadata.get();
  if (metadata) {
    const uint8_t* data = reinterpret_cast<const uint8_t*>(metadata->data());

    url_loader_client_->OnReceiveCachedMetadata(
        std::vector<uint8_t>(data, data + metadata->size()));
  }

  url_loader_client_->OnStartLoadingResponseBody(std::move(consumer_handle_));
  response_ = nullptr;
}

void URLLoader::CompletePendingWrite() {
  response_body_stream_ =
      pending_write_->Complete(pending_write_buffer_offset_);
  total_written_bytes_ += pending_write_buffer_offset_;
  pending_write_ = nullptr;
  pending_write_buffer_offset_ = 0;
}

void URLLoader::SetRawResponseHeaders(
    scoped_refptr<const net::HttpResponseHeaders> headers) {
  raw_response_headers_ = headers;
}

void URLLoader::SendUploadProgress(const net::UploadProgress& progress) {
  url_loader_client_->OnUploadProgress(
      progress.position(), progress.size(),
      base::BindOnce(&URLLoader::OnUploadProgressACK,
                     weak_ptr_factory_.GetWeakPtr()));
}

void URLLoader::OnUploadProgressACK() {
  if (upload_progress_tracker_)
    upload_progress_tracker_->OnAckReceived();
}

void URLLoader::OnSSLCertificateErrorResponse(const net::SSLInfo& ssl_info,
                                              int net_error) {
  // The request can be NULL if it was cancelled by the client.
  if (!url_request_ || !url_request_->is_pending())
    return;

  if (net_error == net::OK) {
    url_request_->ContinueDespiteLastError();
    return;
  }

  url_request_->CancelWithSSLError(net_error, ssl_info);
}

void URLLoader::OnCertificateRequestedResponse(
    const scoped_refptr<net::X509Certificate>& x509_certificate,
    const std::vector<uint16_t>& algorithm_preferences,
    mojom::SSLPrivateKeyPtr ssl_private_key,
    bool cancel_certificate_selection) {
  if (cancel_certificate_selection) {
    url_request_->CancelWithError(net::ERR_SSL_CLIENT_AUTH_CERT_NEEDED);
  } else {
    if (x509_certificate) {
      scoped_refptr<net::SSLPrivateKey> key(new SSLPrivateKeyInternal(
          algorithm_preferences, std::move(ssl_private_key)));
      url_request_->ContinueWithCertificate(std::move(x509_certificate),
                                            std::move(key));
    } else {
      url_request_->ContinueWithCertificate(nullptr, nullptr);
    }
  }
}

bool URLLoader::HasDataPipe() const {
  return pending_write_ || response_body_stream_.is_valid();
}

void URLLoader::RecordBodyReadFromNetBeforePausedIfNeeded() {
  if (!url_request_)
    return;

  if (update_body_read_before_paused_)
    body_read_before_paused_ = url_request_->GetRawBodyBytes();
  if (body_read_before_paused_ != -1) {
    if (!url_request_->was_cached()) {
      UMA_HISTOGRAM_COUNTS_1M("Network.URLLoader.BodyReadFromNetBeforePaused",
                              body_read_before_paused_);
    } else {
      DVLOG(1) << "The request has been paused, but "
               << "Network.URLLoader.BodyReadFromNetBeforePaused is not "
               << "reported because the response body may be from cache. "
               << "body_read_before_paused_: " << body_read_before_paused_;
    }
  }
}

}  // namespace network
