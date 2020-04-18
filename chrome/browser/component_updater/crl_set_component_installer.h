// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_COMPONENT_UPDATER_CRL_SET_COMPONENT_INSTALLER_H_
#define CHROME_BROWSER_COMPONENT_UPDATER_CRL_SET_COMPONENT_INSTALLER_H_

namespace base {
class FilePath;
}  // namespace base

namespace component_updater {

class ComponentUpdateService;

void RegisterCRLSetComponent(ComponentUpdateService* cus,
                             const base::FilePath& user_data_dir);

}  // namespace component_updater

#endif  // CHROME_BROWSER_COMPONENT_UPDATER_CRL_SET_COMPONENT_INSTALLER_H_
