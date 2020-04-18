// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * @fileoverview Selector for picking an MR extension to use.


 */

goog.provide('mr.ExtensionId');
goog.provide('mr.ExtensionSelector');


/**
 * @enum {string}
 */
mr.ExtensionId = {
  PUBLIC: 'pkedcjkdefgpdelpbcmbmeomcjbeemfm',
  DEV: 'enhhojjnijigcajfphajepfemndkmdlo'
};


/**
 * @return {Promise} Resolves if this extension should start itself,
 *     rejects otherwise.
 */
mr.ExtensionSelector.shouldStart = function() {
  return new Promise((resolve, reject) => {
    switch (window.location.host) {
      case mr.ExtensionId.DEV:
        resolve();
        break;
      case mr.ExtensionId.PUBLIC:
        chrome.management.get(mr.ExtensionId.DEV, result => {
          if (chrome.runtime.lastError || !result.enabled) {
            resolve();
          } else {
            reject(Error('Dev extension is enabled'));
          }
        });
        break;
      default:
        reject(Error('Unknown extension id'));
    }
  });
};
