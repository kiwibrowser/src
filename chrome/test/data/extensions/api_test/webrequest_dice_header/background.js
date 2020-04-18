// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

window.diceResponseHeaderCount = 0;
window.controlResponseHeaderCount = 0;

chrome.webRequest.onHeadersReceived.addListener(function(details) {
  let diceHeaderFound = false;
  const headerValue = 'ValueFromExtension'
  const diceResponseHeader = 'X-Chrome-ID-Consistency-Response';
  details.responseHeaders.forEach(function(header) {
    if (header.name == diceResponseHeader){
      ++window.diceResponseHeaderCount;
      diceHeaderFound = true;
      header.value = headerValue;
    } else if (header.name == 'X-Control'){
      ++window.controlResponseHeaderCount;
      header.value = headerValue;
    }
  });
  if (!diceHeaderFound) {
    details.responseHeaders.push({name: diceResponseHeader,
                                  value: headerValue});
  }
  details.responseHeaders.push({name: 'X-New-Header',
                                value: headerValue});
  return {responseHeaders: details.responseHeaders};
},
{urls: ['http://*/extensions/dice.html']},
['blocking', 'responseHeaders']);

chrome.test.sendMessage('ready');
