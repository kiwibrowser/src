(async function(testRunner) {
  var {page, session, dp} = await testRunner.startHTML(`
    <div id='myDiv'>DIV</div>
  `, 'Tests trace events for timer firing.');

  function performActions() {
    var callback;
    var promise = new Promise((fulfill) => callback = fulfill);
    var timerId = setTimeout(function() {
      callback({timerId: timerId, timerId2: timerId2});
    }, 0);

    var timerId2 = setTimeout(function() { }, 0);
    clearTimeout(timerId2);
    return promise;
  }

  function hasTimerId(id, e) {
    return e.args.data.timerId === id;
  }

  var TracingHelper = await testRunner.loadScript('../resources/tracing-test.js');
  var tracingHelper = new TracingHelper(testRunner, session);
  var data = await tracingHelper.invokeAsyncWithTracing(performActions);

  var firedTimerId = data.timerId;
  var removedTimerId = data.timerId2;

  var installTimer1 = tracingHelper.findEvent('TimerInstall', 'I', hasTimerId.bind(this, firedTimerId));
  var installTimer2 = tracingHelper.findEvent('TimerInstall', 'I', hasTimerId.bind(this, removedTimerId));

  testRunner.log('TimerInstall has frame: ' + !!installTimer1.args.data.frame);
  testRunner.log('TimerInstall frames match: ' + (installTimer1.args.data.frame === installTimer2.args.data.frame));

  tracingHelper.findEvent('TimerRemove', 'I', hasTimerId.bind(this, removedTimerId));
  tracingHelper.findEvent('TimerFire', 'X', hasTimerId.bind(this, firedTimerId));

  testRunner.log('SUCCESS: found all expected events.');
  testRunner.completeTest();
})
