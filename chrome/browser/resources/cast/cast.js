// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

window.addEventListener('load', function init() {
  var extensionView = document.querySelector('extensionview');

  /**
   * @param {string} str
   * @return {!Array<string>}
   */
  var splitUrlOnHash = function(str) {
    str = str || '';
    var pos = str.indexOf('#');
    return (pos !== -1) ? [str.substr(0, pos), str.substr(pos + 1)] : [str, ''];
  };

  new MutationObserver(function() {
    var newHash = splitUrlOnHash(extensionView.getAttribute('src'))[1];
    var oldHash = window.location.hash.substr(1);
    if (newHash !== oldHash) {
      window.location.hash = newHash;
    }
  }).observe(extensionView, {attributes: true});

  window.addEventListener('hashchange', function() {
    var newHash = window.location.hash.substr(1);
    var extensionViewSrcParts =
        splitUrlOnHash(extensionView.getAttribute('src'));
    if (newHash !== extensionViewSrcParts[1]) {
      extensionView.load(extensionViewSrcParts[0] + '#' + newHash);
    }
  });

  extensionView.load(
      'chrome-extension://' + loadTimeData.getString('extensionId') +
          '/cast_setup/index.html#' + window.location.hash.substr(1) ||
      'devices');
});
