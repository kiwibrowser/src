// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/** @suppress {duplicate} */
var remoting = remoting || {};

(function(){

'use strict';

/**
 * @constructor
 */
remoting.WebsiteUsageTracker = function() {
  base.Ipc.getInstance().register(
      'websiteConnectionAttempted',
      remoting.WebsiteUsageTracker.onWebsiteVisited_,
      true);
};

remoting.WebsiteUsageTracker.getVisitCount = function() {
  var result = new base.Deferred();
  chrome.storage.sync.get(
      [STORAGE_KEY],
      (results) => {
        var count = Number(results[STORAGE_KEY]);
        result.resolve(isNaN(count) ? 0 : count);
      });
  return result.promise();
};

remoting.WebsiteUsageTracker.onWebsiteVisited_ = function() {
  remoting.WebsiteUsageTracker.getVisitCount().then(
      (count) => {
        chrome.storage.sync.set({
          [STORAGE_KEY]: count + 1
        });
      });
};


var STORAGE_KEY = 'website-host-connection-attempts-count';

}());
