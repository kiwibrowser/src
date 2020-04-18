// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_CAST_CHANNEL_CAST_MESSAGE_HANDLER_H_
#define COMPONENTS_CAST_CHANNEL_CAST_MESSAGE_HANDLER_H_

#include "base/callback_list.h"
#include "base/containers/flat_map.h"
#include "base/containers/flat_set.h"
#include "base/gtest_prod_util.h"
#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "base/sequence_checker.h"
#include "base/time/tick_clock.h"
#include "base/values.h"
#include "components/cast_channel/cast_message_util.h"
#include "components/cast_channel/cast_socket.h"

namespace cast_channel {

class CastSocketService;

template <typename CallbackType>
struct PendingRequest {
 public:
  PendingRequest(int request_id,
                 CallbackType callback,
                 const base::TickClock* clock)
      : request_id(request_id),
        callback(std::move(callback)),
        timeout_timer(clock) {}

  virtual ~PendingRequest() = default;

  int request_id;
  CallbackType callback;
  base::OneShotTimer timeout_timer;
};

// |app_id|: ID of app the result is for.
// |result|: Availability result from the receiver.
using GetAppAvailabilityCallback =
    base::OnceCallback<void(const std::string& app_id,
                            GetAppAvailabilityResult result)>;

// Represents an app availability request to a Cast sink.
struct GetAppAvailabilityRequest
    : public PendingRequest<GetAppAvailabilityCallback> {
 public:
  GetAppAvailabilityRequest(int request_id,
                            GetAppAvailabilityCallback callback,
                            const base::TickClock* clock,
                            const std::string& app_id);
  ~GetAppAvailabilityRequest() override;

  // App ID of the request.
  std::string app_id;
};

// Represents an app launch request to a Cast sink.
using LaunchSessionCallback =
    base::OnceCallback<void(LaunchSessionResponse response)>;
using LaunchSessionRequest = PendingRequest<LaunchSessionCallback>;

// Represents a virtual connection on a cast channel. A virtual connection is
// given by a source and destination ID pair, and must be created before
// messages can be sent. Virtual connections are managed by CastMessageHandler.
struct VirtualConnection {
  VirtualConnection(int channel_id,
                    const std::string& source_id,
                    const std::string& destination_id);
  ~VirtualConnection();

  bool operator<(const VirtualConnection& other) const;

  // ID of cast channel.
  int channel_id;

  // Source ID (e.g. sender-0).
  std::string source_id;

  // Destination ID (e.g. receiver-0).
  std::string destination_id;
};

struct InternalMessage {
  InternalMessage(CastMessageType type, base::Value message);
  ~InternalMessage();

  CastMessageType type;
  base::Value message;
};

// Handles messages that are sent between this browser instance and the Cast
// devices connected to it. This class also manages virtual connections (VCs)
// with each connected Cast device and ensures a proper VC exists before the
// message is sent. This makes the concept of VC transparent to the client.
// This class may be created on any sequence, but other methods (including
// destructor) must be run on the same sequence that CastSocketService runs on.
class CastMessageHandler : public CastSocket::Observer {
 public:
  class Observer {
   public:
    virtual ~Observer() = default;
    virtual void OnAppMessage(int channel_id, const CastMessage& message) = 0;
    virtual void OnInternalMessage(int channel_id,
                                   const InternalMessage& message) = 0;
  };

  explicit CastMessageHandler(CastSocketService* socket_service,
                              const std::string& user_agent,
                              const std::string& browser_version,
                              const std::string& locale);
  ~CastMessageHandler() override;

  // Ensures a virtual connection exists for (|source_id|, |destination_id|) on
  // the device given by |channel_id|, sending a virtual connection request to
  // the device if necessary. Although a virtual connection is automatically
  // created when sending a message, a caller may decide to create it beforehand
  // in order to receive messages sooner.
  void EnsureConnection(int channel_id,
                        const std::string& source_id,
                        const std::string& destination_id);

  // Sends an app availability for |app_id| to the device given by |socket|.
  // |callback| is always invoked asynchronously, and will be invoked when a
  // response is received, or if the request timed out. No-ops if there is
  // already a pending request with the same socket and app ID.
  virtual void RequestAppAvailability(CastSocket* socket,
                                      const std::string& app_id,
                                      GetAppAvailabilityCallback callback);

