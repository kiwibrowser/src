// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_CHROMEOS_DISPLAY_OUTPUT_PROTECTION_CONTROLLER_ASH_H_
#define CHROME_BROWSER_CHROMEOS_DISPLAY_OUTPUT_PROTECTION_CONTROLLER_ASH_H_

#include "base/macros.h"
#include "base/threading/thread_checker.h"
#include "chrome/browser/chromeos/display/output_protection_delegate.h"
#include "ui/display/manager/display_configurator.h"

namespace chromeos {

// Display content protection controller for running with ash.
class OutputProtectionControllerAsh
    : public OutputProtectionDelegate::Controller {
 public:
  OutputProtectionControllerAsh();
  ~OutputProtectionControllerAsh() override;

  // OutputProtectionDelegate::Controller:
  void QueryStatus(
      int64_t display_id,
      const OutputProtectionDelegate::QueryStatusCallback& callback) override;
  void SetProtection(
      int64_t display_id,
      uint32_t desired_method_mask,
      const OutputProtectionDelegate::SetProtectionCallback& callback) override;

 private:
  const uint64_t client_id_;
  base::ThreadChecker thread_checker_;

  DISALLOW_COPY_AND_ASSIGN(OutputProtectionControllerAsh);
};

}  // namespace chromeos

#endif  // CHROME_BROWSER_CHROMEOS_DISPLAY_OUTPUT_PROTECTION_CONTROLLER_ASH_H_
