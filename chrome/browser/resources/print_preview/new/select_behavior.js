// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

cr.define('print_preview_new', function() {
  /**
   * Helper functions for a select with timeout. Implemented by select settings
   * sections, so that the preview does not immediately begin generating and
   * freeze the dropdown when the value is changed.
   * Assumes that the elements implementing this behavior have no more than one
   * select element.
   * @polymerBehavior
   */
  const SelectBehavior = {
    /** @private {string} */
    lastValue_: '',

    /**
     * Timeout used to delay processing of the selection.
     * @private {?number}
     */
    timeout_: null,

    /** @override */
    ready: function() {
      assert(this.shadowRoot.querySelectorAll('select').length == 1);
      this.$$('select').addEventListener('change', () => {
        if (this.timeout_) {
          clearTimeout(this.timeout_);
        }
        this.timeout_ = setTimeout(this.onTimeout_.bind(this), 100);
      });
    },

    /**
     * Should be overridden by elements using this behavior to receive select
     * value updates.
     * @param {string} value The new select value to process.
     */
    onProcessSelectChange: function(value) {},

    /**
     * Called after a timeout after user selects a new option.
     * @private
     */
    onTimeout_: function() {
      this.timeout_ = null;
      const value = /** @type {!HTMLSelectElement} */ (this.$$('select')).value;
      if (this.lastValue_ != value) {
        this.lastValue_ = value;
        this.onProcessSelectChange(value);

        // For testing only
        this.fire('process-select-change');
      }
    },
  };

  return {SelectBehavior: SelectBehavior};
});
