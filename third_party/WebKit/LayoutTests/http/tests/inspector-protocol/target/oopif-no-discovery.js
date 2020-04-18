(async function(testRunner) {
  var {page, session, dp} = await testRunner.startBlank(`Tests oopif discovery.`);

  await dp.Page.enable();
  dp.Page.navigate({url: testRunner.url('../resources/site_per_process_main.html')});
  await dp.Page.onceLoadEventFired();

  testRunner.log('Enabling auto-discovery...');
  dp.Target.setAutoAttach({autoAttach: true, waitForDebuggerOnStart: false});

  let attachedEvent = (await dp.Target.onceAttachedToTarget()).params;
  testRunner.log('Got auto-attached.');
  let frameId = attachedEvent.targetInfo.targetId;

  testRunner.log('Navigating to in-process iframe...');
  let navigatePromise = dp.Page.navigate({frameId, url: testRunner.url('../resources/iframe.html')});
  let detachedPromise = dp.Target.onceDetachedFromTarget();
  await Promise.all([navigatePromise, detachedPromise]);

  let detachedEvent = (await detachedPromise).params;
  testRunner.log('Session id should match: ' + (attachedEvent.sessionId === detachedEvent.sessionId));
  testRunner.log('Target id should match: ' + (attachedEvent.targetInfo.targetId === detachedEvent.targetId));

  testRunner.log('Navigating back to out-of-process iframe...');

  dp.Page.navigate({frameId, url: 'http://devtools.oopif.test:8000/inspector-protocol/resources/iframe.html'});

  let attachedEvent2 = (await dp.Target.onceAttachedToTarget()).params;
  testRunner.log('Target ids should match: ' + (attachedEvent.targetInfo.targetId === attachedEvent2.targetInfo.targetId));

  dp.Target.setAutoAttach({autoAttach: false, waitForDebuggerOnStart: false});
  let detachedEvent2 = (await dp.Target.onceDetachedFromTarget()).params;
  testRunner.log('Session id should match: ' + (attachedEvent2.sessionId === detachedEvent2.sessionId));
  testRunner.log('Target id should match: ' + (attachedEvent2.targetInfo.targetId === detachedEvent2.targetId));

  testRunner.completeTest();
})
