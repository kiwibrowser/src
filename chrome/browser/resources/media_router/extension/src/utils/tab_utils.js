// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * @fileoverview Tab-related utilities methods.

 */

goog.provide('mr.TabUtils');


/**
 * @param {number} tabId
 * @return {!Promise<!Tab>} A promise fulfilled with tab, or rejected if
 *     tab does not exist.
 */
mr.TabUtils.getTab = function(tabId) {
  return new Promise((resolve, reject) => {
           chrome.tabs.get(tabId, resolve);
         })
      .then(tab => {
        if (!tab) {
          throw Error('No such tab ' + tabId);
        }
        return tab;
      });
};
