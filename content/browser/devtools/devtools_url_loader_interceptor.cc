// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/devtools/devtools_url_loader_interceptor.h"
#include "base/base64.h"
#include "base/no_destructor.h"
#include "base/strings/stringprintf.h"
#include "base/time/time.h"
#include "base/unguessable_token.h"
#include "content/browser/devtools/protocol/network_handler.h"
#include "content/browser/frame_host/frame_tree_node.h"
#include "content/browser/loader/navigation_loader_util.h"
#include "content/public/browser/browser_thread.h"
#include "mojo/public/cpp/bindings/binding_set.h"
#include "mojo/public/cpp/system/data_pipe_drainer.h"
#include "net/base/mime_sniffer.h"
#include "net/http/http_util.h"
#include "net/url_request/url_request.h"
#include "services/network/public/cpp/resource_request_body.h"
#include "third_party/blink/public/platform/resource_request_blocked_reason.h"

namespace content {

namespace {

using RequestInterceptedCallback =
    DevToolsNetworkInterceptor::RequestInterceptedCallback;
using ContinueInterceptedRequestCallback =
    DevToolsNetworkInterceptor::ContinueInterceptedRequestCallback;
using GetResponseBodyForInterceptionCallback =
    DevToolsNetworkInterceptor::GetResponseBodyForInterceptionCallback;
using TakeResponseBodyPipeCallback =
    DevToolsNetworkInterceptor::TakeResponseBodyPipeCallback;
using Modifications = DevToolsNetworkInterceptor::Modifications;
using InterceptionStage = DevToolsNetworkInterceptor::InterceptionStage;
using protocol::Response;
using protocol::Network::AuthChallengeResponse;
using GlobalRequestId = std::tuple<int32_t, int32_t, int32_t>;

struct CreateLoaderParameters {
  CreateLoaderParameters(
      int32_t routing_id,
      int32_t request_id,
      uint32_t options,
      network::ResourceRequest request,
      net::MutableNetworkTrafficAnnotationTag traffic_annotation)
      : routing_id(routing_id),
        request_id(request_id),
        options(options),
        request(request),
        traffic_annotation(traffic_annotation) {}

  const int32_t routing_id;
  const int32_t request_id;
  const uint32_t options;
  network::ResourceRequest request;
  const net::MutableNetworkTrafficAnnotationTag traffic_annotation;
};

class BodyReader : public mojo::DataPipeDrainer::Client {
 public:
  explicit BodyReader(base::OnceClosure download_complete_callback)
      : download_complete_callback_(std::move(download_complete_callback)) {}

  void StartReading(mojo::ScopedDataPipeConsumerHandle body);

  void AddCallback(
      std::unique_ptr<GetResponseBodyForInterceptionCallback> callback) {
    callbacks_.push_back(std::move(callback));
    if (data_complete_) {
      DCHECK_EQ(1UL, callbacks_.size());
      BrowserThread::PostTask(
          BrowserThread::UI, FROM_HERE,
          base::BindOnce(&BodyReader::DispatchBodyOnUI, std::move(callbacks_),
                         encoded_body_));
    }
  }

  bool data_complete() const { return data_complete_; }
  const std::string& body() const { return body_; }

  void CancelWithError(std::string error) {
    BrowserThread::PostTask(
        BrowserThread::UI, FROM_HERE,
        base::BindOnce(&BodyReader::DispatchErrorOnUI, std::move(callbacks_),
                       std::move(error)));
  }

 private:
  using CallbackVector =
      std::vector<std::unique_ptr<GetResponseBodyForInterceptionCallback>>;
  static void DispatchBodyOnUI(const CallbackVector& callbacks,
                               const std::string& body);
  static void DispatchErrorOnUI(const CallbackVector& callbacks,
                                const std::string& error);

  void OnDataAvailable(const void* data, size_t num_bytes) override {
    DCHECK(!data_complete_);
    body_.append(std::string(static_cast<const char*>(data), num_bytes));
  }

  void OnDataComplete() override;

  std::unique_ptr<mojo::DataPipeDrainer> body_pipe_drainer_;
  CallbackVector callbacks_;
  base::OnceClosure download_complete_callback_;
  std::string body_;
  std::string encoded_body_;
  bool data_complete_ = false;
};

void BodyReader::StartReading(mojo::ScopedDataPipeConsumerHandle body) {
  DCHECK(!callbacks_.empty());
  DCHECK(!body_pipe_drainer_);
  DCHECK(!data_complete_);

  body_pipe_drainer_.reset(new mojo::DataPipeDrainer(this, std::move(body)));
}

void BodyReader::OnDataComplete() {
  DCHECK(!data_complete_);
  data_complete_ = true;
  body_pipe_drainer_.reset();
  // TODO(caseq): only encode if necessary.
  base::Base64Encode(body_, &encoded_body_);
  BrowserThread::PostTask(BrowserThread::UI, FROM_HERE,
                          base::BindOnce(&BodyReader::DispatchBodyOnUI,
                                         std::move(callbacks_), encoded_body_));
  std::move(download_complete_callback_).Run();
}

// static
void BodyReader::DispatchBodyOnUI(const CallbackVector& callbacks,
                                  const std::string& encoded_body) {
  for (const auto& cb : callbacks)
    cb->sendSuccess(encoded_body, true);
}

// static
void BodyReader::DispatchErrorOnUI(const CallbackVector& callbacks,
                                   const std::string& error) {
  for (const auto& cb : callbacks)
    cb->sendFailure(Response::Error(error));
}

struct ResponseMetadata {
  ResponseMetadata() = default;
  explicit ResponseMetadata(const network::ResourceResponseHead& head)
      : head(head) {}

