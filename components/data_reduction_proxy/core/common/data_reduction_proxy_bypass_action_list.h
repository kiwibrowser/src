// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// This is the list of data reduction proxy bypass actions and their values.
// These actions are specified in the Chrome-Proxy header. For the enum values,
// include the file
// "components/data_reduction_proxy/core/common/data_reduction_proxy_headers.h".
//
// Here we define the values using a macro BYPASS_ACTION_TYPE, so it can be
// expanded differently in some places (for example, to automatically
// map a bypass type value to its symbolic name). As such, new values must be
// appended and cannot be inserted in the middle as there are instances where
// we will load data between different builds.

// No action type specified.
BYPASS_ACTION_TYPE(NONE, 0)

// Attempt to retry the current request while bypassing all Data Reduction
// Proxies; it does not cause other requests to be bypassed.
BYPASS_ACTION_TYPE(BLOCK_ONCE, 1)

// Bypass all Data Reduction Proxies for a specified period of time.
BYPASS_ACTION_TYPE(BLOCK, 2)

// Bypass the current Data Reduction Proxy for a specified period of time.
BYPASS_ACTION_TYPE(BYPASS, 3)

// This must always be last.
BYPASS_ACTION_TYPE(MAX, 4)
