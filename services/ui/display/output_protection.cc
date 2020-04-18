// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/ui/display/output_protection.h"

#include "ui/display/manager/display_configurator.h"

namespace display {

OutputProtection::OutputProtection(DisplayConfigurator* display_configurator)
    : display_configurator_(display_configurator),
      client_id_(display_configurator_->RegisterContentProtectionClient()) {}

OutputProtection::~OutputProtection() {
  display_configurator_->UnregisterContentProtectionClient(client_id_);
}

void OutputProtection::QueryContentProtectionStatus(
    int64_t display_id,
    const QueryContentProtectionStatusCallback& callback) {
  display_configurator_->QueryContentProtectionStatus(client_id_, display_id,
                                                      callback);
}

void OutputProtection::SetContentProtection(
    int64_t display_id,
    uint32_t desired_method_mask,
    const SetContentProtectionCallback& callback) {
  display_configurator_->SetContentProtection(client_id_, display_id,
                                              desired_method_mask, callback);
}

}  // namespace display