  network::ResourceResponseHead head;
  std::unique_ptr<net::RedirectInfo> redirect_info;
  network::mojom::DownloadedTempFilePtr downloaded_file;
  std::vector<uint8_t> cached_metadata;
  size_t encoded_length = 0;
  size_t transfer_size = 0;
  network::URLLoaderCompletionStatus status;
};

class InterceptionJob : public network::mojom::URLLoaderClient,
                        public network::mojom::URLLoader {
 public:
  static InterceptionJob* FindByRequestId(
      const GlobalRequestId& global_req_id) {
    const auto& map = GetInterceptionJobMap();
    auto it = map.find(global_req_id);
    return it == map.end() ? nullptr : it->second;
  }

  InterceptionJob(DevToolsURLLoaderInterceptor::Impl* interceptor,
                  const std::string& id,
                  const base::UnguessableToken& frame_token,
                  int32_t process_id,
                  std::unique_ptr<CreateLoaderParameters> create_loader_params,
                  bool is_download,
                  network::mojom::URLLoaderRequest loader_request,
                  network::mojom::URLLoaderClientPtr client,
                  network::mojom::URLLoaderFactoryPtr target_factory);

  void GetResponseBody(
      std::unique_ptr<GetResponseBodyForInterceptionCallback> callback);
  void TakeResponseBodyPipe(TakeResponseBodyPipeCallback callback);
  void ContinueInterceptedRequest(
      std::unique_ptr<Modifications> modifications,
      std::unique_ptr<ContinueInterceptedRequestCallback> callback);
  void Detach();

  void OnAuthRequest(
      const scoped_refptr<net::AuthChallengeInfo>& auth_info,
      DevToolsURLLoaderInterceptor::HandleAuthRequestCallback callback);

 private:
  static std::map<GlobalRequestId, InterceptionJob*>& GetInterceptionJobMap() {
    DCHECK_CURRENTLY_ON(content::BrowserThread::IO);
    static base::NoDestructor<std::map<GlobalRequestId, InterceptionJob*>> inst;
    return *inst;
  }

  ~InterceptionJob() override {
    size_t erased = GetInterceptionJobMap().erase(global_req_id_);
    DCHECK_EQ(1lu, erased);
  }

  Response InnerContinueRequest(std::unique_ptr<Modifications> modifications);
  Response ProcessAuthResponse(AuthChallengeResponse* auth_challenge_response);
  Response ProcessResponseOverride(const std::string& response);
  Response ProcessRedirectByClient(const std::string& location);
  void SendResponse(const base::StringPiece& body);
  void ApplyModificationsToRequest(
      std::unique_ptr<Modifications> modifications);

  void StartRequest();
  void CancelRequest();
  void Shutdown();

  std::unique_ptr<InterceptedRequestInfo> BuildRequestInfo(
      const network::ResourceResponseHead* head);
  void NotifyClient(std::unique_ptr<InterceptedRequestInfo> request_info);

  void ResponseBodyComplete();

  bool ShouldBypassForResponse() const {
    if (state_ == State::kResponseTaken)
      return false;
    DCHECK_EQ(!!response_metadata_, !!body_reader_);
    DCHECK_EQ(state_, State::kResponseReceived);
    return !response_metadata_;
  }

  // network::mojom::URLLoader methods
  void FollowRedirect(const base::Optional<net::HttpRequestHeaders>&
                          modified_request_headers) override;
  void ProceedWithResponse() override;
  void SetPriority(net::RequestPriority priority,
                   int32_t intra_priority_value) override;
  void PauseReadingBodyFromNet() override;
  void ResumeReadingBodyFromNet() override;

  // network::mojom::URLLoaderClient methods
  void OnReceiveResponse(
      const network::ResourceResponseHead& head,
      network::mojom::DownloadedTempFilePtr downloaded_file) override;
  void OnReceiveRedirect(const net::RedirectInfo& redirect_info,
                         const network::ResourceResponseHead& head) override;
  void OnDataDownloaded(int64_t data_length, int64_t encoded_length) override;
  void OnUploadProgress(int64_t current_position,
                        int64_t total_size,
                        OnUploadProgressCallback callback) override;
  void OnReceiveCachedMetadata(const std::vector<uint8_t>& data) override;
  void OnTransferSizeUpdated(int32_t transfer_size_diff) override;
  void OnStartLoadingResponseBody(
      mojo::ScopedDataPipeConsumerHandle body) override;
  void OnComplete(const network::URLLoaderCompletionStatus& status) override;

  bool CanGetResponseBody(std::string* error_reason);

  const std::string id_;
  const GlobalRequestId global_req_id_;
  const base::UnguessableToken frame_token_;
  const base::TimeTicks start_ticks_;
  const base::Time start_time_;
  const bool report_upload_;

  DevToolsURLLoaderInterceptor::Impl* interceptor_;
  InterceptionStage stage_;

  std::unique_ptr<CreateLoaderParameters> create_loader_params_;
  const bool is_download_;

  mojo::Binding<network::mojom::URLLoaderClient> client_binding_;
  mojo::Binding<network::mojom::URLLoader> loader_binding_;

  network::mojom::URLLoaderClientPtr client_;
  network::mojom::URLLoaderPtr loader_;
  network::mojom::URLLoaderFactoryPtr target_factory_;

  enum State {
    kNotStarted,
    kRequestSent,
    kRedirectReceived,
    kAuthRequired,
    kResponseReceived,
    kResponseTaken,
  };

  State state_;
  bool waiting_for_resolution_;

  std::unique_ptr<BodyReader> body_reader_;
  std::unique_ptr<ResponseMetadata> response_metadata_;

  base::Optional<std::pair<net::RequestPriority, int32_t>> priority_;
  DevToolsURLLoaderInterceptor::HandleAuthRequestCallback
      pending_auth_callback_;
  TakeResponseBodyPipeCallback pending_response_body_pipe_callback_;

  DISALLOW_COPY_AND_ASSIGN(InterceptionJob);
};

}  // namespace

class DevToolsURLLoaderInterceptor::Impl
    : public base::SupportsWeakPtr<DevToolsURLLoaderInterceptor::Impl> {
 public:
  explicit Impl(RequestInterceptedCallback callback)
      : request_intercepted_callback_(callback) {}
  ~Impl() {
    for (auto const& entry : jobs_)
      entry.second->Detach();
  }

  void CreateJob(const base::UnguessableToken& frame_token,
                 int32_t process_id,
                 bool is_download,
                 std::unique_ptr<CreateLoaderParameters> create_params,
                 network::mojom::URLLoaderRequest loader_request,
                 network::mojom::URLLoaderClientPtr client,
                 network::mojom::URLLoaderFactoryPtr target_factory) {
    DCHECK(!frame_token.is_empty());

    static int last_id = 0;

    std::string id = base::StringPrintf("interception-job-%d", ++last_id);
    InterceptionJob* job = new InterceptionJob(
        this, id, frame_token, process_id, std::move(create_params),
        is_download, std::move(loader_request), std::move(client),
        std::move(target_factory));
    jobs_.emplace(std::move(id), job);
  }

  void SetPatterns(std::vector<DevToolsNetworkInterceptor::Pattern> patterns) {
    patterns_ = std::move(patterns);
  }

  InterceptionStage GetInterceptionStage(const GURL& url,
                                         ResourceType resource_type) const {
    InterceptionStage stage = InterceptionStage::DONT_INTERCEPT;
    std::string url_str = protocol::NetworkHandler::ClearUrlRef(url).spec();
    for (const auto& pattern : patterns_) {
      if (pattern.Matches(url_str, resource_type))
        stage |= pattern.interception_stage;
    }
    return stage;
  }

  void GetResponseBody(
      const std::string& interception_id,
      std::unique_ptr<GetResponseBodyForInterceptionCallback> callback) {
    if (InterceptionJob* job = FindJob(interception_id, &callback))
      job->GetResponseBody(std::move(callback));
  }

  void TakeResponseBodyPipe(
      const std::string& interception_id,
      DevToolsNetworkInterceptor::TakeResponseBodyPipeCallback callback) {
    auto it = jobs_.find(interception_id);
    if (it == jobs_.end()) {
      std::move(callback).Run(
          protocol::Response::InvalidParams("Invalid InterceptionId."),
          mojo::ScopedDataPipeConsumerHandle(), std::string());
      return;
    }
    it->second->TakeResponseBodyPipe(std::move(callback));
  }

  void ContinueInterceptedRequest(
      const std::string& interception_id,
      std::unique_ptr<Modifications> modifications,
      std::unique_ptr<ContinueInterceptedRequestCallback> callback) {
    if (InterceptionJob* job = FindJob(interception_id, &callback)) {
      job->ContinueInterceptedRequest(std::move(modifications),
                                      std::move(callback));
    }
  }

 private:
  friend class content::InterceptionJob;

  template <typename Callback>
  InterceptionJob* FindJob(const std::string& id,
                           std::unique_ptr<Callback>* callback) {
    auto it = jobs_.find(id);
    if (it != jobs_.end())
      return it->second;
    BrowserThread::PostTask(
        BrowserThread::UI, FROM_HERE,
        base::BindOnce(
            &Callback::sendFailure, std::move(*callback),
            protocol::Response::InvalidParams("Invalid InterceptionId.")));
    return nullptr;
  }

  void RemoveJob(const std::string& id) { jobs_.erase(id); }

  std::map<std::string, InterceptionJob*> jobs_;
  RequestInterceptedCallback request_intercepted_callback_;
  std::vector<DevToolsNetworkInterceptor::Pattern> patterns_;

  DISALLOW_COPY_AND_ASSIGN(Impl);
};

class DevToolsURLLoaderFactoryProxy : public network::mojom::URLLoaderFactory {
 public:
  DevToolsURLLoaderFactoryProxy(
      const base::UnguessableToken& frame_token,
      int32_t process_id,
      bool is_download,
      network::mojom::URLLoaderFactoryRequest loader_request,
      network::mojom::URLLoaderFactoryPtrInfo target_factory_info,
      base::WeakPtr<DevToolsURLLoaderInterceptor::Impl> interceptor);
  ~DevToolsURLLoaderFactoryProxy() override;

