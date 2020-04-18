// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Bookmark Manager API test for Chrome.
// browser_tests.exe --gtest_filter=ExtensionApiTest.BookmarkManagerEditDisabled

const pass = chrome.test.callbackPass;
const fail = chrome.test.callbackFail;
const assertEq = chrome.test.assertEq;
const assertFalse = chrome.test.assertFalse;
const assertTrue = chrome.test.assertTrue;
const bookmarks = chrome.bookmarks;
const bookmarkManager = chrome.bookmarkManagerPrivate;

var ERROR = "Bookmark editing is disabled.";

// Bookmark model within this test:
//  <root>/
//    Bookmarks Bar/
//      Folder/
//        "BBB"
//      "AAA"
//    Other Bookmarks/

var tests = [
  function verifyModel() {
    bookmarks.getTree(pass(function(result) {
      assertEq(1, result.length);
      var root = result[0];
      assertEq(2, root.children.length);
      bar = root.children[0];
      assertEq(2, bar.children.length);
      folder = bar.children[0];
      aaa = bar.children[1];
      assertEq('Folder', folder.title);
      assertEq('AAA', aaa.title);
      bbb = folder.children[0];
      assertEq('BBB', bbb.title);
    }));
  },

  function createDisabled() {
    bookmarks.create({ parentId: bar.id, title: 'Folder2' }, fail(ERROR));
  },

  function moveDisabled() {
    bookmarks.move(aaa.id, { parentId: folder.id }, fail(ERROR));
  },

  function removeDisabled() {
    bookmarks.remove(aaa.id, fail(ERROR));
  },

  function removeTreeDisabled() {
    bookmarks.removeTree(folder.id, fail(ERROR));
  },

  function updateDisabled() {
    bookmarks.update(aaa.id, { title: 'CCC' }, fail(ERROR));
  },

  function importDisabled() {
    bookmarks.import(fail(ERROR));
  },

  function cutDisabled() {
    bookmarkManager.cut([bbb.id], fail(ERROR));
  },

  function canPasteDisabled() {
    bookmarkManager.canPaste(folder.id, pass(function(result) {
      assertFalse(result, 'Should not be able to paste bookmarks');
    }));
  },

  function pasteDisabled() {
    bookmarkManager.paste(folder.id, [bbb.id], fail(ERROR));
  },

  function editDisabled() {
    bookmarkManager.canEdit(pass(function(result) {
      assertFalse(result, 'Should not be able to edit bookmarks');
    }));
  }
];

chrome.test.runTests(tests);
