// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

chrome.easyUnlockPrivate.onStartAutoPairing.addListener(function() {
  chrome.easyUnlockPrivate.setPermitAccess({
    permitId: 'fake_permit_id',
    id: 'fake_id',
    type: 'access',
    data: 'ZmFrZV9kYXRh'  // 'fake_data'
  });
  chrome.easyUnlockPrivate.setRemoteDevices([
    {
      bluetoothAddress: '11:11:11:11:11:11',
      name: 'fake_remote_device',
      permitRecord: {
        permitId: 'fake_permit_id',
        id: 'fake_id',
        type: 'license',
        data: 'ZmFrZV9kYXRh'  // 'fake_data'
      },
      psk: 'ZmFrZV9wc2s='  // 'fake_psk'
    },
  ]);
  chrome.easyUnlockPrivate.setAutoPairingResult({success: true});
});
