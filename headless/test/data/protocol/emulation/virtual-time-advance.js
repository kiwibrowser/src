// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

(async function(testRunner) {
  var {page, session, dp} = await testRunner.startBlank(`Tests that virtual time advances.`);
  await dp.Page.enable();
  await dp.Runtime.enable();

  dp.Emulation.onVirtualTimePaused(data =>
      testRunner.log(`Paused @ ${data.params.virtualTimeElapsed}ms`));
  dp.Emulation.onVirtualTimeAdvanced(data => {
    // Debug chrome schedules stray tasks that break this test.
    // Our numbers are round, so we prevent this flake 999 times of 1000.
    const time = data.params.virtualTimeElapsed;
    if (time !== Math.round(time))
      return;
    testRunner.log(`Advanced to ${time}ms`);
  });

  dp.Runtime.onConsoleAPICalled(data => {
    const text = data.params.args[0].value;
    testRunner.log(text);
    if (text === 'pass')
      testRunner.completeTest();
  });

  await dp.Emulation.setVirtualTimePolicy({policy: 'pause'});
  await dp.Emulation.setVirtualTimePolicy({
      policy: 'pauseIfNetworkFetchesPending', budget: 5000, waitForNavigation: true});
  dp.Page.navigate({url: testRunner.url('resources/virtual-time-advance.html')});
})

