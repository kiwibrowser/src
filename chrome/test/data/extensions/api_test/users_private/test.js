// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// This just tests the interface. It does not test for specific results, only
// that callbacks are correctly invoked, expected parameters are correct,
// and failures are detected.

function callbackResult(result) {
  if (chrome.runtime.lastError)
    chrome.test.fail(chrome.runtime.lastError.message);
  else if (result == false)
    chrome.test.fail('Failed: ' + result);
}

var kEmail1 = 'asdf@gmail.com';
var kEmail2 = 'asdf2@gmail.com';
var kName1 = kEmail1;
var kName2 = kEmail2;

var availableTests = [
  function addUser() {
    chrome.usersPrivate.addWhitelistedUser(
        kEmail1,
        function(result) {
          callbackResult(result);

          chrome.usersPrivate.getWhitelistedUsers(function(users) {
            var foundUser = false;
            users.forEach(function(user) {
              if (user.email == kEmail1 && user.name == kName1) {
                foundUser = true;
              }
            });
            chrome.test.assertTrue(foundUser);
            chrome.test.succeed();
          });
        });
  },

  function addAndRemoveUsers() {
    chrome.usersPrivate.addWhitelistedUser(
        kEmail1,
        function(result1) {
          callbackResult(result1);

          chrome.usersPrivate.addWhitelistedUser(
              kEmail2,
              function(result2) {
                callbackResult(result2);

                  chrome.usersPrivate.removeWhitelistedUser(
                      kEmail1,
                      function(result3) {

                        chrome.usersPrivate.getWhitelistedUsers(
                            function(users) {
                              chrome.test.assertTrue(users.length == 1);
                              chrome.test.assertEq(kEmail2, users[0].email);
                              chrome.test.assertEq(kName2, users[0].name);
                              chrome.test.succeed();
                            });

                      });
              });
        });

  },

  function isOwner() {
    chrome.usersPrivate.getCurrentUser(function(user) {
      // Since we are testing with --stub-cros-settings this should be true.
      chrome.test.assertTrue(user.isOwner);
      chrome.test.succeed();
    });
  },
];

var testToRun = window.location.search.substring(1);
chrome.test.runTests(availableTests.filter(function(op) {
  return op.name == testToRun;
}));
