// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * @fileoverview An element that represents an SSL certificate entry.
 */
Polymer({
  is: 'certificate-entry',

  behaviors: [I18nBehavior],

  properties: {
    /** @type {!Certificate} */
    model: Object,

    /** @type {!CertificateType} */
    certificateType: String,
  },

  /**
   * @param {number} index
   * @return {boolean} Whether the given index corresponds to the last sub-node.
   * @private
   */
  isLast_: function(index) {
    return index == this.model.subnodes.length - 1;
  },
});
