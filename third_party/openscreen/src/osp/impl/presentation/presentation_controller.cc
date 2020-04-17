// Copyright 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "osp/public/presentation/presentation_controller.h"

#include <algorithm>
#include <sstream>
#include <type_traits>

#include "absl/types/optional.h"
#include "osp/impl/presentation/url_availability_requester.h"
#include "osp/msgs/osp_messages.h"
#include "osp/msgs/request_response_handler.h"
#include "osp/public/message_demuxer.h"
#include "osp/public/network_service_manager.h"
#include "osp/public/protocol_connection_client.h"
#include "platform/api/logging.h"

namespace openscreen {
namespace presentation {

#define DECLARE_MSG_REQUEST_RESPONSE(base_name)                        \
  using RequestMsgType = msgs::Presentation##base_name##Request;       \
  using ResponseMsgType = msgs::Presentation##base_name##Response;     \
                                                                       \
  static constexpr MessageEncodingFunction<RequestMsgType> kEncoder =  \
      &msgs::EncodePresentation##base_name##Request;                   \
  static constexpr MessageDecodingFunction<ResponseMsgType> kDecoder = \
      &msgs::DecodePresentation##base_name##Response;                  \
  static constexpr msgs::Type kResponseType =                          \
      msgs::Type::kPresentation##base_name##Response

struct StartRequest {
  DECLARE_MSG_REQUEST_RESPONSE(Start);

  msgs::PresentationStartRequest request;
  RequestDelegate* delegate;
  Connection::Delegate* presentation_connection_delegate;
};

struct TerminationRequest {
  DECLARE_MSG_REQUEST_RESPONSE(Termination);

