// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * Javascript for device_broker, served from chrome://bluetooth-internals/.
 * Provides a single source to access DevicePtrs. DevicePtrs are cached for
 * for repeated use. Multiple connection requests will result in the same
 * DevicePtr being shared among all requesters.
 */

// Expose for testing.
/** @type {!Map<string, !bluetooth.mojom.DevicePtr|!Promise} */
var connectedDevices = null;

cr.define('device_broker', function() {
  connectedDevices = new Map();

  /**
   * Creates a GATT connection to the device with |address|. If a connection to
   * the device already exists, the promise is resolved with the existing
   * DevicePtr. If a connection is in progress, the promise resolves when
   * the existing connection request promise is fulfilled.
   * @param {string} address
   * @return {!Promise<!bluetooth.mojom.DevicePtr>}
   */
  function connectToDevice(address) {
    var deviceOrPromise = connectedDevices.get(address) || null;
    if (deviceOrPromise !== null)
      return Promise.resolve(deviceOrPromise);

    var promise = adapter_broker.getAdapterBroker()
                      .then(function(adapterBroker) {
                        return adapterBroker.connectToDevice(address);
                      })
                      .then(function(device) {
                        connectedDevices.set(address, device);

                        device.ptr.setConnectionErrorHandler(function() {
                          connectedDevices.delete(address);
                        });

                        return device;
                      })
                      .catch(function(error) {
                        connectedDevices.delete(address);
                        throw error;
                      });

    connectedDevices.set(address, promise);
    return promise;
  }

  return {connectToDevice: connectToDevice};
});
