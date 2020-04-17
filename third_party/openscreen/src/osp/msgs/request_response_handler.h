// Copyright 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef OSP_MSGS_REQUEST_RESPONSE_HANDLER_H_
#define OSP_MSGS_REQUEST_RESPONSE_HANDLER_H_

#include <cstddef>
#include <cstdint>

#include "osp/public/message_demuxer.h"
#include "osp/public/network_service_manager.h"
#include "osp/public/protocol_connection.h"
#include "osp_base/error.h"
#include "osp_base/macros.h"
#include "platform/api/logging.h"

namespace openscreen {

template <typename T>
using MessageDecodingFunction = ssize_t (*)(const uint8_t*, size_t, T*);

// Provides a uniform way of accessing import properties of a request/response
// message pair from a template: request encode function, response decode
// function, request serializable data member.
template <typename T>
struct DefaultRequestCoderTraits {
 public:
  using RequestMsgType = typename T::RequestMsgType;
  static constexpr MessageEncodingFunction<RequestMsgType> kEncoder =
      T::kEncoder;
  static constexpr MessageDecodingFunction<typename T::ResponseMsgType>
      kDecoder = T::kDecoder;

  static const RequestMsgType* serial_request(const T& data) {
    return &data.request;
  }
  static RequestMsgType* serial_request(T& data) { return &data.request; }
};

// Provides a wrapper for the common pattern of sending a request message and
// waiting for a response message with a matching |request_id| field.  It also
// handles the business of queueing messages to be sent until a protocol
// connection is available.
//
// Messages are written using WriteMessage.  This will queue messages if there
// is no protocol connection or write them immediately if there is.  When a
// matching response is received via the MessageDemuxer (taken from the global
// ProtocolConnectionClient), OnMatchedResponse is called on the provided
// Delegate object along with the original request that it matches.
template <typename RequestT,
          typename RequestCoderTraits = DefaultRequestCoderTraits<RequestT>>
class RequestResponseHandler : public MessageDemuxer::MessageCallback {
 public:
  class Delegate {
   public:
    virtual ~Delegate() = default;

    virtual void OnMatchedResponse(RequestT* request,
                                   typename RequestT::ResponseMsgType* response,
                                   uint64_t endpoint_id) = 0;
    virtual void OnError(RequestT* request, Error error) = 0;
  };

  explicit RequestResponseHandler(Delegate* delegate) : delegate_(delegate) {}
  ~RequestResponseHandler() { Reset(); }

  void Reset() {
    connection_ = nullptr;
    for (auto& message : to_send_) {
      delegate_->OnError(&message.request, Error::Code::kRequestCancelled);
    }
    to_send_.clear();
    for (auto& message : sent_) {
      delegate_->OnError(&message.request, Error::Code::kRequestCancelled);
    }
    sent_.clear();
    response_watch_ = MessageDemuxer::MessageWatch();
  }

  // Write a message to the underlying protocol connection, or queue it until
  // one is provided via SetConnection.  If |id| is provided, it can be used to
  // cancel the message via CancelMessage.
  Error WriteMessage(absl::optional<uint64_t> id, RequestT* message) {
    auto* request_msg = RequestCoderTraits::serial_request(*message);
    if (connection_) {
      request_msg->request_id = GetNextRequestId(connection_->endpoint_id());
      Error result =
          connection_->WriteMessage(*request_msg, RequestCoderTraits::kEncoder);
      if (!result.ok()) {
        return result;
      }
      sent_.emplace_back(RequestWithId{id, std::move(*message)});
      EnsureResponseWatch();
    } else {
      to_send_.emplace_back(RequestWithId{id, std::move(*message)});
    }
    return Error::None();
  }

  // Remove the message that was originally written with |id| from the send and
  // sent queues so that we are no longer looking for a response.
  void CancelMessage(uint64_t id) {
    to_send_.erase(std::remove_if(to_send_.begin(), to_send_.end(),
                                  [&id](const RequestWithId& msg) {
                                    return id == msg.id;
                                  }),
                   to_send_.end());
    sent_.erase(std::remove_if(
                    sent_.begin(), sent_.end(),
                    [&id](const RequestWithId& msg) { return id == msg.id; }),
                sent_.end());
    if (sent_.empty()) {
      response_watch_ = MessageDemuxer::MessageWatch();
    }
  }

  // Assign a ProtocolConnection to this handler for writing messages.
  void SetConnection(ProtocolConnection* connection) {
    connection_ = connection;
    for (auto& message : to_send_) {
      auto* request_msg = RequestCoderTraits::serial_request(message.request);
      request_msg->request_id = GetNextRequestId(connection_->endpoint_id());
      Error result =
          connection_->WriteMessage(*request_msg, RequestCoderTraits::kEncoder);
      if (result.ok()) {
        sent_.emplace_back(std::move(message));
      } else {
        delegate_->OnError(&message.request, result);
      }
    }
    if (!to_send_.empty()) {
      EnsureResponseWatch();
    }
    to_send_.clear();
  }

  // MessageDemuxer::MessageCallback overrides.
  ErrorOr<size_t> OnStreamMessage(uint64_t endpoint_id,
                                  uint64_t connection_id,
                                  msgs::Type message_type,
                                  const uint8_t* buffer,
                                  size_t buffer_size,
                                  platform::Clock::time_point now) override {
    if (message_type != RequestT::kResponseType) {
      return 0;
    }
    typename RequestT::ResponseMsgType response;
    ssize_t result =
        RequestCoderTraits::kDecoder(buffer, buffer_size, &response);
    if (result < 0) {
      return 0;
    }
    auto it = std::find_if(
        sent_.begin(), sent_.end(), [&response](const RequestWithId& msg) {
          return RequestCoderTraits::serial_request(msg.request)->request_id ==
                 response.request_id;
        });
    if (it != sent_.end()) {
      delegate_->OnMatchedResponse(&it->request, &response,
                                   connection_->endpoint_id());
      sent_.erase(it);
      if (sent_.empty()) {
        response_watch_ = MessageDemuxer::MessageWatch();
      }
    } else {
      OSP_LOG_WARN << "got response for unknown request id: "
                   << response.request_id;
    }
    return result;
  }

 private:
  struct RequestWithId {
    absl::optional<uint64_t> id;
    RequestT request;
  };

  void EnsureResponseWatch() {
    if (!response_watch_) {
      response_watch_ = NetworkServiceManager::Get()
                            ->GetProtocolConnectionClient()
                            ->message_demuxer()
                            ->WatchMessageType(connection_->endpoint_id(),
                                               RequestT::kResponseType, this);
    }
  }

  uint64_t GetNextRequestId(uint64_t endpoint_id) {
    return NetworkServiceManager::Get()
        ->GetProtocolConnectionClient()
        ->endpoint_request_ids()
        ->GetNextRequestId(endpoint_id);
  }

  ProtocolConnection* connection_ = nullptr;
  Delegate* const delegate_;
  std::vector<RequestWithId> to_send_;
  std::vector<RequestWithId> sent_;
  MessageDemuxer::MessageWatch response_watch_;

  OSP_DISALLOW_COPY_AND_ASSIGN(RequestResponseHandler);
};

}  // namespace openscreen

#endif  // OSP_MSGS_REQUEST_RESPONSE_HANDLER_H_
