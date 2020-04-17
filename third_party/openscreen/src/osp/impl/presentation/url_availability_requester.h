// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef OSP_IMPL_PRESENTATION_URL_AVAILABILITY_REQUESTER_H_
#define OSP_IMPL_PRESENTATION_URL_AVAILABILITY_REQUESTER_H_

#include <map>
#include <memory>
#include <set>
#include <string>

#include "osp/msgs/osp_messages.h"
#include "osp/public/message_demuxer.h"
#include "osp/public/presentation/presentation_controller.h"
#include "osp/public/protocol_connection_client.h"
#include "osp/public/service_info.h"
#include "osp_base/error.h"
#include "platform/api/time.h"

namespace openscreen {
namespace presentation {

// Handles Presentation API URL availability requests and persistent watches.
// It keeps track of the set of currently known receivers as well as all
// registered URLs and observers in order to query each receiver with all URLs.
// It uses the availability protocol message watch mechanism to stay informed of
// any availability changes as long as at least one observer is registered for a
// given URL.
class UrlAvailabilityRequester {
 public:
  explicit UrlAvailabilityRequester(platform::ClockNowFunctionPtr now_function);
  ~UrlAvailabilityRequester();

  // Adds a persistent availability request for |urls| to all known receivers.
  // These URLs will also be queried for any receivers discovered in the future.
  // |observer| will be called back once for the first known availability (which
  // may be cached from previous requests) and when the availability of any of
  // these URLs changes on any receiver.
  void AddObserver(const std::vector<std::string>& urls,
                   ReceiverObserver* observer);

  // Disassociates |observer| from all the URLs in |urls| so it will no longer
  // receive availability updates for these URLs.  Additionally, if |urls| is
  // only a subset of the URL list it was originally added with, it will still
  // be observing the URLs not included here.
  void RemoveObserverUrls(const std::vector<std::string>& urls,
                          ReceiverObserver* observer);

  // Disassociates |observer| from all the URLs it is observing.  This
  // guarantees that it is safe to delete |observer| after this call.
  void RemoveObserver(ReceiverObserver* observer);

  // Informs the UrlAvailabilityRequester of changes to the set of known
  // receivers.  New receivers are immediately queried for all currently
  // observed URLs and removed receivers cause any URLs that were available on
  // that receiver to become unavailable.
  void AddReceiver(const ServiceInfo& info);
  void ChangeReceiver(const ServiceInfo& info);
  void RemoveReceiver(const ServiceInfo& info);
  void RemoveAllReceivers();

  // Ensures that all open availability watches (to all receivers) that are
  // about to expire are refreshed by sending a new request with the same URLs.
  // Returns the time point at which this should next be scheduled to run.
  platform::Clock::time_point RefreshWatches();

 private:
  // Handles Presentation API URL availability requests and watches for one
  // particular receiver.  When first constructed, it attempts to open a
  // ProtocolConnection to the receiver, then it makes an availability request
  // for all the observed URLs, then it continues to listen for update events
  // during the following watch period.  Before a watch will expire, it needs to
  // send a new request to restart the watch, as long as there are active
  // observers for a given URL.
  struct ReceiverRequester final
      : ProtocolConnectionClient::ConnectionRequestCallback,
        MessageDemuxer::MessageCallback {
    struct Request {
      uint64_t watch_id;
      std::vector<std::string> urls;
    };

    struct Watch {
      platform::Clock::time_point deadline;
      std::vector<std::string> urls;
    };

    ReceiverRequester(UrlAvailabilityRequester* listener,
                      const std::string& service_id,
                      const IPEndpoint& endpoint);
    ~ReceiverRequester() override;

    void GetOrRequestAvailabilities(
        const std::vector<std::string>& requested_urls,
        ReceiverObserver* observer);
    void RequestUrlAvailabilities(std::vector<std::string> urls);
    ErrorOr<uint64_t> SendRequest(uint64_t request_id,
                                  const std::vector<std::string>& urls);
    platform::Clock::time_point RefreshWatches(platform::Clock::time_point now);
    Error::Code UpdateAvailabilities(
        const std::vector<std::string>& urls,
        const std::vector<msgs::UrlAvailability>& availabilities);
    void RemoveUnobservedRequests(const std::set<std::string>& unobserved_urls);
    void RemoveUnobservedWatches(const std::set<std::string>& unobserved_urls);
    void RemoveReceiver();

    // ProtocolConnectionClient::ConnectionRequestCallback overrides.
    void OnConnectionOpened(
        uint64_t request_id,
        std::unique_ptr<ProtocolConnection> connection) override;
    void OnConnectionFailed(uint64_t request_id) override;

    // MessageDemuxer::MessageCallback overrides.
    ErrorOr<size_t> OnStreamMessage(uint64_t endpoint_id,
                                    uint64_t connection_id,
                                    msgs::Type message_type,
                                    const uint8_t* buffer,
                                    size_t buffer_size,
                                    platform::Clock::time_point now) override;

    UrlAvailabilityRequester* const listener;

    uint64_t next_watch_id = 1;

    const std::string service_id;
    uint64_t endpoint_id;

    ProtocolConnectionClient::ConnectRequest connect_request;
    // TODO(btolsch): Observe connection and restart all the things on close.
    std::unique_ptr<ProtocolConnection> connection;

    MessageDemuxer::MessageWatch response_watch;
    std::map<uint64_t, Request> request_by_id;
    MessageDemuxer::MessageWatch event_watch;
    std::map<uint64_t, Watch> watch_by_id;

    std::map<std::string, msgs::UrlAvailability> known_availability_by_url;
  };

  const platform::ClockNowFunctionPtr now_function_;

  std::map<std::string, std::vector<ReceiverObserver*>> observers_by_url_;

  std::map<std::string, std::unique_ptr<ReceiverRequester>>
      receiver_by_service_id_;
};

}  // namespace presentation
}  // namespace openscreen

#endif  // OSP_IMPL_PRESENTATION_URL_AVAILABILITY_REQUESTER_H_
