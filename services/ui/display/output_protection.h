// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_UI_DISPLAY_OUTPUT_PROTECTION_H_
#define SERVICES_UI_DISPLAY_OUTPUT_PROTECTION_H_

#include <stdint.h>

#include "base/macros.h"
#include "services/ui/public/interfaces/display/output_protection.mojom.h"

namespace display {

class DisplayConfigurator;

// OutputProtection provides the necessary functionality to configure output
// protection.
class OutputProtection : public mojom::OutputProtection {
 public:
  explicit OutputProtection(DisplayConfigurator* display_configurator);
  ~OutputProtection() override;

  // mojom::OutputProtection:
  void QueryContentProtectionStatus(
      int64_t display_id,
      const QueryContentProtectionStatusCallback& callback) override;
  void SetContentProtection(
      int64_t display_id,
      uint32_t desired_method_mask,
      const SetContentProtectionCallback& callback) override;

 private:
  DisplayConfigurator* const display_configurator_;
  const uint64_t client_id_;

  DISALLOW_COPY_AND_ASSIGN(OutputProtection);
};

}  // namespace display

#endif  // SERVICES_UI_DISPLAY_OUTPUT_PROTECTION_H_
