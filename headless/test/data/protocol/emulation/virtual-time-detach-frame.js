// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

(async function(testRunner) {
  var {page, session, dp} = await testRunner.startBlank(
      `Tests that detaching frame while issuing request doesn't break virtual time.`);
  await dp.Network.enable();
  await dp.Network.setRequestInterception({ patterns: [{ urlPattern: '*' }] });
  dp.Network.onRequestIntercepted(async event => {
    const url = event.params.request.url;
    testRunner.log(new URL(url).pathname);
    // Detach the iframe but leave the css resource fetch hanging.
    if (url.includes(`style.css`))
      await session.evaluate(`document.getElementById('iframe1').remove()`);
    dp.Network.continueInterceptedRequest({
      interceptionId: event.params.interceptionId,
    });
  });

  dp.Emulation.onVirtualTimeBudgetExpired(data => testRunner.completeTest());

  await dp.Emulation.setVirtualTimePolicy({policy: 'pause'});
  await dp.Emulation.setVirtualTimePolicy({
      policy: 'pauseIfNetworkFetchesPending', budget: 5000,
      waitForNavigation: true});
  dp.Page.navigate({url: testRunner.url('resources/virtual-time-detach-frame-index.html')});
})
