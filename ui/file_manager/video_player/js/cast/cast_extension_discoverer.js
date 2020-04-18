// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// TODO(crbug.com/689773): Update to use <script>

/**
 * Discover the ID of installed cast extension.
 * @constructor
 * @struct
 */
function CastExtensionDiscoverer() {}

/**
 * Tentative IDs to try.
 * @type {!Array<string>}
 * @const
 */
CastExtensionDiscoverer.CAST_EXTENSION_IDS = [
  'enhhojjnijigcajfphajepfemndkmdlo',  // Media Router Dev
  'pkedcjkdefgpdelpbcmbmeomcjbeemfm'   // Media Router Stable
];

/**
 * @param {function(?string)} callback Callback called with the extension ID.
 *     The ID may be null if extension is not found.
 */
CastExtensionDiscoverer.findInstalledExtension = function(callback) {
  CastExtensionDiscoverer.findInstalledExtensionHelper_(0, callback);
};

/**
 * @param {number} index Current index which is tried to check.
 * @param {function(?string)} callback Callback function which will be called
 *     the extension is found.
 * @private
 */
CastExtensionDiscoverer.findInstalledExtensionHelper_ = function(index,
    callback) {
  if (index === CastExtensionDiscoverer.CAST_EXTENSION_IDS.length) {
    // no extension found.
    callback(null);
    return;
  }

  CastExtensionDiscoverer.isExtensionInstalled_(
      CastExtensionDiscoverer.CAST_EXTENSION_IDS[index],
      function(installed) {
        if (installed) {
          callback(CastExtensionDiscoverer.CAST_EXTENSION_IDS[index]);
        } else {
          CastExtensionDiscoverer.findInstalledExtensionHelper_(index + 1,
                                                                callback);
        }
      });
};

/**
 * The result will be notified on |callback|. True if installed, false not.
 * @param {string} extensionId Id to be checked.
 * @param {function(boolean)} callback Callback to notify the result.
 * @private
 */
CastExtensionDiscoverer.isExtensionInstalled_ =
    function(extensionId, callback) {

  var xhr = new XMLHttpRequest();
  var url = 'chrome-extension://' + extensionId + '/cast_sender.js';
  xhr.open('GET', url, true);
  xhr.onerror = function() { callback(false); };
  /** @param {*} event */
  xhr.onreadystatechange = function(event) {
    if (xhr.readyState == 4 && xhr.status === 200) {
      // Cast extension found.
      callback(true);
    }
  }.wrap(this);
  xhr.send();
};
