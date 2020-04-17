// Copyright 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "osp/public/presentation/presentation_connection.h"

#include <algorithm>
#include <memory>

#include "absl/strings/string_view.h"
#include "osp/impl/presentation/presentation_common.h"
#include "osp/msgs/osp_messages.h"
#include "osp/public/network_service_manager.h"
#include "osp/public/presentation/presentation_controller.h"
#include "osp/public/presentation/presentation_receiver.h"
#include "osp/public/protocol_connection.h"
#include "osp_base/std_util.h"
#include "platform/api/logging.h"

// TODO(issue/27): Address TODOs in this file
namespace openscreen {
namespace presentation {

namespace {

// TODO(jophba): replace Write methods with a unified write message surface
Error WriteConnectionMessage(const msgs::PresentationConnectionMessage& message,
                             ProtocolConnection* connection) {
  return connection->WriteMessage(message,
                                  msgs::EncodePresentationConnectionMessage);
}
}  // namespace

Connection::Connection(const PresentationInfo& info,
                       Delegate* delegate,
                       ParentDelegate* parent_delegate)
    : presentation_(info),
      state_(State::kConnecting),
      delegate_(delegate),
      parent_delegate_(parent_delegate),
      connection_id_(0),
      protocol_connection_(nullptr) {}

Connection::~Connection() {
  if (state_ == State::kConnected) {
    Close(CloseReason::kDiscarded);
    delegate_->OnDiscarded();
  }
  parent_delegate_->OnConnectionDestroyed(this);
}

void Connection::OnConnected(
    uint64_t connection_id,
    uint64_t endpoint_id,
    std::unique_ptr<ProtocolConnection> protocol_connection) {
  if (state_ != State::kConnecting) {
    return;
  }
  connection_id_ = connection_id;
  endpoint_id_ = endpoint_id;
  protocol_connection_ = std::move(protocol_connection);
  state_ = State::kConnected;
  delegate_->OnConnected();
}

bool Connection::OnClosed() {
  if (state_ != State::kConnecting && state_ != State::kConnected)
    return false;

  protocol_connection_.reset();
  state_ = State::kClosed;

  return true;
}

void Connection::OnClosedByError(Error cause) {
  if (OnClosed())
    delegate_->OnError(std::string(cause));
}

void Connection::OnClosedByRemote() {
  if (OnClosed())
    delegate_->OnClosedByRemote();
}

void Connection::OnTerminated() {
  if (state_ == State::kTerminated)
    return;
  protocol_connection_.reset();
  state_ = State::kTerminated;
  delegate_->OnTerminated();
}

Error Connection::SendString(absl::string_view message) {
  if (state_ != State::kConnected)
    return Error::Code::kNoActiveConnection;

  msgs::PresentationConnectionMessage cbor_message;
  OSP_LOG << "sending '" << message << "' to (" << presentation_.id << ", "
          << connection_id_.value() << ")";
  cbor_message.connection_id = connection_id_.value();
  cbor_message.message.which =
      msgs::PresentationConnectionMessage::Message::Which::kString;

  new (&cbor_message.message.str) std::string(message);

  return WriteConnectionMessage(cbor_message, protocol_connection_.get());
}

Error Connection::SendBinary(std::vector<uint8_t>&& data) {
  if (state_ != State::kConnected)
    return Error::Code::kNoActiveConnection;

  msgs::PresentationConnectionMessage cbor_message;
  OSP_LOG << "sending " << data.size() << " bytes to (" << presentation_.id
          << ", " << connection_id_.value() << ")";
  cbor_message.connection_id = connection_id_.value();
  cbor_message.message.which =
      msgs::PresentationConnectionMessage::Message::Which::kBytes;

  new (&cbor_message.message.bytes) std::vector<uint8_t>(std::move(data));

  return WriteConnectionMessage(cbor_message, protocol_connection_.get());
  return Error::None();
}

Error Connection::Close(CloseReason reason) {
  if (state_ == State::kClosed || state_ == State::kTerminated)
    return Error::Code::kAlreadyClosed;

  state_ = State::kClosed;
  protocol_connection_.reset();

  return parent_delegate_->CloseConnection(this, reason);
}

void Connection::Terminate(TerminationReason reason) {
  if (state_ == State::kTerminated)
    return;
  state_ = State::kTerminated;
  protocol_connection_.reset();
  parent_delegate_->OnPresentationTerminated(presentation_.id, reason);
}

ConnectionManager::ConnectionManager(MessageDemuxer* demuxer) {
  message_watch_ = demuxer->SetDefaultMessageTypeWatch(
      msgs::Type::kPresentationConnectionMessage, this);

  close_request_watch_ = demuxer->SetDefaultMessageTypeWatch(
      msgs::Type::kPresentationConnectionCloseRequest, this);

  close_event_watch_ = demuxer->SetDefaultMessageTypeWatch(
      msgs::Type::kPresentationConnectionCloseEvent, this);
}

void ConnectionManager::AddConnection(Connection* connection) {
  auto emplace_result =
      connections_.emplace(connection->connection_id(), connection);

  OSP_DCHECK(emplace_result.second);
}

void ConnectionManager::RemoveConnection(Connection* connection) {
  auto entry = connections_.find(connection->connection_id());
  if (entry != connections_.end()) {
    connections_.erase(entry);
  }
}

// TODO(jophba): add a utility object to track requests/responses
// TODO(jophba): refine the RegisterWatch/OnStreamMessage API. We
// should add a layer between the message logic and the parse/dispatch
// logic, and remove the CBOR information from ConnectionManager.
ErrorOr<size_t> ConnectionManager::OnStreamMessage(
    uint64_t endpoint_id,
    uint64_t connection_id,
    msgs::Type message_type,
    const uint8_t* buffer,
    size_t buffer_size,
    platform::Clock::time_point now) {
  switch (message_type) {
    case msgs::Type::kPresentationConnectionMessage: {
      msgs::PresentationConnectionMessage message;
      ssize_t result = msgs::DecodePresentationConnectionMessage(
          buffer, buffer_size, &message);
      if (result < 0) {
        OSP_LOG_WARN << "presentation-connection-message parse error";
        return Error::Code::kParseError;
      }

      Connection* connection = GetConnection(message.connection_id);
      if (!connection) {
        return Error::Code::kNoItemFound;
      }

      switch (message.message.which) {
        case decltype(message.message.which)::kString:
          connection->get_delegate()->OnStringMessage(message.message.str);
          break;
        case decltype(message.message.which)::kBytes:
          connection->get_delegate()->OnBinaryMessage(message.message.bytes);
          break;
        default:
          OSP_LOG_WARN << "uninitialized message data in "
                          "presentation-connection-message";
          break;
      }
      return result;
    }

    case msgs::Type::kPresentationConnectionCloseRequest: {
      msgs::PresentationConnectionCloseRequest request;
      ssize_t result = msgs::DecodePresentationConnectionCloseRequest(
          buffer, buffer_size, &request);
      if (result < 0) {
        OSP_LOG_WARN << "decode presentation-connection-close-event error: "
                     << result;
        return Error::Code::kCborInvalidMessage;
      }

      Connection* connection = GetConnection(request.connection_id);
      if (!connection) {
        return Error::Code::kNoActiveConnection;
      }

      // TODO(btolsch): Do we want closed/discarded/error here?
      connection->OnClosedByRemote();
      return result;
    }

    case msgs::Type::kPresentationConnectionCloseEvent: {
      msgs::PresentationConnectionCloseEvent event;
      ssize_t result = msgs::DecodePresentationConnectionCloseEvent(
          buffer, buffer_size, &event);
      if (result < 0) {
        OSP_LOG_WARN << "decode presentation-connection-close-event error: "
                     << result;
        return Error::Code::kParseError;
      }

      Connection* connection = GetConnection(event.connection_id);
      if (!connection) {
        return Error::Code::kNoActiveConnection;
      }

      connection->OnClosedByRemote();
      return result;
    }

    // TODO(jophba): The spec says to close the connection if we get a message
    // we don't understand. Figure out how to honor the spec here.
    default:
      return Error::Code::kUnknownMessageType;
  }
}

Connection* ConnectionManager::GetConnection(uint64_t connection_id) {
  auto entry = connections_.find(connection_id);
  if (entry != connections_.end()) {
    return entry->second;
  }

  OSP_DVLOG << "unknown ID: " << connection_id;
  return nullptr;
}

}  // namespace presentation
}  // namespace openscreen
