// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

var appName = 'com.google.chrome.test.echo';

chrome.test.getConfig(function(config) {
    chrome.test.runTests([
      function invalidHostName() {
        var message = {"text": "Hello!"};
        chrome.runtime.sendNativeMessage(
            'not.installed.app', message,
            chrome.test.callbackFail(
                "Specified native messaging host not found.",
                function(response) {
                  chrome.test.assertEq(undefined, response);
                }));
      },

      function nonexistentHost() {
        var message = {"text": "Hello!"};
        chrome.runtime.sendNativeMessage(
            'com.google.chrome.test.host_binary_missing', message,
            chrome.test.callbackFail(
                "Specified native messaging host not found.",
                function(response) {
                  chrome.test.assertEq(undefined, response);
                }));
      },

      function sendMessageWithCallback() {
        var message = {"text": "Hi there!", "number": 3};
        chrome.runtime.sendNativeMessage(
            appName, message,
            chrome.test.callbackPass(function(response) {
          chrome.test.assertEq(1, response.id);
          chrome.test.assertEq(message, response.echo);
          chrome.test.assertEq(
              response.caller_url, window.location.origin + "/");
        }));
      },

      // The goal of this test is just not to crash.
      function sendMessageWithoutCallback() {
        var message = {"text": "Hi there!", "number": 3};
        chrome.extension.sendNativeMessage(appName, message);
        chrome.test.succeed(); // Mission Complete
      },

      function bigMessage() {
        // Create a special message for which the test host must try sending a
        // message that is bigger than the limit.
        var message = { "bigMessageTest": true };
        chrome.runtime.sendNativeMessage(
            appName, message,
            chrome.test.callbackFail(
                "Error when communicating with the native messaging host.",
                function(response) {
                  chrome.test.assertEq(undefined, response);
                }));
      },

      function connect() {
        var messagesToSend = [{"text": "foo"},
                              {"text": "bar", "funCount": 9001},
                              {}];
        var currentMessage = 0;

        port = chrome.extension.connectNative(appName);
        port.postMessage(messagesToSend[currentMessage]);

        port.onMessage.addListener(function(message) {
          chrome.test.assertEq(currentMessage + 1, message.id);
          chrome.test.assertEq(messagesToSend[currentMessage], message.echo);
          chrome.test.assertEq(
              message.caller_url, window.location.origin + "/");
          currentMessage++;

          if (currentMessage == messagesToSend.length)
            chrome.test.succeed();
          else
            port.postMessage(messagesToSend[currentMessage]);
        });
      },

      // Verify that the case when host stops itself is handled properly.
      function stopHost() {
        port = chrome.extension.connectNative(appName);

        port.onDisconnect.addListener(
            chrome.test.callback(function() {}, "Native host has exited."));

        // Send first message that should stop the host.
        port.postMessage({ "stopHostTest": true });
      }
    ]);
});
