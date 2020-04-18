// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_CHROMEOS_POLICY_DEVICE_POLICY_DECODER_CHROMEOS_H_
#define CHROME_BROWSER_CHROMEOS_POLICY_DEVICE_POLICY_DECODER_CHROMEOS_H_

namespace enterprise_management {
class ChromeDeviceSettingsProto;
}

namespace policy {

class PolicyMap;

// Decodes device policy in ChromeDeviceSettingsProto representation into the a
// PolicyMap.
void DecodeDevicePolicy(
    const enterprise_management::ChromeDeviceSettingsProto& policy,
    PolicyMap* policies);

}  // namespace policy

#endif  // CHROME_BROWSER_CHROMEOS_POLICY_DEVICE_POLICY_DECODER_CHROMEOS_H_
