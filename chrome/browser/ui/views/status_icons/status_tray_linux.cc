// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/views/status_icons/status_tray_linux.h"

#include "build/build_config.h"

#if !defined(OS_CHROMEOS)
#include "chrome/browser/ui/views/status_icons/status_icon_linux_wrapper.h"
#include "ui/views/linux_ui/linux_ui.h"

StatusTrayLinux::StatusTrayLinux() {
}

StatusTrayLinux::~StatusTrayLinux() {
}

std::unique_ptr<StatusIcon> StatusTrayLinux::CreatePlatformStatusIcon(
    StatusIconType type,
    const gfx::ImageSkia& image,
    const base::string16& tool_tip) {
  return StatusIconLinuxWrapper::CreateWrappedStatusIcon(image, tool_tip);
}

StatusTray* StatusTray::Create() {
  const views::LinuxUI* linux_ui = views::LinuxUI::instance();

  // Only create a status tray if we can actually create status icons.
  if (linux_ui && linux_ui->IsStatusIconSupported())
    return new StatusTrayLinux();
  return nullptr;
}
#else  // defined(OS_CHROMEOS)
StatusTray* StatusTray::Create() {
  return nullptr;
}
#endif
