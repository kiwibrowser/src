// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * @fileoverview A helper object used from the the People section to interact
 * with the Easy Unlock functionality of the browser. ChromeOS only.
 */
cr.exportPath('settings');

cr.define('settings', function() {
  /** @interface */
  class EasyUnlockBrowserProxy {
    /**
     * Returns a true promise if Easy Unlock is already enabled on the device.
     * @return {!Promise<boolean>}
     */
    getEnabledStatus() {}

    /**
     * Starts the Easy Unlock setup flow.
     */
    startTurnOnFlow() {}

    /**
     * Returns the Easy Unlock turn off flow status.
     * @return {!Promise<string>}
     */
    getTurnOffFlowStatus() {}

    /**
     * Begins the Easy Unlock turn off flow.
     */
    startTurnOffFlow() {}

    /**
     * Cancels any in-progress Easy Unlock turn-off flows.
     */
    cancelTurnOffFlow() {}
  }

  /**
   * @implements {settings.EasyUnlockBrowserProxy}
   */
  class EasyUnlockBrowserProxyImpl {
    /** @override */
    getEnabledStatus() {
      return cr.sendWithPromise('easyUnlockGetEnabledStatus');
    }

    /** @override */
    startTurnOnFlow() {
      chrome.send('easyUnlockStartTurnOnFlow');
    }

    /** @override */
    getTurnOffFlowStatus() {
      return cr.sendWithPromise('easyUnlockGetTurnOffFlowStatus');
    }

    /** @override */
    startTurnOffFlow() {
      chrome.send('easyUnlockStartTurnOffFlow');
    }

    /** @override */
    cancelTurnOffFlow() {
      chrome.send('easyUnlockCancelTurnOffFlow');
    }
  }

  // The singleton instance_ is replaced with a test version of this wrapper
  // during testing.
  cr.addSingletonGetter(EasyUnlockBrowserProxyImpl);

  return {
    EasyUnlockBrowserProxy: EasyUnlockBrowserProxy,
    EasyUnlockBrowserProxyImpl: EasyUnlockBrowserProxyImpl,
  };
});
