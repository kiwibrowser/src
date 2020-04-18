// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_PREVIEWS_CORE_TEST_PREVIEWS_DECIDER_H_
#define COMPONENTS_PREVIEWS_CORE_TEST_PREVIEWS_DECIDER_H_

#include "components/previews/core/previews_decider.h"

namespace previews {

// Simple test implementation of PreviewsDecider interface.
class TestPreviewsDecider : public previews::PreviewsDecider {
 public:
  TestPreviewsDecider(bool allow_previews);
  ~TestPreviewsDecider() override;

  // previews::PreviewsDecider:
  bool ShouldAllowPreviewAtECT(
      const net::URLRequest& request,
      previews::PreviewsType type,
      net::EffectiveConnectionType effective_connection_type_threshold,
      const std::vector<std::string>& host_blacklist_from_server)
      const override;
  bool ShouldAllowPreview(const net::URLRequest& request,
                          previews::PreviewsType type) const override;
  bool IsURLAllowedForPreview(const net::URLRequest& request,
                              PreviewsType type) const override;

 private:
  bool allow_previews_;
};

}  // namespace previews

#endif  // COMPONENTS_PREVIEWS_CORE_TEST_PREVIEWS_DECIDER_H_
