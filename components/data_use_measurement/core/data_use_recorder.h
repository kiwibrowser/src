// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_DATA_USE_MEASUREMENT_CORE_DATA_USE_RECORDER_H_
#define COMPONENTS_DATA_USE_MEASUREMENT_CORE_DATA_USE_RECORDER_H_

#include <stdint.h>

#include <map>
#include <vector>

#include "base/macros.h"
#include "base/supports_user_data.h"
#include "base/time/time.h"
#include "components/data_use_measurement/core/data_use.h"
#include "net/base/net_export.h"

namespace net {
class URLRequest;
}

namespace data_use_measurement {

// Tracks all network data used by a single high level entity. An entity
// can make multiple URLRequests, so there is a one:many relationship between
// DataUseRecorders and URLRequests. Data used by each URLRequest will be
// tracked by exactly one DataUseRecorder.
class DataUseRecorder {
 public:
  // Stores network data used by a URLRequest.
  struct URLRequestDataUse {
    URLRequestDataUse()
        : bytes_received(0),
          bytes_sent(0),
          started_time(base::TimeTicks::Now()) {}

    int64_t bytes_received;
    int64_t bytes_sent;

    // Time the request started and associated with the DataUseRecorder.
    base::TimeTicks started_time;

   private:
    DISALLOW_COPY_AND_ASSIGN(URLRequestDataUse);
  };

  explicit DataUseRecorder(DataUse::TrafficType traffic_type);
  virtual ~DataUseRecorder();

  // Returns the actual data used by the entity being tracked.
  DataUse& data_use() { return data_use_; }

  const net::URLRequest* main_url_request() const { return main_url_request_; }

  void set_main_url_request(const net::URLRequest* request) {
    main_url_request_ = request;
  }

  bool is_visible() const { return is_visible_; }

  void set_is_visible(bool is_visible) { is_visible_ = is_visible; }

  uint64_t page_transition() const { return page_transition_; }

  void set_page_transition(uint64_t page_transition) {
    page_transition_ = page_transition;
  }

  // Returns whether data use is complete and no additional data can be used
  // by the entity tracked by this recorder. For example,
  bool IsDataUseComplete();

  // Populate the pending requests to |requests|.
  // Reference to the map is not returned since other member functions that
  // modify/erase could be called while iterating.
  void GetPendingURLRequests(std::vector<net::URLRequest*>* requests) const;

  // Adds |request| to the list of pending URLRequests that ascribe data use to
  // this recorder.
  void AddPendingURLRequest(net::URLRequest* request);

  // Moves pending |request| from |this| recorder to |other| recorder, and
  // updates the data use for the recorders.
  void MovePendingURLRequestTo(DataUseRecorder* other,
                               net::URLRequest* request);

  base::TimeTicks GetPendingURLRequestStartTime(net::URLRequest* request);

  // Clears the list of pending URLRequests that ascribe data use to this
  // recorder.
  void RemoveAllPendingURLRequests();

  // Network Delegate methods:
  void OnBeforeUrlRequest(net::URLRequest* request);
  void OnUrlRequestDestroyed(net::URLRequest* request);
  void OnNetworkBytesSent(net::URLRequest* request, int64_t bytes_sent);
  void OnNetworkBytesReceived(net::URLRequest* request, int64_t bytes_received);

 private:
  // Updates the network data use for the url request.
  void UpdateNetworkByteCounts(net::URLRequest* request,
                               int64_t bytes_received,
                               int64_t bytes_sent);

  // Pending URLRequests whose data is being tracked by this DataUseRecorder.
  std::map<net::URLRequest*, URLRequestDataUse> pending_url_requests_;

  // The main frame URLRequest for page loads. Null if this is not tracking a
  // page load.
  const net::URLRequest* main_url_request_;

  // The network data use measured by this DataUseRecorder.
  DataUse data_use_;

  // Whether the entity that owns this data use is currently visible.
  bool is_visible_;

  // ui::PageTransition casted as a uint64_t.
  uint64_t page_transition_;

  DISALLOW_COPY_AND_ASSIGN(DataUseRecorder);
};

}  // namespace data_use_measurement

#endif  // COMPONENTS_DATA_USE_MEASUREMENT_CORE_DATA_USE_RECORDER_H_
