// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

Polymer({
  is: 'setting-zippy',

  properties: {
    iconSrc: {
      type: String,
      value: null,
    },

    hideLine: {
      type: Boolean,
      value: false,
    },
  },

  /**
   * Wrap the icon as a image into a html snippet.
   *
   * @param {string} iconUri the icon uri to be wrapped.
   * @return {string} wrapped html snippet.
   *
   * @private
   */
  getWrappedIcon: function(iconUri) {
    return String.raw`
    <html>
      <style>
        body {
          margin: 0;
        }
        #icon {
          width: 24px;
          height: 24px;
        }
      </style>
    <body><img id='icon' src="` +
        iconUri + '"></body></html>';
  },
});
