// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

window.Polymer = window.Polymer || {};
window.Polymer.dom = 'shadow';

/**
 * @type {Promise} A promise which is fulfilled when HTML imports for custom
 *   elements for file manager UI are loaded.
 */
window.importElementsPromise = new Promise(function(resolve, reject) {
  var startTime = Date.now();

  var link = document.createElement('link');
  link.rel = 'import';
  link.href = 'foreground/elements/elements_bundle.html';
  link.setAttribute('async', '');
  link.onload = function() {
    chrome.metricsPrivate.recordTime(
        'FileBrowser.Load.ImportElements', Date.now() - startTime);
    resolve();
  };
  link.onerror = reject;
  document.head.appendChild(link);
});
