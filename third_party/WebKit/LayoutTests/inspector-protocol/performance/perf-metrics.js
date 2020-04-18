(async function(testRunner) {
  var {page, session, dp} = await testRunner.startBlank(
      'Test that page performance metrics are retrieved.');

  await dumpMetrics();
  await dp.Performance.enable();
  await dumpMetrics();
  const taskTime1 = (await retrieveMetrics()).get('TaskDuration');
  const taskTime2 = (await retrieveMetrics()).get('TaskDuration');
  if (taskTime1 >= taskTime2)
    testRunner.log(`Error: Metric TaskDuration should be monotonically increasing: ${taskTime1} ${taskTime2}.`);
  await dp.Performance.disable();
  await dumpMetrics();

  async function dumpMetrics() {
    const {result:{metrics}} = await dp.Performance.getMetrics();
    testRunner.log('Received metrics:');
    for (const metric of metrics)
      testRunner.log(`\t${metric.name}`);
    checkMetric('Documents');
    checkMetric('Nodes');

    function checkMetric(name) {
      const metric = metrics.find(metric => metric.name === name);
      if (metrics.length && !metric.value)
        testRunner.log(`Error: Metric ${name} has a bad value ${metric.value}`);
    }
  }

  testRunner.completeTest();

})
