// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

goog.provide('mr.mirror.DefaultServiceLoader');
goog.provide('mr.mirror.ServiceLoader');



/**
 * Loads a mirror.Service. Note that this loader does not need to handle event
 * page suspending and waking up (the event page is running when there is a
 * local mirroring route).
 * @record
 */
mr.mirror.ServiceLoader = class {
  /**
   * Loads and returns the service.
   * @return {!Promise<!mr.mirror.Service>}
   */
  loadService() {}
};


/**
 * A lightweight implementation of ServiceLoader which just returns the instance
 * provided to the constructor.
 * @implements {mr.mirror.ServiceLoader}
 */
mr.mirror.DefaultServiceLoader = class {
  /**
   * @param {!mr.mirror.Service} serviceInstance
   */
  constructor(serviceInstance) {
    /**
     * @private {!mr.mirror.Service}
     */
    this.serviceInstance_ = serviceInstance;
  }

  /**
   * @override
   */
  loadService() {
    return Promise.resolve(this.serviceInstance_);
  }
};
