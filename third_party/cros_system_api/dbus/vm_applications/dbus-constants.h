// Copyright 2018 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SYSTEM_API_DBUS_VM_APPLICATIONS_DBUS_CONSTANTS_H_
#define SYSTEM_API_DBUS_VM_APPLICATIONS_DBUS_CONSTANTS_H_

namespace vm_tools {
namespace apps {

const char kVmApplicationsServiceName[] = "org.chromium.VmApplicationsService";
const char kVmApplicationsServicePath[] = "/org/chromium/VmApplicationsService";
const char kVmApplicationsServiceInterface[] = "org.chromium.VmApplicationsService";

const char kVmApplicationsServiceUpdateApplicationListMethod[] = "UpdateApplicationList";

}  // namespace apps
}  // namespace vm_tools

#endif  // SYSTEM_API_DBUS_VM_APPLICATIONS_DBUS_CONSTANTS_H_
