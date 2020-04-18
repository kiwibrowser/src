// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

(async function(testRunner) {
  var {page, session, dp} = await testRunner.startBlank(
      `Tests that virtual time advances 10ms on every navigation.`);
  await dp.Network.enable();

  let resourceCounter = 0;
  dp.Network.onRequestWillBeSent(() => { resourceCounter++ });
  dp.Emulation.onVirtualTimeBudgetExpired(data => {
    testRunner.log('Resources loaded: ' + resourceCounter);
    testRunner.completeTest();
  });

  await dp.Emulation.setVirtualTimePolicy({policy: 'pause'});
  await dp.Emulation.setVirtualTimePolicy({
      policy: 'pauseIfNetworkFetchesPending',
      budget: 5000, waitForNavigation: true,
      maxVirtualTimeTaskStarvationCount: 1000000});  // starvation prevents flakes
  dp.Page.navigate({url: testRunner.url('resources/virtual-time-error-loop.html')});
})
