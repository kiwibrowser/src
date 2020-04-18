// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "device/geolocation/network_location_provider.h"

#include <utility>

#include "base/bind.h"
#include "base/location.h"
#include "base/single_thread_task_runner.h"
#include "base/strings/utf_string_conversions.h"
#include "base/threading/thread_task_runner_handle.h"
#include "base/time/time.h"
#include "device/geolocation/public/cpp/geoposition.h"
#include "net/traffic_annotation/network_traffic_annotation.h"
#include "net/url_request/url_request_context_getter.h"

namespace device {
namespace {
// The maximum period of time we'll wait for a complete set of wifi data
// before sending the request.
const int kDataCompleteWaitSeconds = 2;

// The maximum age of a cached network location estimate before it can no longer
// be returned as a fresh estimate. This should be at least as long as the
// longest polling interval used by the WifiDataProvider.
const int kLastPositionMaxAgeSeconds = 10 * 60;  // 10 minutes
}  // namespace

// static
const size_t NetworkLocationProvider::PositionCache::kMaximumSize = 10;

NetworkLocationProvider::PositionCache::PositionCache() = default;

NetworkLocationProvider::PositionCache::~PositionCache() = default;

bool NetworkLocationProvider::PositionCache::CachePosition(
    const WifiData& wifi_data,
    const mojom::Geoposition& position) {
  // Check that we can generate a valid key for the wifi data.
  base::string16 key;
  if (!MakeKey(wifi_data, &key)) {
    return false;
  }
  // If the cache is full, remove the oldest entry.
  if (cache_.size() == kMaximumSize) {
    DCHECK(cache_age_list_.size() == kMaximumSize);
    CacheAgeList::iterator oldest_entry = cache_age_list_.begin();
    DCHECK(oldest_entry != cache_age_list_.end());
    cache_.erase(*oldest_entry);
    cache_age_list_.erase(oldest_entry);
  }
  DCHECK_LT(cache_.size(), kMaximumSize);
  // Insert the position into the cache.
  std::pair<CacheMap::iterator, bool> result =
      cache_.insert(std::make_pair(key, position));
  if (!result.second) {
    NOTREACHED();  // We never try to add the same key twice.
    CHECK_EQ(cache_.size(), cache_age_list_.size());
    return false;
  }
  cache_age_list_.push_back(result.first);
  DCHECK_EQ(cache_.size(), cache_age_list_.size());
  return true;
}

// Searches for a cached position response for the current WiFi data. Returns
// the cached position if available, nullptr otherwise.
const mojom::Geoposition* NetworkLocationProvider::PositionCache::FindPosition(
    const WifiData& wifi_data) {
  base::string16 key;
  if (!MakeKey(wifi_data, &key)) {
    return nullptr;
  }
  CacheMap::const_iterator iter = cache_.find(key);
  return iter == cache_.end() ? nullptr : &iter->second;
}

// Makes the key for the map of cached positions, using the available data.
// Returns true if a good key was generated, false otherwise.
//
// static
bool NetworkLocationProvider::PositionCache::MakeKey(const WifiData& wifi_data,
                                                     base::string16* key) {
  // Currently we use only WiFi data and base the key only on the MAC addresses.
  DCHECK(key);
  key->clear();
  const size_t kCharsPerMacAddress = 6 * 3 + 1;  // e.g. "11:22:33:44:55:66|"
  key->reserve(wifi_data.access_point_data.size() * kCharsPerMacAddress);
  const base::string16 separator(base::ASCIIToUTF16("|"));
  for (const auto& access_point_data : wifi_data.access_point_data) {
    *key += separator;
    *key += access_point_data.mac_address;
    *key += separator;
  }
  // If the key is the empty string, return false, as we don't want to cache a
  // position for such data.
  return !key->empty();
}

// NetworkLocationProvider
NetworkLocationProvider::NetworkLocationProvider(
    scoped_refptr<net::URLRequestContextGetter> url_context_getter,
    const std::string& api_key,
    LastPositionCache* last_position_cache)
    : wifi_data_provider_manager_(nullptr),
      wifi_data_update_callback_(
          base::Bind(&NetworkLocationProvider::OnWifiDataUpdate,
                     base::Unretained(this))),
      is_wifi_data_complete_(false),
      last_position_delegate_(last_position_cache),
      is_permission_granted_(false),
      is_new_data_available_(false),
      request_(new NetworkLocationRequest(
          std::move(url_context_getter),
          api_key,
          base::Bind(&NetworkLocationProvider::OnLocationResponse,
                     base::Unretained(this)))),
      position_cache_(new PositionCache),
      weak_factory_(this) {
  DCHECK(last_position_delegate_);
}

NetworkLocationProvider::~NetworkLocationProvider() {
  DCHECK(thread_checker_.CalledOnValidThread());
  if (IsStarted())
    StopProvider();
}

void NetworkLocationProvider::SetUpdateCallback(
    const LocationProvider::LocationProviderUpdateCallback& callback) {
  DCHECK(thread_checker_.CalledOnValidThread());
  location_provider_update_callback_ = callback;
}

void NetworkLocationProvider::OnPermissionGranted() {
  const bool was_permission_granted = is_permission_granted_;
  is_permission_granted_ = true;
  if (!was_permission_granted && IsStarted())
    RequestPosition();
}

void NetworkLocationProvider::OnWifiDataUpdate() {
  DCHECK(thread_checker_.CalledOnValidThread());
  DCHECK(IsStarted());
  is_wifi_data_complete_ = wifi_data_provider_manager_->GetData(&wifi_data_);
  if (!is_wifi_data_complete_)
    return;

  wifi_timestamp_ = base::Time::Now();
  is_new_data_available_ = true;
  RequestPosition();
}

void NetworkLocationProvider::OnLocationResponse(
    const mojom::Geoposition& position,
    bool server_error,
    const WifiData& wifi_data) {
  DCHECK(thread_checker_.CalledOnValidThread());
  // Record the position and update our cache.
  last_position_delegate_->SetLastNetworkPosition(position);
  if (ValidateGeoposition(position))
    position_cache_->CachePosition(wifi_data, position);

  // Let listeners know that we now have a position available.
  if (!location_provider_update_callback_.is_null()) {
    location_provider_update_callback_.Run(
        this, last_position_delegate_->GetLastNetworkPosition());
  }
}

void NetworkLocationProvider::StartProvider(bool high_accuracy) {
  DCHECK(thread_checker_.CalledOnValidThread());

  if (IsStarted())
    return;

  // Registers a callback with the data provider. The first call to Register()
  // will create a singleton data provider that will be deleted on Unregister().
  wifi_data_provider_manager_ =
      WifiDataProviderManager::Register(&wifi_data_update_callback_);

  base::ThreadTaskRunnerHandle::Get()->PostDelayedTask(
      FROM_HERE,
      base::BindOnce(&NetworkLocationProvider::RequestPosition,
                     weak_factory_.GetWeakPtr()),
      base::TimeDelta::FromSeconds(kDataCompleteWaitSeconds));

  OnWifiDataUpdate();
}

void NetworkLocationProvider::StopProvider() {
  DCHECK(thread_checker_.CalledOnValidThread());
  DCHECK(IsStarted());
  wifi_data_provider_manager_->Unregister(&wifi_data_update_callback_);
  wifi_data_provider_manager_ = nullptr;
  weak_factory_.InvalidateWeakPtrs();
}

const mojom::Geoposition& NetworkLocationProvider::GetPosition() {
  return last_position_delegate_->GetLastNetworkPosition();
}

void NetworkLocationProvider::RequestPosition() {
  DCHECK(thread_checker_.CalledOnValidThread());

  // The wifi polling policy may require us to wait for several minutes before
  // fresh wifi data is available. To ensure we can return a position estimate
  // quickly when the network location provider is the primary provider, allow
  // a cached value to be returned under certain conditions.
  //
  // If we have a sufficiently recent network location estimate and we do not
  // expect to receive a new one soon (i.e., no new wifi data is available and
  // there is no pending network request), report the last network position
  // estimate as if it were a fresh estimate.
  const mojom::Geoposition& last_position =
      last_position_delegate_->GetLastNetworkPosition();
  if (!is_new_data_available_ && !request_->is_request_pending() &&
      ValidateGeoposition(last_position)) {
    base::Time now = base::Time::Now();
    base::TimeDelta last_position_age = now - last_position.timestamp;
    if (last_position_age.InSeconds() < kLastPositionMaxAgeSeconds &&
        !location_provider_update_callback_.is_null()) {
      // Update the timestamp to the current time.
      mojom::Geoposition position = last_position;
      position.timestamp = now;
      location_provider_update_callback_.Run(this, position);
    }
  }

  if (!is_new_data_available_ || !is_wifi_data_complete_)
    return;
  DCHECK(!wifi_timestamp_.is_null())
      << "|wifi_timestamp_| must be set before looking up position";

  const mojom::Geoposition* cached_position =
      position_cache_->FindPosition(wifi_data_);
  if (cached_position) {
    mojom::Geoposition position(*cached_position);
    DCHECK(ValidateGeoposition(position));
    // The timestamp of a position fix is determined by the timestamp
    // of the source data update. (The value of position.timestamp from
    // the cache could be from weeks ago!)
    position.timestamp = wifi_timestamp_;
    is_new_data_available_ = false;

    // Record the position.
    last_position_delegate_->SetLastNetworkPosition(position);

    // Let listeners know that we now have a position available.
    if (!location_provider_update_callback_.is_null())
      location_provider_update_callback_.Run(this, position);
    return;
  }
  // Don't send network requests until authorized. http://crbug.com/39171
  if (!is_permission_granted_)
    return;

  is_new_data_available_ = false;

  // TODO(joth): Rather than cancel pending requests, we should create a new
  // NetworkLocationRequest for each and hold a set of pending requests.
  DLOG_IF(WARNING, request_->is_request_pending())
      << "NetworkLocationProvider - pre-empting pending network request "
         "with new data. Wifi APs: "
      << wifi_data_.access_point_data.size();

  net::PartialNetworkTrafficAnnotationTag partial_traffic_annotation =
      net::DefinePartialNetworkTrafficAnnotation("network_location_provider",
                                                 "network_location_request",
                                                 R"(
      semantics {
        sender: "Network Location Provider"
      }
      policy {
        setting:
          "Users can control this feature via the Location setting under "
          "'Privacy', 'Content Settings', 'Location'."
        chrome_policy {
          DefaultGeolocationSetting {
            DefaultGeolocationSetting: 2
          }
        }
      })");
  request_->MakeRequest(wifi_data_, wifi_timestamp_,
                        partial_traffic_annotation);
}

bool NetworkLocationProvider::IsStarted() const {
  return wifi_data_provider_manager_ != nullptr;
}

}  // namespace device
