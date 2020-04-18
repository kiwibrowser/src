// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Bookmark Manager API test for Chrome.
// browser_tests.exe --gtest_filter=ExtensionApiTest.BookmarkManager

const pass = chrome.test.callbackPass;
const fail = chrome.test.callbackFail;
const assertEq = chrome.test.assertEq;
const assertTrue = chrome.test.assertTrue;
const assertFalse = chrome.test.assertFalse;
const bookmarks = chrome.bookmarks;
const bookmarkManager = chrome.bookmarkManagerPrivate;
var fooNode, fooNode2, barNode, gooNode, count, emptyFolder, emptyFolder2;
var folder, nodeA, nodeB;
var childFolder, grandChildFolder, childNodeA, childNodeB;

var clipboardArguments;
function doCopy() {
  clipboardArguments = arguments;
  document.execCommand('copy');
}
function doCut() {
  clipboardArguments = arguments;
  document.execCommand('cut');
}
function doPaste() {
  clipboardArguments = arguments;
  document.execCommand('paste');
}
document.addEventListener('copy', function (event) {
  bookmarkManager.copy.apply(null, clipboardArguments);
  event.preventDefault();
});
document.addEventListener('cut', function (event) {
  bookmarkManager.cut.apply(null, clipboardArguments);
  event.preventDefault();
});
document.addEventListener('paste', function (event) {
  bookmarkManager.paste.apply(null, clipboardArguments);
  event.preventDefault();
});

