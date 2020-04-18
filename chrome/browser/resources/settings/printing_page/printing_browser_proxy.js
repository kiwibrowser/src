// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * @fileoverview A helper object used from the Chrome printing section to
 * interact with the browser. Used on operating system that is not Chrome OS.
 */

cr.define('settings', function() {
  /** @interface */
  class PrintingBrowserProxy {
    /**
     * Open the native print system dialog.
     */
    openSystemPrintDialog() {}
  }

  /**
   * @implements {settings.PrintingBrowserProxy}
   */
  class PrintingBrowserProxyImpl {
    /** @override */
    openSystemPrintDialog() {
      chrome.send('openSystemPrintDialog');
    }
  }

  cr.addSingletonGetter(PrintingBrowserProxyImpl);

  return {
    PrintingBrowserProxy: PrintingBrowserProxy,
    PrintingBrowserProxyImpl: PrintingBrowserProxyImpl,
  };
});