 private:
  // network::mojom::URLLoaderFactory implementation
  void CreateLoaderAndStart(network::mojom::URLLoaderRequest loader,
                            int32_t routing_id,
                            int32_t request_id,
                            uint32_t options,
                            const network::ResourceRequest& request,
                            network::mojom::URLLoaderClientPtr client,
                            const net::MutableNetworkTrafficAnnotationTag&
                                traffic_annotation) override;
  void Clone(network::mojom::URLLoaderFactoryRequest request) override;

  void StartOnIO(network::mojom::URLLoaderFactoryRequest loader_request,
                 network::mojom::URLLoaderFactoryPtrInfo target_factory_info);
  void OnProxyBindingError();
  void OnTargetFactoryError();

  const base::UnguessableToken frame_token_;
  const int32_t process_id_;
  const bool is_download_;

  network::mojom::URLLoaderFactoryPtr target_factory_;
  base::WeakPtr<DevToolsURLLoaderInterceptor::Impl> interceptor_;
  mojo::BindingSet<network::mojom::URLLoaderFactory> bindings_;

  SEQUENCE_CHECKER(sequence_checker_);
};

DevToolsURLLoaderFactoryProxy::DevToolsURLLoaderFactoryProxy(
    const base::UnguessableToken& frame_token,
    int32_t process_id,
    bool is_download,
    network::mojom::URLLoaderFactoryRequest loader_request,
    network::mojom::URLLoaderFactoryPtrInfo target_factory_info,
    base::WeakPtr<DevToolsURLLoaderInterceptor::Impl> interceptor)
    : frame_token_(frame_token),
      process_id_(process_id),
      is_download_(is_download),
      interceptor_(std::move(interceptor)) {
  DETACH_FROM_SEQUENCE(sequence_checker_);
  BrowserThread::PostTask(
      BrowserThread::IO, FROM_HERE,
      base::BindOnce(&DevToolsURLLoaderFactoryProxy::StartOnIO,
                     base::Unretained(this), std::move(loader_request),
                     std::move(target_factory_info)));
}

DevToolsURLLoaderFactoryProxy::~DevToolsURLLoaderFactoryProxy() {}

void DevToolsURLLoaderFactoryProxy::CreateLoaderAndStart(
    network::mojom::URLLoaderRequest loader,
    int32_t routing_id,
    int32_t request_id,
    uint32_t options,
    const network::ResourceRequest& request,
    network::mojom::URLLoaderClientPtr client,
    const net::MutableNetworkTrafficAnnotationTag& traffic_annotation) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  DevToolsURLLoaderInterceptor::Impl* interceptor = interceptor_.get();
  if (!interceptor_) {
    target_factory_->CreateLoaderAndStart(
        std::move(loader), routing_id, request_id, options, request,
        std::move(client), traffic_annotation);
    return;
  }
  auto creation_params = std::make_unique<CreateLoaderParameters>(
      routing_id, request_id, options, request, traffic_annotation);
  network::mojom::URLLoaderFactoryPtr factory_clone;
  target_factory_->Clone(MakeRequest(&factory_clone));
  interceptor->CreateJob(frame_token_, process_id_, is_download_,
                         std::move(creation_params), std::move(loader),
                         std::move(client), std::move(factory_clone));
}

