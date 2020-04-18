// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_DATA_REDUCTION_PROXY_CORE_COMMON_DATA_REDUCTION_PROXY_EVENT_STORE_H_
#define COMPONENTS_DATA_REDUCTION_PROXY_CORE_COMMON_DATA_REDUCTION_PROXY_EVENT_STORE_H_

#include <stdint.h>

#include <memory>
#include <string>
#include <vector>

#include "base/macros.h"
#include "base/threading/thread_checker.h"
#include "components/data_reduction_proxy/core/common/data_reduction_proxy_event_storage_delegate.h"
#include "components/data_reduction_proxy/core/common/data_reduction_proxy_headers.h"

namespace base {
class DictionaryValue;
class Value;
}

namespace data_reduction_proxy {

class DataReductionProxyEventStore
    : public DataReductionProxyEventStorageDelegate {
 public:
  // Adds data reduction proxy specific constants to the net_internals
  // constants dictionary.
  static void AddConstants(base::DictionaryValue* constants_dict);

  // Constructs a DataReductionProxyEventStore object.
  DataReductionProxyEventStore();

  virtual ~DataReductionProxyEventStore();

  // Creates a Value summary of Data Reduction Proxy related information:
  // - Whether the proxy is enabled,
  // - The proxy configuration,
  // - The state of the last secure proxy check response,
  // - A stream of the last Data Reduction Proxy related events.
  std::unique_ptr<base::DictionaryValue> GetSummaryValue() const;

  // Adds DATA_REDUCTION_PROXY event with no parameters to the event store.
  void AddEvent(std::unique_ptr<base::Value> event) override;

  // Override of DataReductionProxyEventStorageDelegate.
  // Adds |entry| to the event store and sets |current_configuration_|.
  void AddEnabledEvent(std::unique_ptr<base::Value> entry,
                       bool enabled) override;

  // Override of DataReductionProxyEventStorageDelegate.
  // Adds |entry| to the event store and sets |secure_proxy_check_state_|.
  void AddEventAndSecureProxyCheckState(std::unique_ptr<base::Value> entry,
                                        SecureProxyCheckState state) override;

  // Override of DataReductionProxyEventStorageDelegate.
  // Adds |entry| to the event store and sets |last_bypass_event_| and
  // |expiration_ticks_|.
  void AddAndSetLastBypassEvent(std::unique_ptr<base::Value> entry,
                                int64_t expiration_ticks) override;

  // Returns the list of proxy servers for HTTP origins.
  std::string GetHttpProxyList() const;

  // Returns a sanitized version of the last seen bypass event.
  std::string SanitizedLastBypassEvent() const;

 private:
  friend class DataReductionProxyEventStoreTest;

  // A vector of data reduction proxy related events. It is used as a circular
  // buffer to prevent unbounded memory utilization.
  std::vector<std::unique_ptr<base::Value>> stored_events_;
  // The index of the oldest event in |stored_events_|.
  size_t oldest_event_index_;

  // Whether the data reduction proxy is enabled or not.
  bool enabled_;
  // The current data reduction proxy configuration.
  std::unique_ptr<base::Value> current_configuration_;
  // The state based on the last secure proxy check.
  SecureProxyCheckState secure_proxy_check_state_;
  // The last seen data reduction proxy bypass event.
  std::unique_ptr<base::Value> last_bypass_event_;
  // The expiration time of the |last_bypass_event_|.
  int64_t expiration_ticks_;

  // Enforce usage on the UI thread.
  base::ThreadChecker thread_checker_;

  DISALLOW_COPY_AND_ASSIGN(DataReductionProxyEventStore);
};

}  // namespace data_reduction_proxy

#endif  // COMPONENTS_DATA_REDUCTION_PROXY_CORE_COMMON_DATA_REDUCTION_PROXY_EVENT_STORE_H_
