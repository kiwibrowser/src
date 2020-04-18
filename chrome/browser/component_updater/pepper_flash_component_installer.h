// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_COMPONENT_UPDATER_PEPPER_FLASH_COMPONENT_INSTALLER_H_
#define CHROME_BROWSER_COMPONENT_UPDATER_PEPPER_FLASH_COMPONENT_INSTALLER_H_

namespace component_updater {

class ComponentUpdateService;

// Our job is to 1) find what Pepper flash is installed (if any) and 2) register
// with the component updater to download the latest version when available.
// The first part is IO intensive so we do it asynchronously in the file thread.
void RegisterPepperFlashComponent(ComponentUpdateService* cus);

}  // namespace component_updater

#endif  // CHROME_BROWSER_COMPONENT_UPDATER_PEPPER_FLASH_COMPONENT_INSTALLER_H_