void DevToolsURLLoaderFactoryProxy::StartOnIO(
    network::mojom::URLLoaderFactoryRequest loader_request,
    network::mojom::URLLoaderFactoryPtrInfo target_factory_info) {
  target_factory_.Bind(std::move(target_factory_info));
  target_factory_.set_connection_error_handler(
      base::BindOnce(&DevToolsURLLoaderFactoryProxy::OnTargetFactoryError,
                     base::Unretained(this)));

  bindings_.AddBinding(this, std::move(loader_request));
  bindings_.set_connection_error_handler(
      base::BindRepeating(&DevToolsURLLoaderFactoryProxy::OnProxyBindingError,
                          base::Unretained(this)));
}

void DevToolsURLLoaderFactoryProxy::Clone(
    network::mojom::URLLoaderFactoryRequest request) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  bindings_.AddBinding(this, std::move(request));
}

void DevToolsURLLoaderFactoryProxy::OnTargetFactoryError() {
  delete this;
}

void DevToolsURLLoaderFactoryProxy::OnProxyBindingError() {
  if (bindings_.empty())
    delete this;
}

// static
void DevToolsURLLoaderInterceptor::HandleAuthRequest(
    int32_t process_id,
    int32_t routing_id,
    int32_t request_id,
    const scoped_refptr<net::AuthChallengeInfo>& auth_info,
    HandleAuthRequestCallback callback) {
  GlobalRequestId req_id = std::make_tuple(process_id, routing_id, request_id);
  if (auto* job = InterceptionJob::FindByRequestId(req_id))
    job->OnAuthRequest(auth_info, std::move(callback));
  else
    std::move(callback).Run(true, base::nullopt);
}

DevToolsURLLoaderInterceptor::DevToolsURLLoaderInterceptor(
    FrameTreeNode* const local_root,
    RequestInterceptedCallback callback)
    : local_root_(local_root),
      enabled_(false),
      impl_(new DevToolsURLLoaderInterceptor::Impl(std::move(callback)),
            base::OnTaskRunnerDeleter(
                BrowserThread::GetTaskRunnerForThread(BrowserThread::IO))),
      weak_impl_(impl_->AsWeakPtr()) {}

DevToolsURLLoaderInterceptor::~DevToolsURLLoaderInterceptor() {
  UpdateSubresourceLoaderFactories();
};

void DevToolsURLLoaderInterceptor::UpdateSubresourceLoaderFactories() {
  base::queue<FrameTreeNode*> queue;
  queue.push(local_root_);
  while (!queue.empty()) {
    FrameTreeNode* node = queue.front();
    queue.pop();
    RenderFrameHostImpl* host = node->current_frame_host();
    if (node != local_root_ && host->IsCrossProcessSubframe())
      continue;
    host->UpdateSubresourceLoaderFactories();
    for (size_t i = 0; i < node->child_count(); ++i)
      queue.push(node->child_at(i));
  }
}

void DevToolsURLLoaderInterceptor::SetPatterns(
    std::vector<DevToolsNetworkInterceptor::Pattern> patterns) {
  if (enabled_ != !!patterns.size()) {
    enabled_ = !!patterns.size();
    UpdateSubresourceLoaderFactories();
  }
  BrowserThread::PostTask(
      BrowserThread::IO, FROM_HERE,
      base::BindOnce(&Impl::SetPatterns, base::Unretained(impl_.get()),
                     std::move(patterns)));
}

void DevToolsURLLoaderInterceptor::GetResponseBody(
    const std::string& interception_id,
    std::unique_ptr<GetResponseBodyForInterceptionCallback> callback) {
  BrowserThread::PostTask(
      BrowserThread::IO, FROM_HERE,
      base::BindOnce(&Impl::GetResponseBody, base::Unretained(impl_.get()),
                     interception_id, std::move(callback)));
}

void DevToolsURLLoaderInterceptor::TakeResponseBodyPipe(
    const std::string& interception_id,
    DevToolsNetworkInterceptor::TakeResponseBodyPipeCallback callback) {
  BrowserThread::PostTask(
      BrowserThread::IO, FROM_HERE,
      base::BindOnce(&Impl::TakeResponseBodyPipe, base::Unretained(impl_.get()),
                     interception_id, std::move(callback)));
}

void DevToolsURLLoaderInterceptor::ContinueInterceptedRequest(
    const std::string& interception_id,
    std::unique_ptr<Modifications> modifications,
    std::unique_ptr<ContinueInterceptedRequestCallback> callback) {
  BrowserThread::PostTask(
      BrowserThread::IO, FROM_HERE,
      base::BindOnce(&Impl::ContinueInterceptedRequest,
                     base::Unretained(impl_.get()), interception_id,
                     std::move(modifications), std::move(callback)));
}

bool DevToolsURLLoaderInterceptor::CreateProxyForInterception(
    const base::UnguessableToken frame_token,
    int process_id,
    bool is_download,
    network::mojom::URLLoaderFactoryRequest* request) const {
  if (!enabled_)
    return false;
  network::mojom::URLLoaderFactoryRequest original_request =
      std::move(*request);
  network::mojom::URLLoaderFactoryPtrInfo target_ptr_info;
  *request = MakeRequest(&target_ptr_info);

  new DevToolsURLLoaderFactoryProxy(frame_token, process_id, is_download,
                                    std::move(original_request),
                                    std::move(target_ptr_info), weak_impl_);
  return true;
}

