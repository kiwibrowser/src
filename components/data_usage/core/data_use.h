// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_DATA_USAGE_CORE_DATA_USE_H_
#define COMPONENTS_DATA_USAGE_CORE_DATA_USE_H_

#include <stdint.h>

#include <string>
#include <utility>

#include "base/time/time.h"
#include "components/sessions/core/session_id.h"
#include "net/base/network_change_notifier.h"
#include "url/gurl.h"

namespace data_usage {

struct DataUse {
  // Wraps the content::GlobalRequestID.
  typedef std::pair<int, int> MainFrameGlobalRequestID;

  static const MainFrameGlobalRequestID kInvalidMainFrameGlobalRequestID;

  DataUse(const GURL& url,
          const base::TimeTicks& request_start,
          const GURL& site_for_cookies,
          SessionID tab_id,
          net::NetworkChangeNotifier::ConnectionType connection_type,
          const std::string& mcc_mnc,
          int64_t tx_bytes,
          int64_t rx_bytes);

  DataUse(const DataUse& other);

  ~DataUse();

  bool operator==(const DataUse& other) const;

  // Returns true if |this| and |other| are identical except for byte counts.
  bool CanCombineWith(const DataUse& other) const;

  GURL url;
  // The TimeTicks when the request that is associated with these bytes was
  // started.
  base::TimeTicks request_start;
  GURL site_for_cookies;

  // Tab id where the data use happens. When PlzNavigate is enabled tab id could
  // be invalid(-1) for the mainframe request since tab cannot be retrieved yet.
  // Could be invalid(-1) if the data use does not belong to a tab, for example
  // chrome-services traffic.
  SessionID tab_id;

  // content::GlobalRequestID of the mainframe request. This is populated only
  // when tab id cannot be retrieved of a mainframe request, and used to
  // attribute to its tab ID later.
  MainFrameGlobalRequestID main_frame_global_request_id;

  net::NetworkChangeNotifier::ConnectionType connection_type;
  // MCC+MNC (mobile country code + mobile network code) of the provider of the
  // SIM when the network traffic was exchanged. Set to empty string if SIM is
  // not present. |mcc_mnc| is set even if data was not exchanged on the
  // cellular network. For dual SIM phones, this is set to the MCC/MNC of the
  // SIM in slot 0.
  std::string mcc_mnc;

  int64_t tx_bytes;
  int64_t rx_bytes;
};

}  // namespace data_usage

#endif  // COMPONENTS_DATA_USAGE_CORE_DATA_USE_H_