  msgs::PresentationTerminationRequest request;
};

class Controller::MessageGroupStreams final
    : public ProtocolConnectionClient::ConnectionRequestCallback,
      public ProtocolConnection::Observer,
      public RequestResponseHandler<StartRequest>::Delegate,
      public RequestResponseHandler<TerminationRequest>::Delegate {
 public:
  MessageGroupStreams(Controller* controller, const std::string& service_id);
  ~MessageGroupStreams();

  uint64_t SendStartRequest(StartRequest request);
  void CancelStartRequest(uint64_t request_id);
  void OnMatchedResponse(StartRequest* request,
                         msgs::PresentationStartResponse* response,
                         uint64_t endpoint_id) override;
  void OnError(StartRequest* request, Error error) override;

  void SendTerminationRequest(TerminationRequest request);
  void OnMatchedResponse(TerminationRequest* request,
                         msgs::PresentationTerminationResponse* response,
                         uint64_t endpoint_id) override;
  void OnError(TerminationRequest* request, Error error) override;

  // ProtocolConnectionClient::ConnectionRequestCallback overrides.
  void OnConnectionOpened(
      uint64_t request_id,
      std::unique_ptr<ProtocolConnection> connection) override;
  void OnConnectionFailed(uint64_t request_id) override;

  // ProtocolConnection::Observer overrides.
  void OnConnectionClosed(const ProtocolConnection& connection) override;

 private:
  uint64_t GetNextInternalRequestId();

  Controller* const controller_;
  const std::string service_id_;

  uint64_t next_internal_request_id_ = 1;
  ProtocolConnectionClient::ConnectRequest initiation_connect_request_;
  std::unique_ptr<ProtocolConnection> initiation_protocol_connection_;

  // TODO(btolsch): Improve the ergo of QuicClient::Connect because this is bad.
  bool initiation_connect_request_stack_{false};

  RequestResponseHandler<StartRequest> initiation_handler_;
  RequestResponseHandler<TerminationRequest> termination_handler_;
};

Controller::MessageGroupStreams::MessageGroupStreams(
    Controller* controller,
    const std::string& service_id)
    : controller_(controller),
      service_id_(service_id),
      initiation_handler_(this),
      termination_handler_(this) {}

Controller::MessageGroupStreams::~MessageGroupStreams() = default;

uint64_t Controller::MessageGroupStreams::SendStartRequest(
    StartRequest request) {
  uint64_t request_id = GetNextInternalRequestId();
  if (!initiation_protocol_connection_ && !initiation_connect_request_) {
    initiation_connect_request_stack_ = true;
    initiation_connect_request_ =
        NetworkServiceManager::Get()->GetProtocolConnectionClient()->Connect(
            controller_->receiver_endpoints_[service_id_], this);
    initiation_connect_request_stack_ = false;
  }
  initiation_handler_.WriteMessage(request_id, &request);
  return request_id;
}

void Controller::MessageGroupStreams::CancelStartRequest(uint64_t request_id) {
  // TODO(btolsch): Instead, mark the |request_id| for immediate termination if
  // we get a successful response.
  initiation_handler_.CancelMessage(request_id);
}

void Controller::MessageGroupStreams::OnMatchedResponse(
    StartRequest* request,
    msgs::PresentationStartResponse* response,
    uint64_t endpoint_id) {
  if (response->result != msgs::PresentationStartResponse_result::kSuccess) {
    std::stringstream ss;
    ss << "presentation-start-response for " << request->request.url
       << " failed: " << static_cast<int>(response->result);
    Error error(Error::Code::kUnknownStartError, ss.str());
    OSP_LOG_INFO << error.message();
    request->delegate->OnError(std::move(error));
    return;
  }
  OSP_LOG_INFO << "presentation started for " << request->request.url;
  Controller::ControlledPresentation& presentation =
      controller_->presentations_[request->request.presentation_id];
  presentation.service_id = service_id_;
  presentation.url = request->request.url;
  auto connection = std::make_unique<Connection>(
      Connection::PresentationInfo{request->request.presentation_id,
                                   request->request.url},
      request->presentation_connection_delegate, controller_);
  controller_->OpenConnection(response->connection_id, endpoint_id, service_id_,
                              request->delegate, std::move(connection),
                              NetworkServiceManager::Get()
                                  ->GetProtocolConnectionClient()
                                  ->CreateProtocolConnection(endpoint_id));
}

void Controller::MessageGroupStreams::OnError(StartRequest* request,
                                              Error error) {
  request->delegate->OnError(error);
}

void Controller::MessageGroupStreams::SendTerminationRequest(
    TerminationRequest request) {
  if (!initiation_protocol_connection_ && !initiation_connect_request_) {
    initiation_connect_request_ =
        NetworkServiceManager::Get()->GetProtocolConnectionClient()->Connect(
            controller_->receiver_endpoints_[service_id_], this);
  }
  termination_handler_.WriteMessage(absl::nullopt, &request);
}

void Controller::MessageGroupStreams::OnMatchedResponse(
    TerminationRequest* request,
    msgs::PresentationTerminationResponse* response,
    uint64_t endpoint_id) {
  OSP_VLOG << "got presentation-termination-response for "
           << request->request.presentation_id << " with result "
           << static_cast<int>(response->result);
  controller_->TerminatePresentationById(request->request.presentation_id);
}

void Controller::MessageGroupStreams::OnError(TerminationRequest* request,
                                              Error error) {}

void Controller::MessageGroupStreams::OnConnectionOpened(
    uint64_t request_id,
    std::unique_ptr<ProtocolConnection> connection) {
  if ((initiation_connect_request_ &&
       initiation_connect_request_.request_id() == request_id) ||
      initiation_connect_request_stack_) {
    initiation_protocol_connection_ = std::move(connection);
    initiation_protocol_connection_->SetObserver(this);
    initiation_connect_request_.MarkComplete();
    initiation_handler_.SetConnection(initiation_protocol_connection_.get());
    termination_handler_.SetConnection(initiation_protocol_connection_.get());
  }
}

void Controller::MessageGroupStreams::OnConnectionFailed(uint64_t request_id) {
  if (initiation_connect_request_ &&
      initiation_connect_request_.request_id() == request_id) {
    initiation_connect_request_.MarkComplete();
    initiation_handler_.Reset();
    termination_handler_.Reset();
  }
}

void Controller::MessageGroupStreams::OnConnectionClosed(
    const ProtocolConnection& connection) {
  if (&connection == initiation_protocol_connection_.get()) {
    initiation_handler_.Reset();
    termination_handler_.Reset();
  }
}

uint64_t Controller::MessageGroupStreams::GetNextInternalRequestId() {
  return ++next_internal_request_id_;
}

Controller::ReceiverWatch::ReceiverWatch() = default;
Controller::ReceiverWatch::ReceiverWatch(Controller* controller,
                                         const std::vector<std::string>& urls,
                                         ReceiverObserver* observer)
    : urls_(urls), observer_(observer), controller_(controller) {}

Controller::ReceiverWatch::ReceiverWatch(Controller::ReceiverWatch&& other) {
  swap(*this, other);
}

Controller::ReceiverWatch::~ReceiverWatch() {
  if (observer_) {
    controller_->CancelReceiverWatch(urls_, observer_);
  }
  observer_ = nullptr;
}

Controller::ReceiverWatch& Controller::ReceiverWatch::operator=(
    Controller::ReceiverWatch other) {
  swap(*this, other);
  return *this;
}

void swap(Controller::ReceiverWatch& a, Controller::ReceiverWatch& b) {
  using std::swap;
  swap(a.urls_, b.urls_);
  swap(a.observer_, b.observer_);
  swap(a.controller_, b.controller_);
}

Controller::ConnectRequest::ConnectRequest() = default;
Controller::ConnectRequest::ConnectRequest(Controller* controller,
                                           const std::string& service_id,
                                           bool is_reconnect,
                                           absl::optional<uint64_t> request_id)
    : service_id_(service_id),
      is_reconnect_(is_reconnect),
      request_id_(request_id),
      controller_(controller) {}

Controller::ConnectRequest::ConnectRequest(ConnectRequest&& other) {
  swap(*this, other);
}

Controller::ConnectRequest::~ConnectRequest() {
  if (request_id_) {
    controller_->CancelConnectRequest(service_id_, is_reconnect_,
                                      request_id_.value());
  }
  request_id_ = 0;
}

Controller::ConnectRequest& Controller::ConnectRequest::operator=(
    ConnectRequest other) {
  swap(*this, other);
  return *this;
}

void swap(Controller::ConnectRequest& a, Controller::ConnectRequest& b) {
  using std::swap;
  swap(a.service_id_, b.service_id_);
  swap(a.is_reconnect_, b.is_reconnect_);
  swap(a.request_id_, b.request_id_);
  swap(a.controller_, b.controller_);
}

Controller::Controller(platform::ClockNowFunctionPtr now_function) {
  availability_requester_ =
      std::make_unique<UrlAvailabilityRequester>(now_function);
  connection_manager_ =
      std::make_unique<ConnectionManager>(NetworkServiceManager::Get()
                                              ->GetProtocolConnectionClient()
                                              ->message_demuxer());
  const std::vector<ServiceInfo>& receivers =
      NetworkServiceManager::Get()->GetMdnsServiceListener()->GetReceivers();
  for (const auto& info : receivers) {
    // TODO(issue/33): Replace service_id with endpoint_id when endpoint_id is
    // more than just an IPEndpoint counter and actually relates to a device's
    // identity.
    receiver_endpoints_.emplace(info.service_id, info.v4_endpoint.port
                                                     ? info.v4_endpoint
                                                     : info.v6_endpoint);
    availability_requester_->AddReceiver(info);
  }
  // TODO(btolsch): This is for |receiver_endpoints_|, but this should really be
  // tracked elsewhere so it's available to other protocols as well.
  NetworkServiceManager::Get()->GetMdnsServiceListener()->AddObserver(this);
}

Controller::~Controller() {
  connection_manager_.reset();
  NetworkServiceManager::Get()->GetMdnsServiceListener()->RemoveObserver(this);
}

Controller::ReceiverWatch Controller::RegisterReceiverWatch(
    const std::vector<std::string>& urls,
    ReceiverObserver* observer) {
  availability_requester_->AddObserver(urls, observer);
  return ReceiverWatch(this, urls, observer);
}

Controller::ConnectRequest Controller::StartPresentation(
    const std::string& url,
    const std::string& service_id,
    RequestDelegate* delegate,
    Connection::Delegate* conn_delegate) {
  StartRequest request;
  request.request.url = url;
  request.request.presentation_id = MakePresentationId(url, service_id);
  request.delegate = delegate;
  request.presentation_connection_delegate = conn_delegate;
  uint64_t request_id =
      group_streams_[service_id]->SendStartRequest(std::move(request));
  const bool is_reconnect = false;
  return ConnectRequest(this, service_id, is_reconnect, request_id);
}

Controller::ConnectRequest Controller::ReconnectPresentation(
    const std::vector<std::string>& urls,
    const std::string& presentation_id,
    const std::string& service_id,
    RequestDelegate* delegate,
    Connection::Delegate* conn_delegate) {
  OSP_UNIMPLEMENTED();
  return ConnectRequest();
}

Controller::ConnectRequest Controller::ReconnectConnection(
    std::unique_ptr<Connection> connection,
    RequestDelegate* delegate) {
  OSP_UNIMPLEMENTED();
  return ConnectRequest();
}

Error Controller::CloseConnection(Connection* connection,
                                  Connection::CloseReason reason) {
  OSP_UNIMPLEMENTED();
  return Error::None();
}

Error Controller::OnPresentationTerminated(const std::string& presentation_id,
                                           TerminationReason reason) {
  auto presentation_entry = presentations_.find(presentation_id);
  if (presentation_entry == presentations_.end()) {
    return Error::Code::kNoPresentationFound;
  }
  ControlledPresentation& presentation = presentation_entry->second;
  for (auto* connection : presentation.connections) {
    connection->OnTerminated();
  }
  TerminationRequest request;
  request.request.presentation_id = presentation_id;
  request.request.reason =
      msgs::PresentationTerminationRequest_reason::kUserTerminatedViaController;
  group_streams_[presentation.service_id]->SendTerminationRequest(
      std::move(request));
  presentations_.erase(presentation_entry);
  termination_listener_by_id_.erase(presentation_id);
  return Error::None();
}

void Controller::OnConnectionDestroyed(Connection* connection) {
  auto presentation_entry =
      presentations_.find(connection->presentation_info().id);
  if (presentation_entry == presentations_.end()) {
    return;
  }

  std::vector<Connection*>& connections =
      presentation_entry->second.connections;

  connections.erase(
      std::remove(connections.begin(), connections.end(), connection),
      connections.end());

  connection_manager_->RemoveConnection(connection);
}

std::string Controller::GetServiceIdForPresentationId(
    const std::string& presentation_id) const {
  auto presentation_entry = presentations_.find(presentation_id);
  if (presentation_entry == presentations_.end()) {
    return "";
  }
  return presentation_entry->second.service_id;
}

ProtocolConnection* Controller::GetConnectionRequestGroupStream(
    const std::string& service_id) {
  OSP_UNIMPLEMENTED();
  return nullptr;
}

void Controller::OnError(ServiceListenerError) {}
void Controller::OnMetrics(ServiceListener::Metrics) {}

class Controller::TerminationListener final
    : public MessageDemuxer::MessageCallback {
 public:
  TerminationListener(Controller* controller,
                      const std::string& presentation_id,
                      uint64_t endpoint_id);
  ~TerminationListener() override;

  // MessageDemuxer::MessageCallback overrides.
  ErrorOr<size_t> OnStreamMessage(uint64_t endpoint_id,
                                  uint64_t connection_id,
                                  msgs::Type message_type,
                                  const uint8_t* buffer,
                                  size_t buffer_size,
                                  platform::Clock::time_point now) override;

 private:
  Controller* const controller_;
  std::string presentation_id_;
  MessageDemuxer::MessageWatch event_watch_;
};

Controller::TerminationListener::TerminationListener(
    Controller* controller,
    const std::string& presentation_id,
    uint64_t endpoint_id)
    : controller_(controller), presentation_id_(presentation_id) {
  event_watch_ =
      NetworkServiceManager::Get()
          ->GetProtocolConnectionClient()
          ->message_demuxer()
          ->WatchMessageType(endpoint_id,
                             msgs::Type::kPresentationTerminationEvent, this);
}

Controller::TerminationListener::~TerminationListener() = default;

ErrorOr<size_t> Controller::TerminationListener::OnStreamMessage(
    uint64_t endpoint_id,
    uint64_t connection_id,
    msgs::Type message_type,
    const uint8_t* buffer,
    size_t buffer_size,
    platform::Clock::time_point now) {
  OSP_CHECK_EQ(static_cast<int>(msgs::Type::kPresentationTerminationEvent),
               static_cast<int>(message_type));
  msgs::PresentationTerminationEvent event;
  ssize_t result =
      msgs::DecodePresentationTerminationEvent(buffer, buffer_size, &event);
  if (result < 0) {
    OSP_LOG_WARN << "decode presentation-termination-event error: " << result;
    return Error::Code::kCborParsing;
  } else if (event.presentation_id != presentation_id_) {
    OSP_LOG_WARN << "got presentation-termination-event for wrong id: "
                 << presentation_id_ << " vs. " << event.presentation_id;
    return result;
  }
  OSP_LOG_INFO << "termination event";
  auto presentation_entry =
      controller_->presentations_.find(event.presentation_id);
  if (presentation_entry != controller_->presentations_.end()) {
    for (auto* connection : presentation_entry->second.connections)
      connection->OnTerminated();
    controller_->presentations_.erase(presentation_entry);
  }
  controller_->termination_listener_by_id_.erase(event.presentation_id);
  return result;
}

// static
std::string Controller::MakePresentationId(const std::string& url,
                                           const std::string& service_id) {
  OSP_UNIMPLEMENTED();
  // TODO(btolsch): This is just a placeholder for the demo.
  std::string safe_id = service_id;
  for (auto& c : safe_id)
    if (c < ' ' || c > '~')
      c = '.';
  return safe_id + ":" + url;
}

void Controller::OpenConnection(
    uint64_t connection_id,
    uint64_t endpoint_id,
    const std::string& service_id,
    RequestDelegate* request_delegate,
    std::unique_ptr<Connection>&& connection,
    std::unique_ptr<ProtocolConnection>&& protocol_connection) {
  connection->OnConnected(connection_id, endpoint_id,
                          std::move(protocol_connection));
  const std::string& presentation_id = connection->presentation_info().id;
  auto presentation_entry = presentations_.find(presentation_id);
  if (presentation_entry == presentations_.end()) {
    auto emplace_entry = presentations_.emplace(
        presentation_id,
        ControlledPresentation{
            service_id, connection->presentation_info().url, {}});
    presentation_entry = emplace_entry.first;
  }
  ControlledPresentation& presentation = presentation_entry->second;
  presentation.connections.push_back(connection.get());
  connection_manager_->AddConnection(connection.get());

  auto terminate_entry = termination_listener_by_id_.find(presentation_id);
  if (terminate_entry == termination_listener_by_id_.end()) {
    termination_listener_by_id_.emplace(
        presentation_id, std::make_unique<TerminationListener>(
                             this, presentation_id, endpoint_id));
  }
  request_delegate->OnConnection(std::move(connection));
}

void Controller::TerminatePresentationById(const std::string& presentation_id) {
  auto presentation_entry = presentations_.find(presentation_id);
  if (presentation_entry != presentations_.end()) {
    for (auto* connection : presentation_entry->second.connections) {
      connection->OnTerminated();
    }
    presentations_.erase(presentation_entry);
  }
}

void Controller::CancelReceiverWatch(const std::vector<std::string>& urls,
                                     ReceiverObserver* observer) {
  availability_requester_->RemoveObserverUrls(urls, observer);
}

void Controller::CancelConnectRequest(const std::string& service_id,
                                      bool is_reconnect,
                                      uint64_t request_id) {
  auto group_streams_entry = group_streams_.find(service_id);
  if (group_streams_entry == group_streams_.end())
    return;
  group_streams_entry->second->CancelStartRequest(request_id);
}

void Controller::OnStarted() {}
void Controller::OnStopped() {}
void Controller::OnSuspended() {}
void Controller::OnSearching() {}

void Controller::OnReceiverAdded(const ServiceInfo& info) {
  receiver_endpoints_.emplace(info.service_id, info.v4_endpoint.port
                                                   ? info.v4_endpoint
                                                   : info.v6_endpoint);
  auto group_streams =
      std::make_unique<MessageGroupStreams>(this, info.service_id);
  group_streams_[info.service_id] = std::move(group_streams);
  availability_requester_->AddReceiver(info);
}

void Controller::OnReceiverChanged(const ServiceInfo& info) {
  receiver_endpoints_[info.service_id] =
      info.v4_endpoint.port ? info.v4_endpoint : info.v6_endpoint;
  availability_requester_->ChangeReceiver(info);
}

void Controller::OnReceiverRemoved(const ServiceInfo& info) {
  receiver_endpoints_.erase(info.service_id);
  group_streams_.erase(info.service_id);
  availability_requester_->RemoveReceiver(info);
}

void Controller::OnAllReceiversRemoved() {
  receiver_endpoints_.clear();
  availability_requester_->RemoveAllReceivers();
}

}  // namespace presentation
}  // namespace openscreen
