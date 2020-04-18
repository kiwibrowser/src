// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

const fooDotCom = `
    <script> console.log(document.location.href); </script>
    <iframe src='/a/'></iframe>`;
const fooDotComSlashA = `
    <script> console.log(document.location.href); </script>`;
const barDotCom = `
    <script> console.log(document.location.href); </script>
    <iframe src='/b/' id='frame_b'></iframe>
    <iframe src='/c/'></iframe>`;
const barDotComSlashB = `
    <script> console.log(document.location.href); </script>
    <iframe src='/d/'></iframe>`;
const barDotComSlashC = `
    <script> console.log(document.location.href); </script>`
const barDotComSlashD = `
    <script> console.log(document.location.href); </script>`
const barDotComSlashE = `
    <script> console.log(document.location.href); </script>
    <iframe src='/f/'></iframe>`
const barDotComSlashF = `
    <script> console.log(document.location.href); </script>`

const server = new Map([
  ['http://foo.com/', fooDotCom],
  ['http://foo.com/a/', fooDotComSlashA],
  ['http://bar.com/', barDotCom],
  ['http://bar.com/b/', barDotComSlashB],
  ['http://bar.com/c/', barDotComSlashC],
  ['http://bar.com/d/', barDotComSlashD],
  ['http://bar.com/e/', barDotComSlashE],
  ['http://bar.com/f/', barDotComSlashF]]);

(async function(testRunner) {
  var {page, session, dp} = await testRunner.startBlank(
      `Tests virtual time with history navigation.`);
  await dp.Network.enable();
  await dp.Network.setRequestInterception({ patterns: [{ urlPattern: '*' }] });
  dp.Network.onRequestIntercepted(event => {
    let body = server.get(event.params.request.url);
    dp.Network.continueInterceptedRequest({
      interceptionId: event.params.interceptionId,
      rawResponse: btoa(body)
    });
  });

  const testCommands = [
      `document.location.href = 'http://bar.com/'`,
      `document.getElementById('frame_b').src = '/e/'`,
      `history.back()`,
      `history.forward()`,
      `history.go(-1)`];

  dp.Emulation.onVirtualTimeBudgetExpired(async data => {
    if (!testCommands.length) {
      testRunner.completeTest();
      return;
    }
    const command = testCommands.shift();
    testRunner.log(command);
    await session.evaluate(command);
    await dp.Emulation.setVirtualTimePolicy({
        policy: 'pauseIfNetworkFetchesPending', budget: 5000});
  });

  await dp.Emulation.setVirtualTimePolicy({policy: 'pause'});
  await dp.Emulation.setVirtualTimePolicy({
      policy: 'pauseIfNetworkFetchesPending', budget: 5000,
      waitForNavigation: true});
  dp.Page.navigate({url: 'http://foo.com/'});
})
