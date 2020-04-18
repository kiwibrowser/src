// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

var kNewInputMethodTemplate = '_comp_ime_{EXT_ID}xkb:fr::fra';
var kInitialInputMethodRegex = /_comp_ime_([a-z]{32})xkb:us::eng/;
var kInvalidInputMethod = 'xx::xxx';

var testParams = {
  initialInputMethod: '',
  newInputMethod: '',
  dictionaryLoaded: null,
};

// The tests needs to be executed in order.

function initTests() {
  console.log('initTest: Getting initial inputMethod');
  chrome.inputMethodPrivate.getCurrentInputMethod(function(inputMethod) {
    testParams.initialInputMethod = inputMethod;

    var match = inputMethod.match(kInitialInputMethodRegex);
    chrome.test.assertTrue(!!match);
    chrome.test.assertEq(2, match.length);
    var extensionId = match[1];
    testParams.newInputMethod =
        kNewInputMethodTemplate.replace('{EXT_ID}', extensionId);
    chrome.test.succeed();
  });
}

function setTest() {
  chrome.test.assertTrue(!!testParams.newInputMethod);
  console.log(
      'setTest: Changing input method to: ' + testParams.newInputMethod);
  chrome.inputMethodPrivate.setCurrentInputMethod(testParams.newInputMethod,
    function() {
      chrome.test.assertTrue(
          !chrome.runtime.lastError,
          chrome.runtime.lastError ? chrome.runtime.lastError.message : '');
      chrome.test.succeed();
    });
}

function getTest() {
  chrome.test.assertTrue(!!testParams.newInputMethod);
  console.log('getTest: Getting current input method.');
  chrome.inputMethodPrivate.getCurrentInputMethod(function(inputMethod) {
    chrome.test.assertEq(testParams.newInputMethod, inputMethod);
    chrome.test.succeed();
  });
}

function observeTest() {
  chrome.test.assertTrue(!!testParams.initialInputMethod);
  console.log('observeTest: Adding input method event listener.');

  var listener = function(inputMethod) {
    chrome.inputMethodPrivate.onChanged.removeListener(listener);
    chrome.test.assertEq(testParams.initialInputMethod, inputMethod);
    chrome.test.succeed();
  };
  chrome.inputMethodPrivate.onChanged.addListener(listener);

  console.log('observeTest: Changing input method to: ' +
                  testParams.initialInputMethod);
  chrome.inputMethodPrivate.setCurrentInputMethod(
      testParams.initialInputMethod);
}


function setInvalidTest() {
  console.log(
      'setInvalidTest: Changing input method to: ' + kInvalidInputMethod);
  chrome.inputMethodPrivate.setCurrentInputMethod(kInvalidInputMethod,
    function() {
      chrome.test.assertTrue(!!chrome.runtime.lastError);
      chrome.test.succeed();
    });
}

function getListTest() {
  chrome.test.assertTrue(!!testParams.initialInputMethod);
  chrome.test.assertTrue(!!testParams.newInputMethod);
  console.log('getListTest: Getting input method list.');

  chrome.inputMethodPrivate.getInputMethods(function(inputMethods) {
    chrome.test.assertEq(6, inputMethods.length);
    var foundInitialInputMethod = false;
    var foundNewInputMethod = false;
    for (var i = 0; i < inputMethods.length; ++i) {
      if (inputMethods[i].id == testParams.initialInputMethod)
        foundInitialInputMethod = true;
      if (inputMethods[i].id == testParams.newInputMethod)
        foundNewInputMethod = true;
    }
    chrome.test.assertTrue(foundInitialInputMethod);
    chrome.test.assertTrue(foundNewInputMethod);
    chrome.test.succeed();
  });
}

// Helper function
function getFetchPromise() {
  return new Promise(function(resolve, reject) {
    chrome.inputMethodPrivate.fetchAllDictionaryWords(function(words) {
      if (!!chrome.runtime.lastError) {
        reject(Error(chrome.runtime.lastError));
      } else {
        resolve(words);
      }
    });
  });
}

// Helper function
function getAddPromise(word) {
  return new Promise(function(resolve, reject) {
    chrome.inputMethodPrivate.addWordToDictionary(word, function() {
      if (!!chrome.runtime.lastError) {
        reject(Error(chrome.runtime.lastError));
      } else {
        resolve();
      }
    });
  });
}

function loadDictionaryAsyncTest() {
  testParams.dictionaryLoaded = new Promise(function(resolve, reject) {
    var message = 'before';
    chrome.inputMethodPrivate.onDictionaryLoaded.addListener(
        function listener() {
          chrome.inputMethodPrivate.onDictionaryLoaded.removeListener(listener);
          chrome.test.assertEq(message, 'after');
          resolve();
        });
    message = 'after';
  });
  // We don't need to wait for the promise to resolve before continuing since
  // promises are async wrappers.
  chrome.test.succeed();
}

function fetchDictionaryTest() {
  testParams.dictionaryLoaded
      .then(function () {
        return getFetchPromise();
      })
      .then(function confirmFetch(words) {
        chrome.test.assertTrue(words !== undefined);
        chrome.test.assertTrue(words.length === 0);
        chrome.test.succeed();
      });
}

function addWordToDictionaryTest() {
  var wordToAdd = 'helloworld';
  testParams.dictionaryLoaded
      .then(function() {
        return getAddPromise(wordToAdd);
      })
      // Adding the same word results in an error.
      .then(function() {
        return getAddPromise(wordToAdd);
      })
      .catch(function(error) {
        chrome.test.assertTrue(!!error.message);
        return getFetchPromise();
      })
      .then(function(words) {
        chrome.test.assertTrue(words.length === 1);
        chrome.test.assertEq(words[0], wordToAdd);
        chrome.test.succeed();
      });
}

function dictionaryChangedTest() {
  var wordToAdd = 'helloworld2';
  testParams.dictionaryLoaded
      .then(function() {
        chrome.inputMethodPrivate.onDictionaryChanged.addListener(
            function(added, removed) {
              chrome.test.assertTrue(added.length === 1);
              chrome.test.assertTrue(removed.length === 0);
              chrome.test.assertEq(added[0], wordToAdd);
              chrome.test.succeed();
            });
      })
      .then(function() {
        return getAddPromise(wordToAdd);
      });
}

chrome.test.sendMessage('ready');
chrome.test.runTests(
    [initTests, setTest, getTest, observeTest, setInvalidTest, getListTest,
     loadDictionaryAsyncTest, fetchDictionaryTest, addWordToDictionaryTest,
     dictionaryChangedTest]);
