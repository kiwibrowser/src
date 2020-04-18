// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_COMMON_RESOURCE_TIMING_INFO_H_
#define CONTENT_COMMON_RESOURCE_TIMING_INFO_H_

#include <stdint.h>

#include <string>
#include <vector>

#include "base/optional.h"
#include "base/time/time.h"

namespace content {

// TODO(dcheng): Migrate this struct over to Mojo so it doesn't need to be
// duplicated in //content and //third_party/WebKit.
// See blink::WebServerTimingInfo for more information about this struct's
// fields.
struct ServerTimingInfo {
  ServerTimingInfo();
  ServerTimingInfo(const ServerTimingInfo&);
  ~ServerTimingInfo();

  std::string name;
  double duration = 0.0;
  std::string description;
};

// TODO(dcheng): Migrate this struct over to Mojo so it doesn't need to be
// duplicated in //content and //third_party/WebKit.
// See blink::WebURLLoadTiming for more information about this struct's fields.
struct ResourceLoadTiming {
  ResourceLoadTiming();
  ResourceLoadTiming(const ResourceLoadTiming&);
  ~ResourceLoadTiming();

  base::TimeTicks request_time;
  base::TimeTicks proxy_start;
  base::TimeTicks proxy_end;
  base::TimeTicks dns_start;
  base::TimeTicks dns_end;
  base::TimeTicks connect_start;
  base::TimeTicks connect_end;
  base::TimeTicks worker_start;
  base::TimeTicks worker_ready;
  base::TimeTicks send_start;
  base::TimeTicks send_end;
  base::TimeTicks receive_headers_end;
  base::TimeTicks ssl_start;
  base::TimeTicks ssl_end;
  base::TimeTicks push_start;
  base::TimeTicks push_end;
};

// TODO(dcheng): Migrate this struct over to Mojo so it doesn't need to be
// duplicated in //content and //third_party/WebKit.
// See blink::WebResourceTimingInfo for more information about this struct's
// fields.
struct ResourceTimingInfo {
  ResourceTimingInfo();
  ResourceTimingInfo(const ResourceTimingInfo&);
  ~ResourceTimingInfo();

  std::string name;
  base::TimeTicks start_time;
  std::string initiator_type;
  std::string alpn_negotiated_protocol;
  std::string connection_info;
  base::Optional<ResourceLoadTiming> timing;
  base::TimeTicks last_redirect_end_time;
  base::TimeTicks finish_time;
  uint64_t transfer_size = 0;
  uint64_t encoded_body_size = 0;
  uint64_t decoded_body_size = 0;
  bool did_reuse_connection = false;
  bool allow_timing_details = false;
  bool allow_redirect_details = false;
  bool allow_negative_values = false;
  std::vector<ServerTimingInfo> server_timing;
};

}  // namespace content

#endif  // CONTENT_COMMON_RESOURCE_TIMING_INFO_H_
