// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

var port;

function testUrl(domain) {
    return 'http://' + domain + ':' + port +
      '/extensions/favicon/test_file.html';
}

chrome.test.getConfig(function(config) {
  port = config.testServer.port;
  chrome.test.runTests([
    function host_permission() {
      var updateListener = function(tab_id, info, tab) {
        if (tab.favIconUrl) {
          chrome.test.succeed();
          ['url', 'title', 'favIconUrl'].forEach(function(field) {
            chrome.test.assertTrue(tab[field] != undefined);
          });
        }
      };
      chrome.tabs.create({url:'about:blank'}, function(tab) {
        chrome.tabs.onUpdated.addListener(updateListener);
        chrome.tabs.update({url:testUrl('a.com')});
      });
    },
    function no_host_permission() {
      var updateListener = function(tab_id, info, tab) {
        ['url', 'title', 'faviconUrl'].forEach(function(field) {
          chrome.test.assertEq(undefined, tab[field]);
        });
        chrome.test.succeed();
      };
      chrome.tabs.create({url:'about:blank'}, function(tab) {
        chrome.tabs.onUpdated.addListener(updateListener);
        chrome.tabs.update({url:testUrl('no_host_permission')});
      });
    }
  ]);
});