InterceptionJob::InterceptionJob(
    DevToolsURLLoaderInterceptor::Impl* interceptor,
    const std::string& id,
    const base::UnguessableToken& frame_token,
    int process_id,
    std::unique_ptr<CreateLoaderParameters> create_loader_params,
    bool is_download,
    network::mojom::URLLoaderRequest loader_request,
    network::mojom::URLLoaderClientPtr client,
    network::mojom::URLLoaderFactoryPtr target_factory)
    : id_(std::move(id)),
      global_req_id_(
          std::make_tuple(process_id,
                          create_loader_params->request.render_frame_id,
                          create_loader_params->request_id)),
      frame_token_(frame_token),
      start_ticks_(base::TimeTicks::Now()),
      start_time_(base::Time::Now()),
      report_upload_(!!create_loader_params->request.request_body),
      interceptor_(interceptor),
      create_loader_params_(std::move(create_loader_params)),
      is_download_(is_download),
      client_binding_(this),
      loader_binding_(this),
      client_(std::move(client)),
      target_factory_(std::move(target_factory)),
      state_(kNotStarted),
      waiting_for_resolution_(false) {
  const network::ResourceRequest& request = create_loader_params_->request;
  stage_ = interceptor_->GetInterceptionStage(
      request.url, static_cast<ResourceType>(request.resource_type));

  loader_binding_.Bind(std::move(loader_request));
  loader_binding_.set_connection_error_handler(
      base::BindOnce(&InterceptionJob::Shutdown, base::Unretained(this)));

  auto& job_map = GetInterceptionJobMap();
  bool inserted = job_map.emplace(global_req_id_, this).second;
  DCHECK(inserted);

  if (stage_ & InterceptionStage::REQUEST) {
    NotifyClient(BuildRequestInfo(nullptr));
    return;
  }

  StartRequest();
}

bool InterceptionJob::CanGetResponseBody(std::string* error_reason) {
  if (!(stage_ & InterceptionStage::RESPONSE)) {
    *error_reason =
        "Can only get response body on HeadersReceived pattern matched "
        "requests.";
    return false;
  }
  if (state_ != State::kResponseReceived || !waiting_for_resolution_) {
    *error_reason =
        "Can only get response body on requests captured after headers "
        "received.";
    return false;
  }
  return true;
}

void InterceptionJob::GetResponseBody(
    std::unique_ptr<GetResponseBodyForInterceptionCallback> callback) {
  std::string error_reason;
  if (!CanGetResponseBody(&error_reason)) {
    BrowserThread::PostTask(
        BrowserThread::UI, FROM_HERE,
        base::BindOnce(&GetResponseBodyForInterceptionCallback::sendFailure,
                       std::move(callback),
                       Response::Error(std::move(error_reason))));
    return;
  }
  if (!body_reader_) {
    body_reader_ = std::make_unique<BodyReader>(base::BindOnce(
        &InterceptionJob::ResponseBodyComplete, base::Unretained(this)));
    client_binding_.ResumeIncomingMethodCallProcessing();
    loader_->ResumeReadingBodyFromNet();
  }
  body_reader_->AddCallback(std::move(callback));
}

void InterceptionJob::TakeResponseBodyPipe(
    TakeResponseBodyPipeCallback callback) {
  std::string error_reason;
  if (!CanGetResponseBody(&error_reason)) {
    BrowserThread::PostTask(
        BrowserThread::UI, FROM_HERE,
        base::BindOnce(std::move(callback),
                       Response::Error(std::move(error_reason)),
                       mojo::ScopedDataPipeConsumerHandle(), std::string()));
    return;
  }
  DCHECK_EQ(state_, State::kResponseReceived);
  DCHECK(!!response_metadata_);
  state_ = State::kResponseTaken;
  pending_response_body_pipe_callback_ = std::move(callback);
  client_binding_.ResumeIncomingMethodCallProcessing();
  loader_->ResumeReadingBodyFromNet();
}

void InterceptionJob::ContinueInterceptedRequest(
    std::unique_ptr<Modifications> modifications,
    std::unique_ptr<ContinueInterceptedRequestCallback> callback) {
  Response response = InnerContinueRequest(std::move(modifications));
  // |this| may be destroyed at this point.
  bool success = response.isSuccess();
  base::OnceClosure task =
      success ? base::BindOnce(&ContinueInterceptedRequestCallback::sendSuccess,
                               std::move(callback))
              : base::BindOnce(&ContinueInterceptedRequestCallback::sendFailure,
                               std::move(callback), std::move(response));
  BrowserThread::PostTask(BrowserThread::UI, FROM_HERE, std::move(task));
}

void InterceptionJob::Detach() {
  stage_ = InterceptionStage::DONT_INTERCEPT;
  interceptor_ = nullptr;
  if (!waiting_for_resolution_)
    return;
  if (state_ == State::kAuthRequired) {
    state_ = State::kRequestSent;
    waiting_for_resolution_ = false;
    std::move(pending_auth_callback_).Run(true, base::nullopt);
    return;
  }
  InnerContinueRequest(std::make_unique<Modifications>());
}

