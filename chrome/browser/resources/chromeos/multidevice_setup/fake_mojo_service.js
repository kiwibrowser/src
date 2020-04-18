// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// TODO(jordynass): Delete these testing utilities when code is productionized.

cr.define('multidevice_setup', function() {
  /** @enum {number} */
  const SetBetterTogetherHostResponseCode = {
    SUCCESS: 1,
    ERROR_OFFLINE: 2,
    ERROR_NETWORK_REQUEST_FAILED: 3,
  };

  const FakeMojoService = {
    responseCode: SetBetterTogetherHostResponseCode.SUCCESS,

    deviceCount: 2,

    getEligibleBetterTogetherHosts: function() {
      const deviceNames = ['Pixel', 'Pixel XL', 'Nexus 5', 'Nexus 6P'];
      let devices = [];
      for (let i = 0; i < this.deviceCount; i++) {
        const deviceName = deviceNames[i % 4];
        devices.push({name: deviceName, publicKey: deviceName + '--' + i});
      }
      return new Promise(function(resolve, reject) {
        resolve({devices: devices});
      });
    },

    setBetterTogetherHost: function(publicKey) {
      if (this.responseCode == SetBetterTogetherHostResponseCode.SUCCESS) {
        console.log('Calling SetBetterTogetherHost on device ' + publicKey);
      } else {
        console.warn('Unable to set host. Response code: ' + this.responseCode);
      }
      return new Promise((resolve, reject) => {
        resolve({responseCode: this.responseCode});
      });
    },

    /**
     * Utility function for resetting fake service to simulate working service
     * for user with 2 potential hosts.
     */
    reset: function() {
      this.deviceCount = 2;
      this.responseCode = SetBetterTogetherHostResponseCode.SUCCESS;
    },
  };

  return {
    SetBetterTogetherHostResponseCode: SetBetterTogetherHostResponseCode,
    FakeMojoService: FakeMojoService,
  };
});
