// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// This is the list of data reduction proxy bypass event types and their values.
// For the enum values, include the file
// "components/data_reduction_proxy/core/common/data_reduction_proxy_headers.h".
//
// Here we define the values using a macro BYPASS_EVENT_TYPE, so it can be
// expanded differently in some places (for example, to automatically
// map a bypass type value to its symbolic name). As such, new values must be
// appended and cannot be inserted in the middle as there are instances where
// we will load data between different builds.

// Bypass due to explicit instruction for the current request.
BYPASS_EVENT_TYPE(CURRENT, 0)

// Bypass the proxy for less than one minute.
BYPASS_EVENT_TYPE(SHORT, 1)

// Bypass the proxy for one to five minutes.
BYPASS_EVENT_TYPE(MEDIUM, 2)

// Bypass the proxy for more than five minutes.
BYPASS_EVENT_TYPE(LONG, 3)

// Bypass due to a 4xx missing via header.
BYPASS_EVENT_TYPE(MISSING_VIA_HEADER_4XX, 4)

// Bypass due to other missing via header, excluding 4xx errors.
BYPASS_EVENT_TYPE(MISSING_VIA_HEADER_OTHER, 5)

// Bypass due to 407 response from proxy without a challenge.
BYPASS_EVENT_TYPE(MALFORMED_407, 6)

// Bypass due to a 500 internal server error
BYPASS_EVENT_TYPE(STATUS_500_HTTP_INTERNAL_SERVER_ERROR, 7)

// Bypass because the request URI was too long.
BYPASS_EVENT_TYPE(STATUS_502_HTTP_BAD_GATEWAY, 8)

// Bypass due to a 503 response.
BYPASS_EVENT_TYPE(STATUS_503_HTTP_SERVICE_UNAVAILABLE, 9)

// Bypass due to any network error.
BYPASS_EVENT_TYPE(NETWORK_ERROR, 10)

// Bypass due to URL redirect cycle.
BYPASS_EVENT_TYPE(URL_REDIRECT_CYCLE, 11)

// This must always be last.
BYPASS_EVENT_TYPE(MAX, 12)