var tests = [
  function sortChildren() {
    folder = {
      parentId: '1',
      title: 'Folder'
    };
    nodeA = {
      title: 'a',
      url: 'http://www.example.com/a'
    };
    nodeB = {
      title: 'b',
      url: 'http://www.example.com/b'
    };
    bookmarks.create(folder, pass(function(result) {
      folder.id = result.id;
      nodeA.parentId = folder.id;
      nodeB.parentId = folder.id;

      bookmarks.create(nodeB, pass(function(result) {
        nodeB.id = result.id;
      }));
      bookmarks.create(nodeA, pass(function(result) {
        nodeA.id = result.id;
      }));
    }));
  },

  function sortChildren2() {
    bookmarkManager.sortChildren(folder.id);

    bookmarks.getChildren(folder.id, pass(function(children) {
      assertEq(nodeA.id, children[0].id);
      assertEq(nodeB.id, children[1].id);
    }));
  },

  function setupSubtree() {
    childFolder = {
      parentId: folder.id,
      title: 'Child Folder'
    };
    childNodeA = {
      title: 'childNodeA',
      url: 'http://www.example.com/childNodeA'
    };
    childNodeB = {
      title: 'childNodeB',
      url: 'http://www.example.com/childNodeB'
    };
    grandChildFolder = {
      title: 'grandChildFolder'
    };
    bookmarks.create(childFolder, pass(function(result) {
      childFolder.id = result.id;
      childNodeA.parentId = childFolder.id;
      childNodeB.parentId = childFolder.id;
      grandChildFolder.parentId = childFolder.id;

      bookmarks.create(childNodeA, pass(function(result) {
        childNodeA.id = result.id;
      }));
      bookmarks.create(childNodeB, pass(function(result) {
        childNodeB.id = result.id;
      }));
      bookmarks.create(grandChildFolder, pass(function(result) {
        grandChildFolder.id = result.id;
      }));
    }))
  },

  function getSubtree() {
    bookmarkManager.getSubtree(childFolder.id, false, pass(function(result) {
      var children = result[0].children;
      assertEq(3, children.length);
      assertEq(childNodeA.id, children[0].id);
      assertEq(childNodeB.id, children[1].id);
      assertEq(grandChildFolder.id, children[2].id);
    }))
  },

  function getSubtreeFoldersOnly() {
    bookmarkManager.getSubtree(childFolder.id, true, pass(function(result) {
      var children = result[0].children;
      assertEq(1, children.length);
      assertEq(grandChildFolder.id, children[0].id);
    }))
  },

  // The clipboard test is split into different parts to allow asynchronous
  // operations to finish.
  function clipboard() {
    // Create a new bookmark.
    fooNode = {
      parentId: '1',
      title: 'Foo',
      url: 'http://www.example.com/foo'
    };

    emptyFolder = {
      parentId: '1',
      title: 'Empty Folder'
    }

    bookmarks.create(fooNode, pass(function(result) {
      fooNode.id = result.id;
      fooNode.index = result.index;
      count = result.index + 1;
    }));

    bookmarks.create(emptyFolder, pass(function(result) {
      emptyFolder.id = result.id;
      emptyFolder.index = result.index;
      count = result.index + 1;
    }));

    // Create a couple more bookmarks to test proper insertion of pasted items.
    barNode = {
      parentId: '1',
      title: 'Bar',
      url: 'http://www.example.com/bar'
    };

    bookmarks.create(barNode, pass(function(result) {
      barNode.id = result.id;
      barNode.index = result.index;
      count = result.index + 1;
    }));

    gooNode = {
      parentId: '1',
      title: 'Goo',
      url: 'http://www.example.com/goo'
    };

    bookmarks.create(gooNode, pass(function(result) {
      gooNode.id = result.id;
      gooNode.index = result.index;
      count = result.index + 1;
    }));
  },

  function clipboard2() {
    // Copy the fooNode.
    doCopy([fooNode.id]);

    // Ensure canPaste is now true.
    bookmarkManager.canPaste('1', pass(function(result) {
      assertTrue(result, 'Should be able to paste now');
    }));

    // Paste it.
    doPaste('1');

    // Ensure it got added at the end.
    bookmarks.getChildren('1', pass(function(result) {
      count++;
      assertEq(count, result.length);

      fooNode2 = result[result.length - 1];

      assertEq(fooNode.title + " (1)", fooNode2.title);
      assertEq(fooNode.url, fooNode2.url);
      assertEq(fooNode.parentId, fooNode2.parentId);
    }));
  },

  function clipboard3() {
    // Cut fooNode bookmarks.
    doCut([fooNode.id, fooNode2.id]);

    // Ensure count decreased by 2.
    bookmarks.getChildren('1', pass(function(result) {
      count -= 2;
      assertEq(count, result.length);
    }));

    // Ensure canPaste is still true.
    bookmarkManager.canPaste('1', pass(function(result) {
      assertTrue(result, 'Should be able to paste now');
    }));
  },

  function clipboard4() {
    // Paste the cut bookmarks at a specific position between bar and goo.
    doPaste('1', [barNode.id]);

    // Check that the two bookmarks were pasted after bar.
    bookmarks.getChildren('1', pass(function(result) {
      count += 2;
      assertEq(count, result.length);

      // Look for barNode's index.
      for (var barIndex = 0; barIndex < result.length; barIndex++) {
        if (result[barIndex].id == barNode.id)
          break;
      }
      assertTrue(barIndex + 2 < result.length);

      var last = result[barIndex + 1];
      var last2 = result[barIndex + 2];
      assertEq(fooNode.title, last.title);
      assertEq(fooNode.url, last.url);
      assertEq(fooNode.parentId, last.parentId);
      assertEq(last.title + " (1)", last2.title);
      assertEq(last.url, last2.url);
      assertEq(last.parentId, last2.parentId);

      // Remember last2 id, so we can use it in next test.
      fooNode2.id = last2.id;
    }));
  },

  // Ensure we can copy empty folders
  function clipboard5() {
    // Copy it.
    doCopy([emptyFolder.id]);

    // Ensure canPaste is now true.
    bookmarkManager.canPaste('1', pass(function(result) {
      assertTrue(result, 'Should be able to paste now');
    }));

    // Paste it at the end of a multiple selection.
    doPaste('1', [barNode.id, fooNode2.id]);

    // Ensure it got added at the right place.
    bookmarks.getChildren('1', pass(function(result) {
      count++;
      assertEq(count, result.length);

      // Look for fooNode2's index.
      for (var foo2Index = 0; foo2Index < result.length; foo2Index++) {
        if (result[foo2Index].id == fooNode2.id)
          break;
      }
      assertTrue(foo2Index + 1 < result.length);

      emptyFolder2 = result[foo2Index + 1];

      assertEq(emptyFolder2.title, emptyFolder.title);
      assertEq(emptyFolder2.url, emptyFolder.url);
      assertEq(emptyFolder2.parentId, emptyFolder.parentId);
    }));
  },

  function clipboard6() {
    // Verify that we can't cut managed folders.
    bookmarks.getChildren('4', pass(function(result) {
      assertEq(2, result.length);
      const error = "Can't modify managed bookmarks.";
      bookmarkManager.cut([ result[0].id ], fail(error));

      // Copying is fine.
      bookmarkManager.copy([ result[0].id ], pass());

      // Pasting to a managed folder is not allowed.
      assertTrue(result[1].url === undefined);
      bookmarkManager.canPaste(result[1].id, pass(function(result) {
        assertFalse(result, 'Should not be able to paste to managed folders.');
      }));

      bookmarkManager.paste(result[1].id, fail(error));
    }));
  },

  function canEdit() {
    bookmarkManager.canEdit(pass(function(result) {
      assertTrue(result, 'Should be able to edit bookmarks');
    }));
  },

  function getSetMetaInfo() {
    bookmarkManager.getMetaInfo(nodeA.id, 'meta', pass(function(result) {
      assertTrue(!result);
    }));
    chrome.test.listenOnce(bookmarkManager.onMetaInfoChanged, pass(
        function(id, changes) {
      assertEq(nodeA.id, id);
      assertEq({meta: 'bla'}, changes);
    }));
    bookmarkManager.setMetaInfo(nodeA.id, 'meta', 'bla');
    bookmarkManager.setMetaInfo(nodeA.id, 'meta2', 'foo');
    bookmarkManager.getMetaInfo(nodeA.id, 'meta', pass(function(result) {
      assertEq('bla', result);
    }));

    bookmarkManager.getMetaInfo(nodeA.id, pass(function(result) {
      assertEq({meta: 'bla', meta2: 'foo'}, result);
    }));
  },

  function setMetaInfoPermanent() {
    bookmarks.getTree(pass(function(nodes) {
      var unmodifiableFolder = nodes[0].children[0];
      bookmarkManager.setMetaInfo(unmodifiableFolder.id, 'meta', 'foo', fail(
          "Can't modify the root bookmark folders."));
      bookmarkManager.updateMetaInfo(unmodifiableFolder.id, {a: 'a', b: 'b'},
          fail("Can't modify the root bookmark folders."));
    }));
  },

  function setMetaInfoManaged() {
    bookmarks.getChildren('4', pass(function(result) {
      assertTrue(result.length > 0);
      bookmarkManager.setMetaInfo(result[0].id, 'meta', 'foo', fail(
          "Can't modify managed bookmarks."));
      bookmarkManager.updateMetaInfo(result[0].id, {a: 'a', b: 'b'},
          fail("Can't modify managed bookmarks."));
    }));
  },

  function updateMetaInfo() {
    bookmarkManager.getMetaInfo(nodeB.id, pass(function(result){
      assertEq({}, result);
    }));

    chrome.test.listenOnce(bookmarkManager.onMetaInfoChanged, pass(
        function(id, changes) {
      assertEq(nodeB.id, id);
      assertEq({a: 'a', b: 'b', c: 'c'}, changes);
    }));
    bookmarkManager.updateMetaInfo(nodeB.id, {a: 'a', b: 'b', c: 'c'}, pass(
        function() {
      chrome.test.listenOnce(bookmarkManager.onMetaInfoChanged, pass(
          function(id, changes) {
        assertEq(nodeB.id, id);
        assertEq({a: 'aa', d: 'd'}, changes);
      }));
      bookmarkManager.updateMetaInfo(nodeB.id, {a: 'aa', b: 'b', d: 'd'});
      bookmarkManager.getMetaInfo(nodeB.id, pass(function(result) {
        assertEq({a: 'aa', b: 'b', c: 'c', d: 'd'}, result);
      }));
    }));
  },

  function createWithMetaInfo() {
    var node = {title: 'title', url: 'http://www.google.com/'};
    var metaInfo = {a: 'a', b: 'b'};
    chrome.test.listenOnce(bookmarks.onCreated, pass(function(id, created) {
      assertEq(node.title, created.title);
      assertEq(node.url, created.url);
      bookmarkManager.getMetaInfo(id, pass(function(result) {
        assertEq(metaInfo, result);
      }));
    }));
    bookmarkManager.createWithMetaInfo(node, metaInfo, pass(
        function(createdNode) {
      assertEq(node.title, createdNode.title);
      assertEq(node.url, createdNode.url);
    }));
  }
];

chrome.test.runTests(tests);
