// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

var assertNoLastError = chrome.test.assertNoLastError;

var tests = [
  function simple() {
    chrome.contextMenus.create({"title":"1"}, chrome.test.callbackPass());
  },

  function no_properties() {
    chrome.contextMenus.create({}, function(id) {
      chrome.test.assertTrue(chrome.runtime.lastError != null);
      chrome.test.succeed();
    });
  },

  function remove() {
    var id;
    id = chrome.contextMenus.create({"title":"1"}, function() {
      assertNoLastError();
      chrome.contextMenus.remove(id, chrome.test.callbackPass());
    });
  },

  function update() {
    var id;
    id = chrome.contextMenus.create({"title":"update test"}, function() {
      assertNoLastError();
      chrome.contextMenus.update(id, {"title": "test2"},
                                chrome.test.callbackPass());
    });

    chrome.contextMenus.create({"id": "test3", "type": "checkbox",
                                "title": "test3"}, function() {
      assertNoLastError();
      // Calling update without specifying "type" should not change the menu
      // item's type to "normal" and therefore setting "checked" should not
      // fail.
      chrome.contextMenus.update("test3", {"checked": true},
                                chrome.test.callbackPass());
    });
  },

  function removeAll() {
    chrome.contextMenus.create({"title":"1"}, function() {
      assertNoLastError();
      chrome.contextMenus.create({"title":"2"}, function() {
        assertNoLastError();
        chrome.contextMenus.removeAll(chrome.test.callbackPass());
      });
    });
  },

  function hasParent() {
    var id;
    id = chrome.contextMenus.create({"title":"parent"}, function() {
      assertNoLastError();
      chrome.contextMenus.create({"title":"child", "parentId":id},
                                function() {
        assertNoLastError();
        chrome.test.succeed();
      });
    });
  }
];


// Add tests for creating menu item with various types and contexts.
var types = ["checkbox", "radio", "separator"];
var contexts = ["all", "page", "selection", "link", "editable", "image",
                "video", "audio"];
function makeCreateTest(type, contexts) {
  var result = function() {
    var title = type;
    if (contexts && contexts.length > 0) {
      title += " " + contexts.join(",");
    }
    var properties = {"title": title, "type": type};

    chrome.contextMenus.create(properties, chrome.test.callbackPass());
  };
  result.generatedName = "create_" + type +
                         (contexts ? "-" + contexts.join(",") : "");
  return result;
}

for (var i in types) {
  tests.push(makeCreateTest(types[i]));
}
for (var i in contexts) {
  tests.push(makeCreateTest("normal", [ contexts[i] ]));
}

chrome.test.runTests(tests);
