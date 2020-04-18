// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/subresource_filter/core/common/activation_state.h"

#include <ostream>
#include <sstream>
#include <string>

#include "base/trace_event/trace_event_argument.h"

namespace subresource_filter {

std::unique_ptr<base::trace_event::TracedValue> ActivationState::ToTracedValue()
    const {
  auto value = std::make_unique<base::trace_event::TracedValue>();
  std::ostringstream level;
  level << activation_level;
  value->SetString("activation_level", level.str());
  value->SetBoolean("filtering_disabled_for_document",
                    filtering_disabled_for_document);
  value->SetBoolean("generic_blocking_rules_disabled",
                    generic_blocking_rules_disabled);
  value->SetBoolean("measure_performance", measure_performance);
  value->SetBoolean("enable_logging", enable_logging);
  return value;
}

}  // namespace subresource_filter