  // Sends a broadcast message containing |app_ids| and |request| to the socket
  // given by |channel_id|.
  virtual void SendBroadcastMessage(int channel_id,
                                    const std::vector<std::string>& app_ids,
                                    const BroadcastRequest& request);

  // Requests a session launch for |app_id| on the device given by |channel_id|.
  // |callback| will be invoked with the response or with a timed out result if
  // no response comes back before |launch_timeout|.
  void LaunchSession(int channel_id,
                     const std::string& app_id,
                     base::TimeDelta launch_timeout,
                     LaunchSessionCallback callback);

  // Sends |message| to the device given by |channel_id|. The caller may use
  // this method to forward app messages from the SDK client to the device. It
  // is invalid to call this method with a message in one of the Cast internal
  // message namespaces.
  void SendAppMessage(int channel_id, const CastMessage& message);

  void AddObserver(Observer* observer);
  void RemoveObserver(Observer* observer);

  const std::string& sender_id() const { return sender_id_; }

 private:
  friend class CastMessageHandlerTest;

  // Set of PendingRequests for a CastSocket.
  class PendingRequests {
   public:
    PendingRequests();
    ~PendingRequests();

    bool AddAppAvailabilityRequest(
        std::unique_ptr<GetAppAvailabilityRequest> request);
    bool AddLaunchRequest(std::unique_ptr<LaunchSessionRequest> request,
                          base::TimeDelta timeout);

    void HandlePendingRequest(int request_id,
                              const base::DictionaryValue& response);

   private:
    // Invokes the pending callback associated with |request_id| with a timed
    // out result.
    void AppAvailabilityTimedOut(int request_id);
    void LaunchSessionTimedOut(int request_id);

    // App availability requests pending responses, keyed by app ID.
    base::flat_map<std::string, std::unique_ptr<GetAppAvailabilityRequest>>
        pending_app_availability_requests;

    std::unique_ptr<LaunchSessionRequest> pending_launch_session_request;
  };

  // Used internally to generate the next ID to use in a request type message.
  int NextRequestId() { return ++next_request_id_; }

  PendingRequests* GetOrCreatePendingRequests(int channel_id);

  // CastSocket::Observer implementation.
  void OnError(const CastSocket& socket, ChannelError error_state) override;
  void OnMessage(const CastSocket& socket, const CastMessage& message) override;

  // Sends |message| over |socket|. This also ensures the necessary virtual
  // connection exists before sending the message.
  void SendCastMessage(CastSocket* socket, const CastMessage& message);

  // Sends a virtual connection request to |socket| if the virtual connection
  // for (|source_id|, |destination_id|) does not yet exist.
  void DoEnsureConnection(CastSocket* socket,
                          const std::string& source_id,
                          const std::string& destination_id);

  // Callback for CastTransport::SendMessage.
  void OnMessageSent(int result);

  void HandleCastInternalMessage(const CastSocket& socket,
                                 const CastMessage& message);

  // Set of pending requests keyed by socket ID.
  base::flat_map<int, std::unique_ptr<PendingRequests>> pending_requests_;

  // Source ID used for platform messages. The suffix is randomized to
  // distinguish it from other Cast senders on the same network.
  const std::string sender_id_;

  // User agent and browser version strings included in virtual connection
  // messages.
  const std::string user_agent_;
  const std::string browser_version_;

  // Locale string used for session launch requests.
  const std::string locale_;

  int next_request_id_ = 0;

  base::ObserverList<Observer> observers_;

  // Set of virtual connections opened to receivers.
  base::flat_set<VirtualConnection> virtual_connections_;

  CastSocketService* const socket_service_;

  // Non-owned pointer to TickClock used for request timeouts.
  const base::TickClock* const clock_;

  SEQUENCE_CHECKER(sequence_checker_);
  base::WeakPtrFactory<CastMessageHandler> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(CastMessageHandler);
};

}  // namespace cast_channel

#endif  // COMPONENTS_CAST_CHANNEL_CAST_MESSAGE_HANDLER_H_
