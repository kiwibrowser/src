// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// The script requests pin and checks the input. If correct PIN (1234) is
// provided, the script requests to close the dialog and stops there. If wrong
// PIN is provided, the request is repeated until the limit of 3 bad tries is
// reached. If the dialog is closed, the request is repeated without considering
// it a wrong attempt. This allows the testing of quota limit of closed dialogs
// (3 closed dialogs per 10 minutes).
function userInputCallback(codeValue) {
  if (chrome.runtime.lastError) {
    // Should end up here only when quota is exceeded.
    lastError = chrome.runtime.lastError.message;
    return;
  }

  if (attempts >= 3) {
    chrome.test.sendMessage('No attempt left');
    return;
  }

  if (!codeValue || !codeValue.userInput) {
    chrome.certificateProvider.requestPin(
        {signRequestId: 123}, userInputCallback);
    chrome.test.sendMessage('User closed the dialog', function(message) {
      if (message == 'GetLastError') {
        chrome.test.sendMessage(lastError);
      }
    });
    return;
  }

  var success = codeValue.userInput == '1234';  // Validate the code.
  if (success) {
    chrome.certificateProvider.stopPinRequest(
        {signRequestId: 123}, closeCallback);
    chrome.test.sendMessage(lastError == '' ? 'Success' : lastError);
  } else {
    attempts++;
    var code = attempts < 3 ? {signRequestId: 123, errorType: 'INVALID_PIN'} : {
      signRequestId: 123,
      requestType: 'PUK',
      errorType: 'MAX_ATTEMPTS_EXCEEDED',
      attemptsLeft: 0
    };
    chrome.certificateProvider.requestPin(code, userInputCallback);
    chrome.test.sendMessage(lastError == '' ? 'Invalid PIN' : lastError);
  }
}

function closeCallback() {
  if (chrome.runtime.lastError != null) {
    console.error('Error: ' + chrome.runtime.lastError.message);
    lastError = chrome.runtime.lastError.message;
    return;
  }
}

var attempts = 0;
var lastError = '';
chrome.certificateProvider.requestPin({signRequestId: 123}, userInputCallback);
