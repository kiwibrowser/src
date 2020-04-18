// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/public/renderer/document_state.h"

#include "content/public/renderer/navigation_state.h"

namespace content {

DocumentState::DocumentState()
    : was_fetched_via_spdy_(false),
      was_alpn_negotiated_(false),
      was_alternate_protocol_available_(false),
      connection_info_(net::HttpResponseInfo::CONNECTION_INFO_UNKNOWN),
      was_load_data_with_base_url_request_(false),
      can_load_local_resources_(false) {}

DocumentState::~DocumentState() {}

void DocumentState::set_navigation_state(NavigationState* navigation_state) {
  navigation_state_.reset(navigation_state);
}

}  // namespace content
