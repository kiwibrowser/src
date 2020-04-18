// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/network/throttling/throttling_controller.h"

#include <utility>

#include "net/http/http_request_info.h"
#include "services/network/throttling/network_conditions.h"
#include "services/network/throttling/throttling_network_interceptor.h"

namespace network {

ThrottlingController* ThrottlingController::instance_ = nullptr;

ThrottlingController::ThrottlingController() = default;
ThrottlingController::~ThrottlingController() = default;

// static
ThrottlingNetworkInterceptor* ThrottlingController::GetInterceptor(
    const std::string& client_id) {
  if (!instance_ || client_id.empty())
    return nullptr;
  return instance_->FindInterceptor(client_id);
}

// static
void ThrottlingController::SetConditions(
    const std::string& client_id,
    std::unique_ptr<NetworkConditions> conditions) {
  if (!instance_) {
    if (!conditions)
      return;
    instance_ = new ThrottlingController();
  }
  instance_->SetNetworkConditions(client_id, std::move(conditions));
}

ThrottlingNetworkInterceptor* ThrottlingController::FindInterceptor(
    const std::string& client_id) {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  auto it = interceptors_.find(client_id);
  return it != interceptors_.end() ? it->second.get() : nullptr;
}

void ThrottlingController::SetNetworkConditions(
    const std::string& client_id,
    std::unique_ptr<NetworkConditions> conditions) {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);

  auto it = interceptors_.find(client_id);
  if (it == interceptors_.end()) {
    if (!conditions)
      return;
    std::unique_ptr<ThrottlingNetworkInterceptor> new_interceptor(
        new ThrottlingNetworkInterceptor());
    new_interceptor->UpdateConditions(std::move(conditions));
    interceptors_[client_id] = std::move(new_interceptor);
  } else {
    if (!conditions) {
      std::unique_ptr<NetworkConditions> online_conditions(
          new NetworkConditions());
      it->second->UpdateConditions(std::move(online_conditions));
      interceptors_.erase(client_id);
      if (interceptors_.empty()) {
        delete this;
        instance_ = nullptr;
      }
    } else {
      it->second->UpdateConditions(std::move(conditions));
    }
  }
}

}  // namespace network