Response InterceptionJob::InnerContinueRequest(
    std::unique_ptr<Modifications> modifications) {
  if (!waiting_for_resolution_)
    return Response::Error("Invalid state for continueInterceptedRequest");
  waiting_for_resolution_ = false;

  if (state_ == State::kAuthRequired) {
    if (!modifications->auth_challenge_response.isJust())
      return Response::InvalidParams("authChallengeResponse required.");
    return ProcessAuthResponse(
        modifications->auth_challenge_response.fromJust());
  }
  if (modifications->auth_challenge_response.isJust())
    return Response::InvalidParams("authChallengeResponse not expected.");

  if (modifications->mark_as_canceled || modifications->error_reason) {
    int error = modifications->error_reason
                    ? *modifications->error_reason
                    : (modifications->mark_as_canceled ? net::ERR_ABORTED
                                                       : net::ERR_FAILED);
    network::URLLoaderCompletionStatus status(error);
    status.completion_time = base::TimeTicks::Now();
    if (modifications->error_reason == net::ERR_BLOCKED_BY_CLIENT) {
      // So we know that these modifications originated from devtools
      // (also known as inspector), and can therefore annotate the
      // request. We only do this for one specific error code thus
      // far, to minimize risk of breaking other usages.
      status.extended_error_code =
          static_cast<int>(blink::ResourceRequestBlockedReason::kInspector);
    }
    client_->OnComplete(status);
    Shutdown();
    return Response::OK();
  }

  if (modifications->raw_response)
    return ProcessResponseOverride(*modifications->raw_response);

  if (state_ == State::kRedirectReceived) {
    // TODO(caseq): report error if other modifications are present.
    if (modifications->modified_url.isJust()) {
      std::string location = modifications->modified_url.fromJust();
      CancelRequest();
      auto* headers = response_metadata_->head.headers.get();
      headers->RemoveHeader("location");
      headers->AddHeader("location: " + location);
      return ProcessRedirectByClient(location);
    }
    client_->OnReceiveRedirect(*response_metadata_->redirect_info,
                               response_metadata_->head);
    return Response::OK();
  }

  if (body_reader_) {
    if (body_reader_->data_complete())
      SendResponse(body_reader_->body());

    // There are read callbacks pending, so let the reader do its job and come
    // back when it's done.
    return Response::OK();
  }

  if (response_metadata_) {
    if (state_ == State::kResponseTaken) {
      return Response::InvalidParams(
          "Unable to continue request as is after body is taken");
    }
    // TODO(caseq): report error if other modifications are present.
    DCHECK_EQ(State::kResponseReceived, state_);
    DCHECK(!body_reader_);
    client_->OnReceiveResponse(response_metadata_->head,
                               std::move(response_metadata_->downloaded_file));
    response_metadata_.reset();
    loader_->ResumeReadingBodyFromNet();
    client_binding_.ResumeIncomingMethodCallProcessing();
    return Response::OK();
  }

  DCHECK_EQ(State::kNotStarted, state_);
  ApplyModificationsToRequest(std::move(modifications));
  StartRequest();
  return Response::OK();
}

void InterceptionJob::ApplyModificationsToRequest(
    std::unique_ptr<Modifications> modifications) {
  network::ResourceRequest* request = &create_loader_params_->request;

  // Note this redirect is not visible to the page by design. If they want a
  // visible redirect they can mock a response with a 302.
  if (modifications->modified_url.isJust())
    request->url = GURL(modifications->modified_url.fromJust());

  if (modifications->modified_method.isJust())
    request->method = modifications->modified_method.fromJust();

  if (modifications->modified_post_data.isJust()) {
    const std::string& post_data = modifications->modified_post_data.fromJust();
    request->request_body = network::ResourceRequestBody::CreateFromBytes(
        post_data.data(), post_data.size());
  }

  if (modifications->modified_headers.isJust()) {
    request->headers.Clear();
    std::unique_ptr<protocol::DictionaryValue> headers =
        modifications->modified_headers.fromJust()->toValue();
    for (size_t i = 0; i < headers->size(); i++) {
      protocol::DictionaryValue::Entry entry = headers->at(i);
      std::string value;
      if (!entry.second->asString(&value))
        continue;
      if (base::EqualsCaseInsensitiveASCII(entry.first,
                                           net::HttpRequestHeaders::kReferer)) {
        request->referrer = GURL(value);
        request->referrer_policy = net::URLRequest::NEVER_CLEAR_REFERRER;
      } else {
        request->headers.SetHeader(entry.first, value);
      }
    }
  }
}

Response InterceptionJob::ProcessAuthResponse(
    AuthChallengeResponse* auth_challenge_response) {
  std::string response = auth_challenge_response->GetResponse();
  state_ = State::kRequestSent;
  if (response == AuthChallengeResponse::ResponseEnum::Default) {
    std::move(pending_auth_callback_).Run(true, base::nullopt);
  } else if (response == AuthChallengeResponse::ResponseEnum::CancelAuth) {
    std::move(pending_auth_callback_).Run(false, base::nullopt);
  } else if (response ==
             AuthChallengeResponse::ResponseEnum::ProvideCredentials) {
    net::AuthCredentials credentials(
        base::UTF8ToUTF16(auth_challenge_response->GetUsername("")),
        base::UTF8ToUTF16(auth_challenge_response->GetPassword("")));
    std::move(pending_auth_callback_).Run(false, std::move(credentials));
  } else {
    return Response::InvalidParams("Unrecognized authChallengeResponse.");
  }

  return Response::OK();
}

Response InterceptionJob::ProcessResponseOverride(const std::string& response) {
  CancelRequest();

  std::string raw_headers;
  int header_size =
      net::HttpUtil::LocateEndOfHeaders(response.c_str(), response.size());
  if (header_size == -1) {
    LOG(WARNING) << "Can't find headers in result";
    header_size = 0;
  } else {
    raw_headers =
        net::HttpUtil::AssembleRawHeaders(response.c_str(), header_size);
  }
  CHECK_LE(static_cast<size_t>(header_size), response.size());
  size_t body_size = response.size() - header_size;

  response_metadata_ = std::make_unique<ResponseMetadata>();
  network::ResourceResponseHead* head = &response_metadata_->head;

  head->request_time = start_time_;
  head->response_time = base::Time::Now();
  head->headers = new net::HttpResponseHeaders(std::move(raw_headers));
  head->headers->GetMimeTypeAndCharset(&head->mime_type, &head->charset);
  if (head->mime_type.empty()) {
    net::SniffMimeType(response.data() + header_size, body_size,
                       create_loader_params_->request.url, "",
                       net::ForceSniffFileUrlsForHtml::kDisabled,
                       &head->mime_type);
  }
  head->content_length = body_size;
  head->encoded_data_length = header_size;
  head->encoded_body_length = 0;
  head->request_start = start_ticks_;
  head->response_start = base::TimeTicks::Now();

  std::string location_url;
  if (head->headers->IsRedirect(&location_url))
    return ProcessRedirectByClient(location_url);

  response_metadata_->transfer_size = body_size;

  response_metadata_->status.completion_time = base::TimeTicks::Now();
  response_metadata_->status.encoded_data_length = response.size();
  response_metadata_->status.encoded_body_length = body_size;
  response_metadata_->status.decoded_body_length = body_size;

  SendResponse(base::StringPiece(response.data() + header_size, body_size));
  return Response::OK();
}

