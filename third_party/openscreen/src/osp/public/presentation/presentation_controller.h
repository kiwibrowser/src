// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef OSP_PUBLIC_PRESENTATION_PRESENTATION_CONTROLLER_H_
#define OSP_PUBLIC_PRESENTATION_PRESENTATION_CONTROLLER_H_

#include <map>
#include <memory>
#include <string>

#include "absl/strings/string_view.h"
#include "absl/types/optional.h"
#include "osp/public/presentation/presentation_connection.h"
#include "osp/public/protocol_connection.h"
#include "osp/public/service_listener.h"
#include "osp_base/error.h"
#include "platform/api/time.h"

namespace openscreen {
namespace presentation {

class UrlAvailabilityRequester;

class RequestDelegate {
 public:
  virtual ~RequestDelegate() = default;

  virtual void OnConnection(std::unique_ptr<Connection> connection) = 0;
  virtual void OnError(const Error& error) = 0;
};

class ReceiverObserver {
 public:
  virtual ~ReceiverObserver() = default;

  // Called when there is an unrecoverable error in requesting availability.
  // This means the availability is unknown and there is no further response to
  // wait for.
  virtual void OnRequestFailed(const std::string& presentation_url,
                               const std::string& service_id) = 0;

  // Called when receivers compatible with |presentation_url| are known to be
  // available.
  virtual void OnReceiverAvailable(const std::string& presentation_url,
                                   const std::string& service_id) = 0;
  // Only called for |service_id| values previously advertised as available.
  virtual void OnReceiverUnavailable(const std::string& presentation_url,
                                     const std::string& service_id) = 0;
};

class Controller final : public ServiceListener::Observer,
                         public Connection::ParentDelegate {
 public:
  class ReceiverWatch {
   public:
    ReceiverWatch();
    ReceiverWatch(Controller* controller,
                  const std::vector<std::string>& urls,
                  ReceiverObserver* observer);
    ReceiverWatch(ReceiverWatch&&);
    ~ReceiverWatch();

    ReceiverWatch& operator=(ReceiverWatch);

    explicit operator bool() const { return observer_; }

    friend void swap(ReceiverWatch& a, ReceiverWatch& b);

   private:
    std::vector<std::string> urls_;
    ReceiverObserver* observer_ = nullptr;
    Controller* controller_ = nullptr;
  };

  class ConnectRequest {
   public:
    ConnectRequest();
    ConnectRequest(Controller* controller,
                   const std::string& service_id,
                   bool is_reconnect,
                   absl::optional<uint64_t> request_id);
    ConnectRequest(ConnectRequest&&);
    ~ConnectRequest();

    ConnectRequest& operator=(ConnectRequest);

    explicit operator bool() const { return request_id_.has_value(); }

    friend void swap(ConnectRequest& a, ConnectRequest& b);

   private:
    std::string service_id_;
    bool is_reconnect_;
    absl::optional<uint64_t> request_id_;
    Controller* controller_;
  };

  explicit Controller(platform::ClockNowFunctionPtr now_function);
  ~Controller();

  // Requests receivers compatible with all urls in |urls| and registers
  // |observer| for availability changes.  The screens will be a subset of the
  // screen list maintained by the ServiceListener.  Returns an RAII object that
  // tracks the registration.
  ReceiverWatch RegisterReceiverWatch(const std::vector<std::string>& urls,
                                      ReceiverObserver* observer);

  // Requests that a new presentation be created on |service_id| using
  // |presentation_url|, with the result passed to |delegate|.
  // |conn_delegate| is passed to the resulting connection.  The returned
  // ConnectRequest object may be destroyed before any |delegate| methods are
  // called to cancel the request.
  ConnectRequest StartPresentation(const std::string& url,
                                   const std::string& service_id,
                                   RequestDelegate* delegate,
                                   Connection::Delegate* conn_delegate);

  // Requests reconnection to the presentation with the given id and URL running
  // on |service_id|, with the result passed to |delegate|.  |conn_delegate| is
  // passed to the resulting connection.  The returned ConnectRequest object may
  // be destroyed before any |delegate| methods are called to cancel the
  // request.
  ConnectRequest ReconnectPresentation(const std::vector<std::string>& urls,
                                       const std::string& presentation_id,
                                       const std::string& service_id,
                                       RequestDelegate* delegate,
                                       Connection::Delegate* conn_delegate);

  // Requests reconnection with a previously-connected connection.  This both
  // avoids having to respecify the parameters and connection delegate but also
  // simplifies the implementation of the Presentation API requirement to return
  // the same connection object where possible.
  ConnectRequest ReconnectConnection(std::unique_ptr<Connection> connection,
                                     RequestDelegate* delegate);

  // Connection::ParentDelegate overrides.
  Error CloseConnection(Connection* connection,
                        Connection::CloseReason reason) override;

  // Also called by the embedder to report that a presentation has been
  // terminated.
  Error OnPresentationTerminated(const std::string& presentation_id,
                                 TerminationReason reason) override;

  void OnConnectionDestroyed(Connection* connection) override;

  // Returns an empty string if no such presentation ID is found.
  std::string GetServiceIdForPresentationId(
      const std::string& presentation_id) const;

  ProtocolConnection* GetConnectionRequestGroupStream(
      const std::string& service_id);

  // TODO(btolsch): still used?
  void SetConnectionRequestGroupStreamForTest(
      const std::string& service_id,
      std::unique_ptr<ProtocolConnection> stream);

 private:
  class TerminationListener;
  class MessageGroupStreams;

  struct ControlledPresentation {
    std::string service_id;
    std::string url;
    std::vector<Connection*> connections;
  };

  static std::string MakePresentationId(const std::string& url,
                                        const std::string& service_id);

  void OpenConnection(uint64_t connection_id,
                      uint64_t endpoint_id,
                      const std::string& service_id,
                      RequestDelegate* request_delegate,
                      std::unique_ptr<Connection>&& connection,
                      std::unique_ptr<ProtocolConnection>&& stream);

  void TerminatePresentationById(const std::string& presentation_id);

  // Cancels compatible receiver monitoring for the given |urls|, |observer|
  // pair.
  void CancelReceiverWatch(const std::vector<std::string>& urls,
                           ReceiverObserver* observer);

  // Cancels a presentation connect request for the given |request_id| if one is
  // pending.
  void CancelConnectRequest(const std::string& service_id,
                            bool is_reconnect,
                            uint64_t request_id);

  // ServiceListener::Observer overrides.
  void OnStarted() override;
  void OnStopped() override;
  void OnSuspended() override;
  void OnSearching() override;
  void OnReceiverAdded(const ServiceInfo& info) override;
  void OnReceiverChanged(const ServiceInfo& info) override;
  void OnReceiverRemoved(const ServiceInfo& info) override;
  void OnAllReceiversRemoved() override;
  void OnError(ServiceListenerError) override;
  void OnMetrics(ServiceListener::Metrics) override;

  std::map<std::string, uint64_t> next_connection_id_;

  std::map<std::string, ControlledPresentation> presentations_;

  std::unique_ptr<ConnectionManager> connection_manager_;

  std::unique_ptr<UrlAvailabilityRequester> availability_requester_;
  std::map<std::string, IPEndpoint> receiver_endpoints_;

  std::map<std::string, std::unique_ptr<MessageGroupStreams>> group_streams_;
  std::map<std::string, std::unique_ptr<TerminationListener>>
      termination_listener_by_id_;
};

}  // namespace presentation
}  // namespace openscreen

#endif  // OSP_PUBLIC_PRESENTATION_PRESENTATION_CONTROLLER_H_
