// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

cr.define('settings', function() {
  /** @interface */
  class ChromeCleanupProxy {
    /**
     * Registers the current ChromeCleanupHandler as an observer of
     * ChromeCleanerController events.
     */
    registerChromeCleanerObserver() {}

    /**
     * Starts scanning the user's computer.
     * @param {boolean} logsUploadEnabled
     */
    startScanning(logsUploadEnabled) {}

    /**
     * Starts a cleanup on the user's computer.
     * @param {boolean} logsUploadEnabled
     */
    startCleanup(logsUploadEnabled) {}

    /**
     * Restarts the user's computer.
     */
    restartComputer() {}

    /**
     * Updates the cleanup logs upload permission status.
     * @param {boolean} enabled
     */
    setLogsUploadPermission(enabled) {}

    /**
     * Notifies Chrome that the state of the details section changed.
     * @param {boolean} enabled
     */
    notifyShowDetails(enabled) {}

    /**
     * Notifies Chrome that the "learn more" link was clicked.
     */
    notifyLearnMoreClicked() {}

    /**
     * Requests the plural string for the "show more" link in the detailed
     * view for either files to delete or registry keys.
     * @param {number} numHiddenItems
     * @return {!Promise<string>}
     */
    getMoreItemsPluralString(numHiddenItems) {}

    /**
     * Requests the plural string for the "items to remove" link in the detailed
     * view.
     * @param {number} numItems
     * @return {!Promise<string>}
     */
    getItemsToRemovePluralString(numItems) {}
  }

  /**
   * @implements {settings.ChromeCleanupProxy}
   */
  class ChromeCleanupProxyImpl {
    /** @override */
    registerChromeCleanerObserver() {
      chrome.send('registerChromeCleanerObserver');
    }

    /** @override */
    startScanning(logsUploadEnabled) {
      chrome.send('startScanning', [logsUploadEnabled]);
    }

    /** @override */
    startCleanup(logsUploadEnabled) {
      chrome.send('startCleanup', [logsUploadEnabled]);
    }

    /** @override */
    restartComputer() {
      chrome.send('restartComputer');
    }

    /** @override */
    setLogsUploadPermission(enabled) {
      chrome.send('setLogsUploadPermission', [enabled]);
    }

    /** @override */
    notifyShowDetails(enabled) {
      chrome.send('notifyShowDetails', [enabled]);
    }

    /** @override */
    notifyLearnMoreClicked() {
      chrome.send('notifyChromeCleanupLearnMoreClicked');
    }

    /** @override */
    getMoreItemsPluralString(numHiddenItems) {
      return cr.sendWithPromise('getMoreItemsPluralString', numHiddenItems);
    }

    /** @override */
    getItemsToRemovePluralString(numItems) {
      return cr.sendWithPromise('getItemsToRemovePluralString', numItems);
    }
  }

  cr.addSingletonGetter(ChromeCleanupProxyImpl);

  return {
    ChromeCleanupProxy: ChromeCleanupProxy,
    ChromeCleanupProxyImpl: ChromeCleanupProxyImpl,
  };
});
