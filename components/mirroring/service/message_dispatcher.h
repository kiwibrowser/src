// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_MIRRORING_SERVICE_MESSAGE_DISPATCHER_H_
#define COMPONENTS_MIRRORING_SERVICE_MESSAGE_DISPATCHER_H_

#include "base/callback.h"
#include "base/containers/flat_map.h"
#include "base/macros.h"
#include "components/mirroring/service/interface.h"
#include "components/mirroring/service/receiver_response.h"

namespace mirroring {

// Dispatches inbound/outbound messages. The outbound messages are sent out
// through |outbound_channel|, and the inbound messages are handled by this
// class.
class MessageDispatcher final : public CastMessageChannel {
 public:
  using ErrorCallback = base::RepeatingCallback<void(const std::string&)>;
  // TODO(xjz): Also pass a CastMessageChannel interface request for inbound
  // message channel.
  MessageDispatcher(CastMessageChannel* outbound_channel,
                    ErrorCallback error_callback);
  ~MessageDispatcher() override;

  using ResponseCallback =
      base::RepeatingCallback<void(const ReceiverResponse& response)>;
  // Registers/Unregisters callback for a certain type of responses.
  void Subscribe(ResponseType type, ResponseCallback callback);
  void Unsubscribe(ResponseType type);

  using OnceResponseCallback =
      base::OnceCallback<void(const ReceiverResponse& response)>;
  // Sends the given message and subscribes to replies until an acceptable one
  // is received or a timeout elapses. Message of the given response type is
  // delivered to the supplied callback if the sequence number of the response
  // matches |sequence_number|. If the timeout period elapses, the callback will
  // be run once with an unknown type of |response|.
  void RequestReply(const CastMessage& message,
                    ResponseType response_type,
                    int32_t sequence_number,
                    const base::TimeDelta& timeout,
                    OnceResponseCallback callback);

  // Get the sequence number for the next outbound message. Never returns 0.
  int32_t GetNextSeqNumber();

  // Requests to send outbound |message|.
  void SendOutboundMessage(const CastMessage& message);

 private:
  class RequestHolder;

  // CastMessageChannel implementation. Handles inbound messages.
  void Send(const CastMessage& message) override;

  // Takes care of sending outbound messages.
  CastMessageChannel* const outbound_channel_;
  const ErrorCallback error_callback_;

  int32_t last_sequence_number_;

  // Holds callbacks for different types of responses.
  base::flat_map<ResponseType, ResponseCallback> callback_map_;

  DISALLOW_COPY_AND_ASSIGN(MessageDispatcher);
};

}  // namespace mirroring

#endif  // COMPONENTS_MIRRORING_SERVICE_MESSAGE_DISPATCHER_H_
