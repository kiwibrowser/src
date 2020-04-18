// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/data_reduction_proxy/core/common/data_reduction_proxy_event_storage_delegate_test_utils.h"

#include <utility>

#include "base/values.h"
#include "components/data_reduction_proxy/core/common/data_reduction_proxy_event_storage_delegate.h"

namespace data_reduction_proxy {

TestDataReductionProxyEventStorageDelegate::
    TestDataReductionProxyEventStorageDelegate()
    : delegate_(nullptr) {
}

TestDataReductionProxyEventStorageDelegate::
    ~TestDataReductionProxyEventStorageDelegate() {
}

void TestDataReductionProxyEventStorageDelegate::SetStorageDelegate(
    DataReductionProxyEventStorageDelegate* delegate) {
  delegate_ = delegate;
}

void TestDataReductionProxyEventStorageDelegate::AddEvent(
    std::unique_ptr<base::Value> event) {
  if (delegate_)
    delegate_->AddEvent(std::move(event));
}

void TestDataReductionProxyEventStorageDelegate::AddEnabledEvent(
    std::unique_ptr<base::Value> event,
    bool enabled) {
  if (delegate_)
    delegate_->AddEnabledEvent(std::move(event), enabled);
}

void TestDataReductionProxyEventStorageDelegate::AddAndSetLastBypassEvent(
    std::unique_ptr<base::Value> event,
    int64_t expiration_ticks) {
  if (delegate_)
    delegate_->AddAndSetLastBypassEvent(std::move(event), expiration_ticks);
}

void TestDataReductionProxyEventStorageDelegate::
    AddEventAndSecureProxyCheckState(std::unique_ptr<base::Value> event,
                                     SecureProxyCheckState state) {
  if (delegate_)
    delegate_->AddEventAndSecureProxyCheckState(std::move(event), state);
}

}  // namespace data_reduction_proxy
