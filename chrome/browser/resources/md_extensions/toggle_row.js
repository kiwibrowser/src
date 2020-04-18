// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

cr.define('extensions', function() {
  'use strict';

  /**
   * An extensions-toggle-row provides a way of having a clickable row that can
   * modify a cr-toggle, by leveraging the native <label> functionality. It uses
   * a hidden native <input type="checkbox"> to achieve this.
   */
  const ExtensionsToggleRow = Polymer({
    is: 'extensions-toggle-row',

    properties: {
      checked: Boolean,
    },

    /**
     * Exposing the clickable part of extensions-toggle-row for testing
     * purposes.
     * @return {!HTMLElement}
     */
    getLabel() {
      return this.$.label;
    },

    /**
     * @param {!Event}
     * @private
     */
    onNativeClick_: function(e) {
      // Even though the native checkbox is hidden and can't be actually
      // cilcked/tapped by the user, because it resides within the <label> the
      // browser emits an extraneous event when the label is clicked. Stop
      // propagation so that it does not interfere with |onLabelTap_| listener.
      e.stopPropagation();
    },

    /**
     * Fires when the native checkbox changes value. This happens when the user
     * clicks directly on the <label>.
     * @param {!Event} e
     * @private
     */
    onNativeChange_: function(e) {
      e.stopPropagation();

      // Sync value of native checkbox and cr-toggle and |checked|.
      this.$.crToggle.checked = this.$.native.checked;
      this.checked = this.$.native.checked;

      this.fire('change', this.checked);
    },

    /**
     * Fires
     * @param {!CustomEvent} e
     * @private
     */
    onCrToggleChange_: function(e) {
      e.stopPropagation();

      // Sync value of native checkbox and cr-toggle.
      this.$.native.checked = e.detail;

      this.fire('change', this.checked);
    },
  });

  return {ExtensionsToggleRow: ExtensionsToggleRow};
});
