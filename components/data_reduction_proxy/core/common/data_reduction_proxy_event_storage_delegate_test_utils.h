// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_DATA_REDUCTION_PROXY_CORE_COMMON_DATA_REDUCTION_PROXY_EVENT_STORE_TEST_UTILS_H_
#define COMPONENTS_DATA_REDUCTION_PROXY_CORE_COMMON_DATA_REDUCTION_PROXY_EVENT_STORE_TEST_UTILS_H_

#include <stdint.h>

#include <memory>

#include "components/data_reduction_proxy/core/common/data_reduction_proxy_event_storage_delegate.h"

namespace base {
class Value;
}

namespace data_reduction_proxy {

class TestDataReductionProxyEventStorageDelegate
    : public DataReductionProxyEventStorageDelegate {
 public:
  TestDataReductionProxyEventStorageDelegate();

  virtual ~TestDataReductionProxyEventStorageDelegate();

  // Sets |delegate_| at a later point in time.
  void SetStorageDelegate(DataReductionProxyEventStorageDelegate* delegate);

  // Overrides of DataReductionProxyEventStorageDelegate:
  void AddEvent(std::unique_ptr<base::Value> event) override;
  void AddEnabledEvent(std::unique_ptr<base::Value> event,
                       bool enabled) override;
  void AddAndSetLastBypassEvent(std::unique_ptr<base::Value> event,
                                int64_t expiration_ticks) override;
  void AddEventAndSecureProxyCheckState(std::unique_ptr<base::Value> event,
                                        SecureProxyCheckState state) override;

 private:
  // If not null, |this| will send DataReductionProxyEventStorageDelegate
  // calls to |delegate_|.
  DataReductionProxyEventStorageDelegate* delegate_;
};

}  // namespace data_reduction_proxy

#endif  // COMPONENTS_DATA_REDUCTION_PROXY_CORE_COMMON_DATA_REDUCTION_PROXY_EVENT_STORE_TEST_UTILS_H_
