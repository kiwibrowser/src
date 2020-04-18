// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

'use strict';

/**
 * Obtain a URL GET parameter based on its name in order to use it for NaCl
 * module configuration.
 * @param {string} name The name of the GET parameter.
 * @return {string} The corresponding GET parameter value.
 */
function getUrlParameter(name) {
  var getUrlRegex = new RegExp(
      '[?|&]' + name + '=' +
      '([^&;]+?)(&|#|;|$)');
  var param =
      (getUrlRegex.exec(location.search) || [, ''])[1].replace(/\+/g, '%20');
  return decodeURIComponent(param) || null;
}

// Load the NaCl module.
document.addEventListener('DOMContentLoaded', function() {
  var pathToNmfFile = getUrlParameter('pathToNmfFile');
  var mimeType = getUrlParameter('mimeType');
  if (!pathToNmfFile || !mimeType) {
    console.error('No nmf configure file or mime type for the NaCl module.');
    return;
  }

  var elementDiv = document.createElement('div');
  // No need to handle messages from NaCl. Tests output is displayed only to
  // stdout.
  elementDiv.addEventListener('load', function() {
    window.close();  // Kill browser once NaCl module is loaded. Tests are
                     // run when NaCl module is loading, so by the time we
                     // get the 'load' event, tests were already run.
  }, true);

  var elementEmbed = document.createElement('embed');
  elementEmbed.style.width = 0;
  elementEmbed.style.height = 0;
  elementEmbed.src = pathToNmfFile;
  elementEmbed.type = mimeType;
  elementDiv.appendChild(elementEmbed);

  document.body.appendChild(elementDiv);
});
