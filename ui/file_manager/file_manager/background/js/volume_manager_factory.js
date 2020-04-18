// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

var volumeManagerFactory = (function() {
  /**
   * The singleton instance of VolumeManager. Initialized by the first
   * invocation of getInstance().
   * @type {VolumeManager}
   */
  var instance = null;

  /**
   * @type {Promise}
   */
  var instancePromise = null;

  /**
   * Returns the VolumeManager instance asynchronously. If it has not been
   * created or is under initialization, it will waits for the finish of the
   * initialization.
   * @param {function(VolumeManager)=} opt_callback Called with the
   *     VolumeManager instance. TODO(hirono): Remove the callback and use
   *     Promise instead.
   * @return {Promise} Promise to be fulfilled with the volume manager.
   */
  function getInstance(opt_callback) {
    if (!instancePromise) {
      instance = new VolumeManagerImpl();
      instancePromise = new Promise(function(fulfill) {
        instance.initialize_(function() {
          return fulfill(instance);
        });
      });
    }
    if (opt_callback)
      instancePromise.then(opt_callback);
    return instancePromise;
  };

  /**
   * Returns instance of VolumeManager for debug purpose.
   * This method returns VolumeManager.instance which may not be initialized.
   *
   * @return {VolumeManager} Volume manager.
   */
  function getInstanceForDebug() {
    return instance;
  };

  /**
   * Revokes the singleton instance for testing.
   */
  function revokeInstanceForTesting() {
    instancePromise = null;
    instance = null;
  };

  return {
    getInstance: getInstance,
    getInstanceForDebug: getInstanceForDebug,
    revokeInstanceForTesting: revokeInstanceForTesting
  };
}());