Response InterceptionJob::ProcessRedirectByClient(const std::string& location) {
  GURL redirect_url = create_loader_params_->request.url.Resolve(location);

  if (!redirect_url.is_valid())
    return Response::Error("Invalid redirect URL in overriden headers");

  const net::HttpResponseHeaders& headers = *response_metadata_->head.headers;
  const network::ResourceRequest& request = create_loader_params_->request;

  auto first_party_url_policy =
      request.update_first_party_url_on_redirect
          ? net::URLRequest::FirstPartyURLPolicy::
                UPDATE_FIRST_PARTY_URL_ON_REDIRECT
          : net::URLRequest::FirstPartyURLPolicy::NEVER_CHANGE_FIRST_PARTY_URL;

  response_metadata_->redirect_info = std::make_unique<net::RedirectInfo>(
      net::RedirectInfo::ComputeRedirectInfo(
          request.method, request.url, request.site_for_cookies,
          first_party_url_policy, request.referrer_policy,
          request.referrer.spec(), &headers, headers.response_code(),
          redirect_url, false /* token_binding_negotiated */,
          false /* copy_fragment */));

  client_->OnReceiveRedirect(*response_metadata_->redirect_info,
                             response_metadata_->head);
  return Response::OK();
}

void InterceptionJob::SendResponse(const base::StringPiece& body) {
  client_->OnReceiveResponse(response_metadata_->head,
                             std::move(response_metadata_->downloaded_file));

  // We shouldn't be able to transfer a string that big over the protocol,
  // but just in case...
  DCHECK_LE(body.size(), UINT32_MAX)
      << "Response bodies larger than " << UINT32_MAX << " are not supported";
  mojo::DataPipe pipe(body.size());
  uint32_t num_bytes = body.size();
  MojoResult res = pipe.producer_handle->WriteData(body.data(), &num_bytes,
                                                   MOJO_WRITE_DATA_FLAG_NONE);
  DCHECK_EQ(0u, res);
  DCHECK_EQ(num_bytes, body.size());

  if (!response_metadata_->cached_metadata.empty())
    client_->OnReceiveCachedMetadata(response_metadata_->cached_metadata);
  client_->OnStartLoadingResponseBody(std::move(pipe.consumer_handle));
  if (response_metadata_->transfer_size)
    client_->OnTransferSizeUpdated(response_metadata_->transfer_size);

  client_->OnComplete(response_metadata_->status);
  Shutdown();
}

void InterceptionJob::ResponseBodyComplete() {
  if (waiting_for_resolution_)
    return;
  // We're here only if client has already told us to proceed with unmodified
  // response.
  SendResponse(body_reader_->body());
}

void InterceptionJob::StartRequest() {
  DCHECK_EQ(State::kNotStarted, state_);
  DCHECK(!response_metadata_);

  state_ = State::kRequestSent;

  network::mojom::URLLoaderClientPtr loader_client;
  client_binding_.Bind(MakeRequest(&loader_client));
  client_binding_.set_connection_error_handler(
      base::BindOnce(&InterceptionJob::Shutdown, base::Unretained(this)));

  target_factory_->CreateLoaderAndStart(
      MakeRequest(&loader_), create_loader_params_->routing_id,
      create_loader_params_->request_id, create_loader_params_->options,
      create_loader_params_->request, std::move(loader_client),
      create_loader_params_->traffic_annotation);

  if (priority_)
    loader_->SetPriority(priority_->first, priority_->second);
}

void InterceptionJob::CancelRequest() {
  if (state_ == State::kNotStarted)
    return;
  client_binding_.Close();
  loader_.reset();
  if (body_reader_) {
    body_reader_->CancelWithError(
        "Another command has cancelled the fetch request");
    body_reader_.reset();
  }
  state_ = State::kNotStarted;
}

std::unique_ptr<InterceptedRequestInfo> InterceptionJob::BuildRequestInfo(
    const network::ResourceResponseHead* head) {
  auto result = std::make_unique<InterceptedRequestInfo>();
  result->interception_id = id_;
  result->network_request =
      protocol::NetworkHandler::CreateRequestFromResourceRequest(
          create_loader_params_->request);
  result->frame_id = frame_token_;
  ResourceType resource_type =
      static_cast<ResourceType>(create_loader_params_->request.resource_type);
  result->resource_type = resource_type;
  result->is_navigation = resource_type == RESOURCE_TYPE_MAIN_FRAME ||
                          resource_type == RESOURCE_TYPE_SUB_FRAME;

  // TODO(caseq): merge with NetworkHandler::BuildResponse()
  if (head && head->headers) {
    result->http_response_status_code = head->headers->response_code();
    auto headers_dict = protocol::DictionaryValue::create();
    size_t iter = 0;
    std::string name;
    std::string value;
    while (head->headers->EnumerateHeaderLines(&iter, &name, &value)) {
      std::string old_value;
      bool merge_with_another = headers_dict->getString(name, &old_value);
      headers_dict->setString(
          name, merge_with_another ? old_value + '\n' + value : value);
    }
    result->response_headers =
        protocol::Object::fromValue(headers_dict.get(), nullptr);
  }
  return result;
}

void InterceptionJob::NotifyClient(
    std::unique_ptr<InterceptedRequestInfo> request_info) {
  waiting_for_resolution_ = true;
  BrowserThread::PostTask(
      BrowserThread::UI, FROM_HERE,
      base::BindOnce(interceptor_->request_intercepted_callback_,
                     std::move(request_info)));
}

void InterceptionJob::Shutdown() {
  if (interceptor_)
    interceptor_->RemoveJob(id_);
  delete this;
}

// URLLoader methods
void InterceptionJob::FollowRedirect(
    const base::Optional<net::HttpRequestHeaders>& modified_request_headers) {
  DCHECK(!modified_request_headers.has_value()) << "Redirect with modified "
                                                   "headers was not supported "
                                                   "yet. crbug.com/845683";
  DCHECK(!waiting_for_resolution_);

  network::ResourceRequest* request = &create_loader_params_->request;
  const net::RedirectInfo& info = *response_metadata_->redirect_info;
  request->method = info.new_method;
  request->url = info.new_url;
  request->site_for_cookies = info.new_site_for_cookies;
  request->referrer_policy = info.new_referrer_policy;
  request->referrer = GURL(info.new_referrer);
  response_metadata_.reset();
  if (interceptor_) {
    stage_ = interceptor_->GetInterceptionStage(
        request->url, static_cast<ResourceType>(request->resource_type));
  }
  if (state_ == State::kRedirectReceived) {
    state_ = State::kRequestSent;
    loader_->FollowRedirect(base::nullopt);
    return;
  }

  DCHECK_EQ(State::kNotStarted, state_);
  StartRequest();
}

