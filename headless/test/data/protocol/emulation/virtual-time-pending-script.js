// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

const htmlWithScript =
    `<script src="/large.js"></script>`;

// We want to pad |kLargeDotJS| out with some dummy code which is parsed
// asynchronously to make sure the virtual_time_pauser in PendingScript
// actually does something. We construct a large number of long unused
// function declarations which seems to trigger the desired code path.
const dummy = [];
for (let i = 0; i < 1024; ++i)
  dummy.push(`var i${i}=function(){return '${'A'.repeat(4096)}';}`);

const largeDotJS = `
(function() {
var setTitle = newTitle => document.title = newTitle;
${dummy.join('\n')}
setTitle('Test PASS');
})();`;

const server = new Map([
  ['http://test.com/index.html', htmlWithScript],
  ['http://test.com/large.js', largeDotJS]]);

(async function(testRunner) {
  var {page, session, dp} = await testRunner.startBlank(
      `Tests that pending script does not break virtual time.`);
  await dp.Network.enable();
  await dp.Network.setRequestInterception({ patterns: [{ urlPattern: '*' }] });
  dp.Network.onRequestIntercepted(event => {
    let body = server.get(event.params.request.url);
    dp.Network.continueInterceptedRequest({
      interceptionId: event.params.interceptionId,
      rawResponse: btoa(body)
    });
  });

  dp.Emulation.onVirtualTimeBudgetExpired(async data => {
    testRunner.log(await session.evaluate('document.title'));
    testRunner.completeTest();
  });

  await dp.Emulation.setVirtualTimePolicy({policy: 'pause'});
  await dp.Emulation.setVirtualTimePolicy({
      policy: 'pauseIfNetworkFetchesPending', budget: 5000,
      waitForNavigation: true});
  dp.Page.navigate({url: 'http://test.com/index.html'});
})
