// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

var nacl = nacl || {};

(function() {
/**
 * Takes the |moduleListData| input argument which represents data about
 * the currently available modules and populates the html jstemplate
 * with that data. It expects an object structure like the above.
 * @param {Object} moduleListData Information about available modules
 */
function renderTemplate(moduleListData) {
  // Process the template.
  var input = new JsEvalContext(moduleListData);
  var output = $('naclInfoTemplate');
  jstProcess(input, output);
}

/**
 * Asks the C++ NaClUIDOMHandler to get details about the NaCl and return
 * the data in returnNaClInfo() (below).
 */
function requestNaClInfo() {
  chrome.send('requestNaClInfo');
}

/**
 * Called by the WebUI to re-populate the page with data representing the
 * current state of NaCl.
 * @param {Object} moduleListData Information about available modules
 */
nacl.returnNaClInfo = function(moduleListData) {
  $('loading-message').hidden = 'hidden';
  $('body-container').hidden = '';
  renderTemplate(moduleListData);
};

// Get data and have it displayed upon loading.
document.addEventListener('DOMContentLoaded', requestNaClInfo);
})();
