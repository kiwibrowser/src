// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/previews/core/test_previews_decider.h"

namespace previews {

TestPreviewsDecider::TestPreviewsDecider(bool allow_previews)
    : allow_previews_(allow_previews) {}

TestPreviewsDecider::~TestPreviewsDecider() {}

bool TestPreviewsDecider::ShouldAllowPreviewAtECT(
    const net::URLRequest& request,
    previews::PreviewsType type,
    net::EffectiveConnectionType effective_connection_type_threshold,
    const std::vector<std::string>& host_blacklist_from_server) const {
  return allow_previews_;
}

bool TestPreviewsDecider::ShouldAllowPreview(
    const net::URLRequest& request,
    previews::PreviewsType type) const {
  return allow_previews_;
}

bool TestPreviewsDecider::IsURLAllowedForPreview(const net::URLRequest& request,
                                                 PreviewsType type) const {
  return allow_previews_;
}

}  // namespace previews
