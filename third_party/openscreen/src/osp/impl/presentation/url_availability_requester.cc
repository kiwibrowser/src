// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "osp/impl/presentation/url_availability_requester.h"

#include <algorithm>
#include <chrono>
#include <memory>

#include "osp/impl/presentation/presentation_common.h"
#include "osp/public/network_service_manager.h"
#include "platform/api/logging.h"

using openscreen::platform::Clock;
using std::chrono::seconds;

namespace openscreen {
namespace presentation {
namespace {

static constexpr Clock::duration kWatchDuration = seconds(20);
static constexpr Clock::duration kWatchRefreshPadding = seconds(2);

std::vector<std::string>::iterator PartitionUrlsBySetMembership(
    std::vector<std::string>* urls,
    const std::set<std::string>& membership_test) {
  return std::partition(
      urls->begin(), urls->end(), [&membership_test](const std::string& url) {
        return membership_test.find(url) == membership_test.end();
      });
}

void MoveVectorSegment(std::vector<std::string>::iterator first,
                       std::vector<std::string>::iterator last,
                       std::set<std::string>* target) {
  for (auto it = first; it != last; ++it)
    target->emplace(std::move(*it));
}

uint64_t GetNextRequestId(const uint64_t endpoint_id) {
  return NetworkServiceManager::Get()
      ->GetProtocolConnectionClient()
      ->endpoint_request_ids()
      ->GetNextRequestId(endpoint_id);
}

}  // namespace

UrlAvailabilityRequester::UrlAvailabilityRequester(
    platform::ClockNowFunctionPtr now_function)
    : now_function_(now_function) {
  OSP_DCHECK(now_function_);
}

UrlAvailabilityRequester::~UrlAvailabilityRequester() = default;

void UrlAvailabilityRequester::AddObserver(const std::vector<std::string>& urls,
                                           ReceiverObserver* observer) {
  for (const auto& url : urls) {
    observers_by_url_[url].push_back(observer);
  }
  for (auto& entry : receiver_by_service_id_) {
    auto& receiver = entry.second;
    receiver->GetOrRequestAvailabilities(urls, observer);
  }
}

void UrlAvailabilityRequester::RemoveObserverUrls(
    const std::vector<std::string>& urls,
    ReceiverObserver* observer) {
  std::set<std::string> unobserved_urls;
  for (const auto& url : urls) {
    auto observer_entry = observers_by_url_.find(url);
    if (observer_entry == observers_by_url_.end())
      continue;
    auto& observers = observer_entry->second;
    observers.erase(std::remove(observers.begin(), observers.end(), observer),
                    observers.end());
    if (observers.empty()) {
      unobserved_urls.emplace(std::move(observer_entry->first));
      observers_by_url_.erase(observer_entry);
      for (auto& entry : receiver_by_service_id_) {
        auto& receiver = entry.second;
        receiver->known_availability_by_url.erase(url);
      }
    }
  }

  for (auto& entry : receiver_by_service_id_) {
    auto& receiver = entry.second;
    receiver->RemoveUnobservedRequests(unobserved_urls);
    receiver->RemoveUnobservedWatches(unobserved_urls);
  }
}

void UrlAvailabilityRequester::RemoveObserver(ReceiverObserver* observer) {
  std::set<std::string> unobserved_urls;
  for (auto& entry : observers_by_url_) {
    auto& observer_list = entry.second;
    auto it = std::remove(observer_list.begin(), observer_list.end(), observer);
    if (it != observer_list.end()) {
      observer_list.erase(it);
      if (observer_list.empty())
        unobserved_urls.insert(entry.first);
    }
  }

  for (auto& entry : receiver_by_service_id_) {
    auto& receiver = entry.second;
    receiver->RemoveUnobservedRequests(unobserved_urls);
    receiver->RemoveUnobservedWatches(unobserved_urls);
  }
}

void UrlAvailabilityRequester::AddReceiver(const ServiceInfo& info) {
  auto result = receiver_by_service_id_.emplace(
      info.service_id,
      std::make_unique<ReceiverRequester>(
          this, info.service_id,
          info.v4_endpoint.address ? info.v4_endpoint : info.v6_endpoint));
  std::unique_ptr<ReceiverRequester>& receiver = result.first->second;
  std::vector<std::string> urls;
  urls.reserve(observers_by_url_.size());
  for (const auto& url : observers_by_url_)
    urls.push_back(url.first);
  receiver->RequestUrlAvailabilities(std::move(urls));
}

void UrlAvailabilityRequester::ChangeReceiver(const ServiceInfo& info) {}

void UrlAvailabilityRequester::RemoveReceiver(const ServiceInfo& info) {
  auto receiver_entry = receiver_by_service_id_.find(info.service_id);
  if (receiver_entry != receiver_by_service_id_.end()) {
    auto& receiver = receiver_entry->second;
    receiver->RemoveReceiver();
    receiver_by_service_id_.erase(receiver_entry);
  }
}

void UrlAvailabilityRequester::RemoveAllReceivers() {
  for (auto& entry : receiver_by_service_id_) {
    auto& receiver = entry.second;
    receiver->RemoveReceiver();
  }
  receiver_by_service_id_.clear();
}

Clock::time_point UrlAvailabilityRequester::RefreshWatches() {
  const Clock::time_point now = now_function_();
  Clock::time_point minimum_schedule_time = now + kWatchDuration;
  for (auto& entry : receiver_by_service_id_) {
    auto& receiver = entry.second;
    const Clock::time_point requested_schedule_time =
        receiver->RefreshWatches(now);
    if (requested_schedule_time < minimum_schedule_time)
      minimum_schedule_time = requested_schedule_time;
  }
  return minimum_schedule_time;
}

UrlAvailabilityRequester::ReceiverRequester::ReceiverRequester(
    UrlAvailabilityRequester* listener,
    const std::string& service_id,
    const IPEndpoint& endpoint)
    : listener(listener),
      service_id(service_id),
      connect_request(
          NetworkServiceManager::Get()->GetProtocolConnectionClient()->Connect(
              endpoint,
              this)) {}

UrlAvailabilityRequester::ReceiverRequester::~ReceiverRequester() = default;

void UrlAvailabilityRequester::ReceiverRequester::GetOrRequestAvailabilities(
    const std::vector<std::string>& requested_urls,
    ReceiverObserver* observer) {
  std::vector<std::string> unknown_urls;
  for (const auto& url : requested_urls) {
    auto availability_entry = known_availability_by_url.find(url);
    if (availability_entry == known_availability_by_url.end()) {
      unknown_urls.emplace_back(url);
      continue;
    }

    msgs::UrlAvailability availability = availability_entry->second;
    if (observer) {
      switch (availability) {
        case msgs::UrlAvailability::kAvailable:
          observer->OnReceiverAvailable(url, service_id);
          break;
        case msgs::UrlAvailability::kUnavailable:
        case msgs::UrlAvailability::kInvalid:
          observer->OnReceiverUnavailable(url, service_id);
          break;
      }
    }
  }
  if (!unknown_urls.empty()) {
    RequestUrlAvailabilities(std::move(unknown_urls));
  }
}

void UrlAvailabilityRequester::ReceiverRequester::RequestUrlAvailabilities(
    std::vector<std::string> urls) {
  if (urls.empty())
    return;
  const uint64_t request_id = GetNextRequestId(endpoint_id);
  ErrorOr<uint64_t> watch_id_or_error(0);
  if (!connection || (watch_id_or_error = SendRequest(request_id, urls))) {
    request_by_id.emplace(request_id,
                          Request{watch_id_or_error.value(), std::move(urls)});
  } else {
    for (const auto& url : urls)
      for (auto& observer : listener->observers_by_url_[url])
        observer->OnRequestFailed(url, service_id);
  }
}

ErrorOr<uint64_t> UrlAvailabilityRequester::ReceiverRequester::SendRequest(
    uint64_t request_id,
    const std::vector<std::string>& urls) {
  uint64_t watch_id = next_watch_id++;
  msgs::PresentationUrlAvailabilityRequest cbor_request;
  cbor_request.request_id = request_id;
  cbor_request.urls = urls;
  cbor_request.watch_id = watch_id;

  msgs::CborEncodeBuffer buffer;
  if (msgs::EncodePresentationUrlAvailabilityRequest(cbor_request, &buffer)) {
    OSP_VLOG << "writing presentation-url-availability-request";
    connection->Write(buffer.data(), buffer.size());
    watch_by_id.emplace(
        watch_id, Watch{listener->now_function_() + kWatchDuration, urls});
    if (!event_watch) {
      event_watch = GetClientDemuxer()->WatchMessageType(
          endpoint_id, msgs::Type::kPresentationUrlAvailabilityEvent, this);
    }
    if (!response_watch) {
      response_watch = GetClientDemuxer()->WatchMessageType(
          endpoint_id, msgs::Type::kPresentationUrlAvailabilityResponse, this);
    }
    return watch_id;
  }
  return Error::Code::kCborEncoding;
}

Clock::time_point UrlAvailabilityRequester::ReceiverRequester::RefreshWatches(
    Clock::time_point now) {
  Clock::time_point minimum_schedule_time = now + kWatchDuration;
  std::vector<std::vector<std::string>> new_requests;
  for (auto entry = watch_by_id.begin(); entry != watch_by_id.end();) {
    Watch& watch = entry->second;
    const Clock::time_point buffered_deadline =
        watch.deadline - kWatchRefreshPadding;
    if (now > buffered_deadline) {
      new_requests.emplace_back(std::move(watch.urls));
      entry = watch_by_id.erase(entry);
    } else {
      ++entry;
      if (buffered_deadline < minimum_schedule_time)
        minimum_schedule_time = buffered_deadline;
    }
  }
  if (watch_by_id.empty())
    StopWatching(&event_watch);

  for (auto& request : new_requests)
    RequestUrlAvailabilities(std::move(request));

  return minimum_schedule_time;
}

Error::Code UrlAvailabilityRequester::ReceiverRequester::UpdateAvailabilities(
    const std::vector<std::string>& urls,
    const std::vector<msgs::UrlAvailability>& availabilities) {
  auto availability_it = availabilities.begin();
  if (urls.size() != availabilities.size()) {
    return Error::Code::kCborInvalidMessage;
  }
  for (const auto& url : urls) {
    auto observer_entry = listener->observers_by_url_.find(url);
    if (observer_entry == listener->observers_by_url_.end())
      continue;
    std::vector<ReceiverObserver*>& observers = observer_entry->second;
    auto result = known_availability_by_url.emplace(url, *availability_it);
    auto entry = result.first;
    bool inserted = result.second;
    bool updated = (entry->second != *availability_it);
    if (inserted || updated) {
      switch (*availability_it) {
        case msgs::UrlAvailability::kAvailable:
          for (auto* observer : observers)
            observer->OnReceiverAvailable(url, service_id);
          break;
        case msgs::UrlAvailability::kUnavailable:
        case msgs::UrlAvailability::kInvalid:
          for (auto* observer : observers)
            observer->OnReceiverUnavailable(url, service_id);
          break;
        default:
          break;
      }
    }
    ++availability_it;
  }
  return Error::Code::kNone;
}

void UrlAvailabilityRequester::ReceiverRequester::RemoveUnobservedRequests(
    const std::set<std::string>& unobserved_urls) {
  std::map<uint64_t, Request> new_requests;
  std::set<std::string> still_observed_urls;
  for (auto entry = request_by_id.begin(); entry != request_by_id.end();
       ++entry) {
    Request& request = entry->second;
    auto split = PartitionUrlsBySetMembership(&request.urls, unobserved_urls);
    if (split == request.urls.end())
      continue;
    MoveVectorSegment(request.urls.begin(), split, &still_observed_urls);
    if (connection)
      watch_by_id.erase(request.watch_id);
  }
  if (!still_observed_urls.empty()) {
    const uint64_t new_request_id = GetNextRequestId(endpoint_id);
    ErrorOr<uint64_t> watch_id_or_error(0);
    std::vector<std::string> urls;
    urls.reserve(still_observed_urls.size());
    for (auto& url : still_observed_urls)
      urls.emplace_back(std::move(url));
    if (!connection ||
        (watch_id_or_error = SendRequest(new_request_id, urls))) {
      new_requests.emplace(new_request_id,
                           Request{watch_id_or_error.value(), std::move(urls)});
    } else {
      for (const auto& url : urls)
        for (auto& observer : listener->observers_by_url_[url])
          observer->OnRequestFailed(url, service_id);
    }
  }

  for (auto& entry : new_requests)
    request_by_id.emplace(entry.first, std::move(entry.second));

  if (request_by_id.empty())
    StopWatching(&response_watch);
}

void UrlAvailabilityRequester::ReceiverRequester::RemoveUnobservedWatches(
    const std::set<std::string>& unobserved_urls) {
  std::set<std::string> still_observed_urls;
  for (auto entry = watch_by_id.begin(); entry != watch_by_id.end();) {
    Watch& watch = entry->second;
    auto split = PartitionUrlsBySetMembership(&watch.urls, unobserved_urls);
    if (split == watch.urls.end()) {
      ++entry;
      continue;
    }
    MoveVectorSegment(watch.urls.begin(), split, &still_observed_urls);
    entry = watch_by_id.erase(entry);
  }

  std::vector<std::string> urls;
  urls.reserve(still_observed_urls.size());
  for (auto& url : still_observed_urls)
    urls.emplace_back(std::move(url));
  RequestUrlAvailabilities(std::move(urls));
  // TODO(btolsch): These message watch cancels could be tested by expecting
  // messages to fall through to the default watch.
  if (watch_by_id.empty())
    StopWatching(&event_watch);
}

void UrlAvailabilityRequester::ReceiverRequester::RemoveReceiver() {
  for (const auto& availability : known_availability_by_url) {
    if (availability.second == msgs::UrlAvailability::kAvailable) {
      const std::string& url = availability.first;
      for (auto& observer : listener->observers_by_url_[url])
        observer->OnReceiverUnavailable(url, service_id);
    }
  }
}

void UrlAvailabilityRequester::ReceiverRequester::OnConnectionOpened(
    uint64_t request_id,
    std::unique_ptr<ProtocolConnection> connection) {
  connect_request.MarkComplete();
  // TODO(btolsch): This is one place where we need to make sure the QUIC
  // connection stays alive, even without constant traffic.
  endpoint_id = connection->endpoint_id();
  this->connection = std::move(connection);
  ErrorOr<uint64_t> watch_id_or_error(0);
  for (auto entry = request_by_id.begin(); entry != request_by_id.end();) {
    if ((watch_id_or_error = SendRequest(entry->first, entry->second.urls))) {
      entry->second.watch_id = watch_id_or_error.value();
      ++entry;
    } else {
      entry = request_by_id.erase(entry);
    }
  }
}

void UrlAvailabilityRequester::ReceiverRequester::OnConnectionFailed(
    uint64_t request_id) {
  connect_request.MarkComplete();

  std::set<std::string> waiting_urls;
  for (auto& entry : request_by_id) {
    Request& request = entry.second;
    for (auto& url : request.urls) {
      waiting_urls.emplace(std::move(url));
    }
  }
  for (const auto& url : waiting_urls)
    for (auto& observer : listener->observers_by_url_[url])
      observer->OnRequestFailed(url, service_id);

  std::string id = std::move(service_id);
  listener->receiver_by_service_id_.erase(id);
}

ErrorOr<size_t> UrlAvailabilityRequester::ReceiverRequester::OnStreamMessage(
    uint64_t endpoint_id,
    uint64_t connection_id,
    msgs::Type message_type,
    const uint8_t* buffer,
    size_t buffer_size,
    Clock::time_point now) {
  switch (message_type) {
    case msgs::Type::kPresentationUrlAvailabilityResponse: {
      msgs::PresentationUrlAvailabilityResponse response;
      ssize_t result = msgs::DecodePresentationUrlAvailabilityResponse(
          buffer, buffer_size, &response);
      if (result < 0) {
        if (result == msgs::kParserEOF)
          return Error::Code::kCborIncompleteMessage;
        OSP_LOG_WARN << "parse error: " << result;
        return Error::Code::kCborParsing;
      } else {
        auto request_entry = request_by_id.find(response.request_id);
        if (request_entry == request_by_id.end()) {
          OSP_LOG_ERROR << "bad response id: " << response.request_id;
          return Error::Code::kCborInvalidResponseId;
        }
        std::vector<std::string>& urls = request_entry->second.urls;
        if (urls.size() != response.url_availabilities.size()) {
          OSP_LOG_WARN << "bad response size: expected " << urls.size()
                       << " but got " << response.url_availabilities.size();
          return Error::Code::kCborInvalidMessage;
        }
        Error::Code update_result =
            UpdateAvailabilities(urls, response.url_availabilities);
        if (update_result != Error::Code::kNone) {
          return update_result;
        }
        request_by_id.erase(response.request_id);
        if (request_by_id.empty())
          StopWatching(&response_watch);
        return result;
      }
    } break;
    case msgs::Type::kPresentationUrlAvailabilityEvent: {
      msgs::PresentationUrlAvailabilityEvent event;
      ssize_t result = msgs::DecodePresentationUrlAvailabilityEvent(
          buffer, buffer_size, &event);
      if (result < 0) {
        if (result == msgs::kParserEOF)
          return Error::Code::kCborIncompleteMessage;
        OSP_LOG_WARN << "parse error: " << result;
        return Error::Code::kCborParsing;
      } else {
        auto watch_entry = watch_by_id.find(event.watch_id);
        if (watch_entry != watch_by_id.end()) {
          std::vector<std::string> urls = watch_entry->second.urls;
          Error::Code update_result =
              UpdateAvailabilities(urls, event.url_availabilities);
          if (update_result != Error::Code::kNone) {
            return update_result;
          }
        }
        return result;
      }
    } break;
    default:
      break;
  }
  return Error::Code::kCborParsing;
}

}  // namespace presentation
}  // namespace openscreen
