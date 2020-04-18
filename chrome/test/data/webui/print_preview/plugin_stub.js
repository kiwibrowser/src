// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

cr.define('print_preview', function() {
  /**
   * Test version of the print preview PDF plugin
   */
  class PDFPluginStub {
    /**
     * @param {!Function} loadCallback The function to call when the plugin has
     *     loaded.
     */
    constructor(loadCallback) {
      /**
       * @private {!Function} The callback to run when the plugin has loaded.
       */
      this.loadCallback_ = loadCallback;
    }

    /**
     * Stubbed out since some tests result in a call.
     * @param {string} url The url to initialize the plugin to.
     * @param {boolean} color Whether the preview should be in color.
     * @param {!Array<number>} pages The pages to preview.
     * @param {boolean} modifiable Whether the source document is modifiable.
     */
    resetPrintPreviewMode(url, color, pages, modifiable) {}

    /**
     * Called when the preview area wants the plugin to load a preview page.
     * Immediately calls loadCallback_().
     * @param {string} url The preview URL
     * @param {number} index The index of the page number to load.
     */
    loadPreviewPage(url, index) {
      if (this.loadCallback_)
        this.loadCallback_();
    }
  }

  return {PDFPluginStub: PDFPluginStub};
});
