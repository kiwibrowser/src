// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

const html = `
    <iframe src="/iframe.html" width="400" height="200" id="iframe1"></iframe>`;
const iframe = `
    <link rel="stylesheet" type="text/css" href="style.css">
    <h1>Hello from the iframe!</h1>`;
const css = `.test { color: blue; }`;

const server = new Map([
  ['http://test.com/index.html', { body: html }],
  ['http://test.com/iframe.html', { headers: ['HTTP/1.1 302 Found', 'Location: iframe2.html']}],
  ['http://test.com/iframe2.html', { body: iframe }],
  ['http://test.com/style.css', { headers: ['HTTP/1.1 302 Found', 'Location: style2.css'] }],
  ['http://test.com/style2.css', { body: css }]]);

(async function(testRunner) {
  var {page, session, dp} = await testRunner.startBlank(
      `Tests that redirects don't break virtual time.`);
  await dp.Network.enable();
  await dp.Network.setRequestInterception({ patterns: [{ urlPattern: '*' }] });

  dp.Network.onRequestIntercepted(event => {
    let body = server.get(event.params.request.url).body || '';
    let headers = server.get(event.params.request.url).headers || [];
    dp.Network.continueInterceptedRequest({
      interceptionId: event.params.interceptionId,
      rawResponse: btoa(headers.join('\r\n') + '\r\n\r\n' + body)
    });
  });

  dp.Network.onRequestWillBeSend(data => testRunner.log(data));
  dp.Emulation.onVirtualTimeBudgetExpired(_ => testRunner.completeTest());

  await dp.Emulation.setVirtualTimePolicy({policy: 'pause'});
  await dp.Emulation.setVirtualTimePolicy({
      policy: 'pauseIfNetworkFetchesPending', budget: 5000,
      waitForNavigation: true});
  dp.Page.navigate({url: 'http://test.com/index.html'});
})
