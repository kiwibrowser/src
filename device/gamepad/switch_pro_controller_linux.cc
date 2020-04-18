// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "device/gamepad/switch_pro_controller_linux.h"

#include "base/posix/eintr_wrapper.h"
#include "device/gamepad/gamepad_standard_mappings.h"

namespace device {

SwitchProControllerLinux::SwitchProControllerLinux(int fd) : fd_(fd) {}

SwitchProControllerLinux::~SwitchProControllerLinux() = default;

size_t SwitchProControllerLinux::ReadInputReport(void* report) {
  DCHECK(report);
  ssize_t bytes_read = HANDLE_EINTR(read(fd_, report, kReportSize));
  return bytes_read < 0 ? 0 : static_cast<size_t>(bytes_read);
}

size_t SwitchProControllerLinux::WriteOutputReport(void* report,
                                                   size_t report_length) {
  DCHECK(report);
  DCHECK_GE(report_length, 1U);
  ssize_t bytes_written = HANDLE_EINTR(write(fd_, report, report_length));
  return bytes_written < 0 ? 0 : static_cast<size_t>(bytes_written);
}

}  // namespace device
