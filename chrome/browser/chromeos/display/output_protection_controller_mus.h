// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_CHROMEOS_DISPLAY_OUTPUT_PROTECTION_CONTROLLER_MUS_H_
#define CHROME_BROWSER_CHROMEOS_DISPLAY_OUTPUT_PROTECTION_CONTROLLER_MUS_H_

#include "base/macros.h"
#include "base/threading/thread_checker.h"
#include "chrome/browser/chromeos/display/output_protection_delegate.h"
#include "services/ui/public/interfaces/display/output_protection.mojom.h"

namespace chromeos {

// Display content protection controller for running with Mus server.
class OutputProtectionControllerMus
    : public OutputProtectionDelegate::Controller {
 public:
  OutputProtectionControllerMus();
  ~OutputProtectionControllerMus() override;

  // OutputProtectionDelegate::Controller:
  void QueryStatus(
      int64_t display_id,
      const OutputProtectionDelegate::QueryStatusCallback& callback) override;
  void SetProtection(
      int64_t display_id,
      uint32_t desired_method_mask,
      const OutputProtectionDelegate::SetProtectionCallback& callback) override;

 private:
  display::mojom::OutputProtectionPtr output_protection_;
  base::ThreadChecker thread_checker_;

  DISALLOW_COPY_AND_ASSIGN(OutputProtectionControllerMus);
};

}  // namespace chromeos

#endif  // CHROME_BROWSER_CHROMEOS_DISPLAY_OUTPUT_PROTECTION_CONTROLLER_MUS_H_
