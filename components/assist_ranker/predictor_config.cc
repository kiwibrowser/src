// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/assist_ranker/predictor_config.h"

namespace assist_ranker {

const base::flat_set<std::string>* GetEmptyWhitelist() {
  static auto* whitelist = new base::flat_set<std::string>();
  return whitelist;
}

}  // namespace assist_ranker