void InterceptionJob::ProceedWithResponse() {
  NOTREACHED();
}

void InterceptionJob::SetPriority(net::RequestPriority priority,
                                  int32_t intra_priority_value) {
  priority_ = std::make_pair(priority, intra_priority_value);

  if (loader_)
    loader_->SetPriority(priority, intra_priority_value);
}

void InterceptionJob::PauseReadingBodyFromNet() {
  if (!body_reader_ && loader_ && state_ != State::kResponseTaken)
    loader_->PauseReadingBodyFromNet();
}

void InterceptionJob::ResumeReadingBodyFromNet() {
  if (!body_reader_ && loader_ && state_ != State::kResponseTaken)
    loader_->ResumeReadingBodyFromNet();
}

// URLLoaderClient methods
void InterceptionJob::OnReceiveResponse(
    const network::ResourceResponseHead& head,
    network::mojom::DownloadedTempFilePtr downloaded_file) {
  state_ = State::kResponseReceived;
  DCHECK(!response_metadata_);
  if (!(stage_ & InterceptionStage::RESPONSE)) {
    client_->OnReceiveResponse(head, std::move(downloaded_file));
    return;
  }
  loader_->PauseReadingBodyFromNet();
  client_binding_.PauseIncomingMethodCallProcessing();

  response_metadata_ = std::make_unique<ResponseMetadata>(head);
  response_metadata_->downloaded_file = std::move(downloaded_file);

  auto request_info = BuildRequestInfo(&head);
  const network::ResourceRequest& request = create_loader_params_->request;
  request_info->is_download =
      request_info->is_navigation && request.allow_download &&
      (is_download_ || navigation_loader_util::IsDownload(
                           request.url, head.headers.get(), head.mime_type));
  NotifyClient(std::move(request_info));
}

void InterceptionJob::OnReceiveRedirect(
    const net::RedirectInfo& redirect_info,
    const network::ResourceResponseHead& head) {
  DCHECK_EQ(State::kRequestSent, state_);
  state_ = State::kRedirectReceived;
  response_metadata_ = std::make_unique<ResponseMetadata>(head);
  response_metadata_->redirect_info =
      std::make_unique<net::RedirectInfo>(redirect_info);

  if (!(stage_ & InterceptionStage::REQUEST)) {
    client_->OnReceiveRedirect(redirect_info, head);
    return;
  }

  std::unique_ptr<InterceptedRequestInfo> request_info =
      BuildRequestInfo(&head);
  request_info->http_response_status_code = redirect_info.status_code;
  request_info->redirect_url = redirect_info.new_url.spec();
  NotifyClient(std::move(request_info));
}

void InterceptionJob::OnDataDownloaded(int64_t data_length,
                                       int64_t encoded_length) {
  if (ShouldBypassForResponse())
    client_->OnDataDownloaded(data_length, encoded_length);
}

void InterceptionJob::OnUploadProgress(int64_t current_position,
                                       int64_t total_size,
                                       OnUploadProgressCallback callback) {
  if (!report_upload_)
    return;
  client_->OnUploadProgress(current_position, total_size, std::move(callback));
}

void InterceptionJob::OnReceiveCachedMetadata(
    const std::vector<uint8_t>& data) {
  if (ShouldBypassForResponse())
    client_->OnReceiveCachedMetadata(data);
  else
    response_metadata_->cached_metadata = data;
}

void InterceptionJob::OnTransferSizeUpdated(int32_t transfer_size_diff) {
  if (ShouldBypassForResponse())
    client_->OnTransferSizeUpdated(transfer_size_diff);
  else
    response_metadata_->transfer_size += transfer_size_diff;
}

void InterceptionJob::OnStartLoadingResponseBody(
    mojo::ScopedDataPipeConsumerHandle body) {
  if (pending_response_body_pipe_callback_) {
    DCHECK_EQ(State::kResponseTaken, state_);
    DCHECK(!body_reader_);
    BrowserThread::PostTask(
        BrowserThread::UI, FROM_HERE,
        base::BindOnce(std::move(pending_response_body_pipe_callback_),
                       Response::OK(), std::move(body),
                       response_metadata_->head.mime_type));
    return;
  }
  DCHECK_EQ(State::kResponseReceived, state_);
  if (ShouldBypassForResponse())
    client_->OnStartLoadingResponseBody(std::move(body));
  else
    body_reader_->StartReading(std::move(body));
}

void InterceptionJob::OnComplete(
    const network::URLLoaderCompletionStatus& status) {
  // Essentially ShouldBypassForResponse(), but skip DCHECKs
  // since this may be called in any state during shutdown.
  if (!response_metadata_) {
    client_->OnComplete(status);
    Shutdown();
    return;
  }
  response_metadata_->status = status;
  // No need to listen to the channel any more, so just close it, so if the pipe
  // is closed by the other end, |shutdown| isn't run.
  client_binding_.Close();
  loader_.reset();
}

void InterceptionJob::OnAuthRequest(
    const scoped_refptr<net::AuthChallengeInfo>& auth_info,
    DevToolsURLLoaderInterceptor::HandleAuthRequestCallback callback) {
  DCHECK_EQ(kRequestSent, state_);
  DCHECK(pending_auth_callback_.is_null());
  DCHECK(!waiting_for_resolution_);

  if (!(stage_ & InterceptionStage::REQUEST)) {
    std::move(callback).Run(true, base::nullopt);
    return;
  }
  state_ = State::kAuthRequired;
  auto request_info = BuildRequestInfo(nullptr);
  request_info->auth_challenge =
      protocol::Network::AuthChallenge::Create()
          .SetSource(auth_info->is_proxy
                         ? protocol::Network::AuthChallenge::SourceEnum::Proxy
                         : protocol::Network::AuthChallenge::SourceEnum::Server)
          .SetOrigin(auth_info->challenger.Serialize())
          .SetScheme(auth_info->scheme)
          .SetRealm(auth_info->realm)
          .Build();
  pending_auth_callback_ = std::move(callback);
  NotifyClient(std::move(request_info));
}

}  // namespace content
