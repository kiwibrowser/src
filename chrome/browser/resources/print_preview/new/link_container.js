// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

Polymer({
  is: 'print-preview-link-container',

  properties: {
    appKioskMode: Boolean,

    /** @type {?print_preview.Destination} */
    destination: Object,

    disabled: Boolean,

    /** @private {boolean} */
    shouldShowSystemDialogLink_: {
      type: Boolean,
      computed: 'computeShouldShowSystemDialogLink_(appKioskMode, destination)',
    },
  },

  /**
   * @return {boolean} Whether the system dialog link should be visible.
   * @private
   */
  computeShouldShowSystemDialogLink_: function() {
    if (this.appKioskMode)
      return false;
    if (!cr.isWindows)
      return true;
    return !!this.destination &&
        this.destination.origin == print_preview.DestinationOrigin.LOCAL &&
        this.destination.id !=
        print_preview.Destination.GooglePromotedId.SAVE_AS_PDF;
  },

  /** @private */
  onSystemDialogClick_: function() {
    if (!this.shouldShowSystemDialogLink_)
      return;
    // <if expr="not is_win">
    this.$.systemDialogThrobber.removeAttribute('hidden');
    // </if>
    this.fire('print-with-system-dialog');
  },

  // <if expr="is_macosx">
  /** @private */
  onOpenInPreviewClick_: function() {
    this.$.openPdfInPreviewThrobber.removeAttribute('hidden');
    this.fire('open-pdf-in-preview');
  },
  // </if>
});
