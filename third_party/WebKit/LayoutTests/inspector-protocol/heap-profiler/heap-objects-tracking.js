(async function(testRunner) {
  var {page, session, dp} = await testRunner.startBlank('Test that heap tracking actually reports data fragments.');

  await session.evaluate(`
    var junkArray = new Array(1000);
    function junkGenerator() {
      for (var i = 0; i < junkArray.length; ++i)
        junkArray[i] = '42 ' + i;
      window.junkArray = junkArray;
    }
    setInterval(junkGenerator, 0)
  `);

  var lastSeenObjectIdEventCount = 0;
  var heapStatsUpdateEventCount = 0;
  var fragments = [];

  dp.HeapProfiler.onLastSeenObjectId(async messageObject => {
    ++lastSeenObjectIdEventCount;
    if (lastSeenObjectIdEventCount <= 2) {
      var params = messageObject['params'];
      testRunner.log('HeapProfiler.lastSeenObjectId has params: ' + !!params);
      testRunner.log('HeapProfiler.lastSeenObjectId has params.lastSeenObjectId: ' + !!params.lastSeenObjectId);
      testRunner.log('HeapProfiler.lastSeenObjectId has timestamp: ' + !!params.timestamp);
      testRunner.log('A heap stats fragment did arrive before HeapProfiler.lastSeenObjectId: ' + !!fragments.length);
      testRunner.log('');
    }
    if (lastSeenObjectIdEventCount === 2) {
      // Wait for two updates and then stop tracing.
      await dp.HeapProfiler.stopTrackingHeapObjects();
      testRunner.log('Number of heapStatsUpdate events >= numbrt of lastSeenObjectId events: ' + (heapStatsUpdateEventCount >= lastSeenObjectIdEventCount));
      testRunner.log('At least 2 lastSeenObjectId arrived: ' + (lastSeenObjectIdEventCount >= 2));
      testRunner.log('SUCCESS: tracking stopped');
      testRunner.completeTest();
    }
  });

  dp.HeapProfiler.onHeapStatsUpdate(messageObject => {
    ++heapStatsUpdateEventCount;
    var params = messageObject['params'];
    if (heapStatsUpdateEventCount <= 2)
      testRunner.log('HeapProfiler.heapStatsUpdate has params: ' + !!params);
    var statsUpdate = params.statsUpdate;
    if (heapStatsUpdateEventCount <= 2) {
      testRunner.log('HeapProfiler.heapStatsUpdate has statsUpdate: ' + !!statsUpdate);
      testRunner.log('statsUpdate length is not zero: ' + !!statsUpdate.length);
      testRunner.log('statsUpdate length is a multiple of three: ' + !(statsUpdate.length % 3));
      testRunner.log('statsUpdate: first fragmentIndex in first update: ' + statsUpdate[0]);
      testRunner.log('statsUpdate: total count of objects is not zero: ' + !!statsUpdate[1]);
      testRunner.log('statsUpdate: total size of objects is not zero: ' + !!statsUpdate[2]);
      testRunner.log('');
    }
    fragments.push(statsUpdate);
  });

  await dp.HeapProfiler.startTrackingHeapObjects();
  testRunner.log('SUCCESS: tracking started');
})
