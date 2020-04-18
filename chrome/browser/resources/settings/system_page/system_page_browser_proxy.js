// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/** @fileoverview Handles interprocess communication for the system page. */

cr.define('settings', function() {
  /** @interface */
  class SystemPageBrowserProxy {
    /** Shows the native system proxy settings. */
    showProxySettings() {}

    /**
     * @return {boolean} Whether hardware acceleration was enabled when the user
     *     started Chrome.
     */
    wasHardwareAccelerationEnabledAtStartup() {}
  }

  /**
   * @implements {settings.SystemPageBrowserProxy}
   */
  class SystemPageBrowserProxyImpl {
    /** @override */
    showProxySettings() {
      chrome.send('showProxySettings');
    }

    /** @override */
    wasHardwareAccelerationEnabledAtStartup() {
      return loadTimeData.getBoolean('hardwareAccelerationEnabledAtStartup');
    }
  }

  cr.addSingletonGetter(SystemPageBrowserProxyImpl);

  return {
    SystemPageBrowserProxy: SystemPageBrowserProxy,
    SystemPageBrowserProxyImpl: SystemPageBrowserProxyImpl,
  };
});
