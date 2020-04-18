(async function(testRunner) {
  var {page, session, dp} = await testRunner.startHTML(`
    <style>
    .my-class {
        min-width: 100px;
        background-color: red;
    }
    </style>
    <div id='myDiv'>DIV</div>
  `, 'Tests trace events for layout.');

  function performActions() {
    var div = document.querySelector('#myDiv');
    div.classList.add('my-class');
    div.offsetWidth;
    return Promise.resolve();
  }

  var TracingHelper = await testRunner.loadScript('../resources/tracing-test.js');
  var tracingHelper = new TracingHelper(testRunner, session);
  await tracingHelper.invokeAsyncWithTracing(performActions);

  var schedRecalc = tracingHelper.findEvent('ScheduleStyleRecalculation', 'I');
  var recalcBegin = tracingHelper.findEvent('UpdateLayoutTree', 'B');
  var recalcEnd = tracingHelper.findEvent('UpdateLayoutTree', 'E');
  testRunner.log('UpdateLayoutTree frames match: ' + (schedRecalc.args.data.frame === recalcBegin.args.beginData.frame));
  testRunner.log('UpdateLayoutTree elementCount > 0: ' + (recalcEnd.args.elementCount > 0));

  var invalidate = tracingHelper.findEvent('InvalidateLayout', 'I');
  var layoutBegin = tracingHelper.findEvent('Layout', 'B');
  var layoutEnd = tracingHelper.findEvent('Layout', 'E');

  testRunner.log('InvalidateLayout frames match: ' + (recalcBegin.args.beginData.frame === invalidate.args.data.frame));

  var beginData = layoutBegin.args.beginData;
  testRunner.log('Layout frames match: ' + (invalidate.args.data.frame === beginData.frame));
  testRunner.log('dirtyObjects > 0: ' + (beginData.dirtyObjects > 0));
  testRunner.log('totalObjects > 0: ' + (beginData.totalObjects > 0));

  var endData = layoutEnd.args.endData;
  testRunner.log('has rootNode id: ' + (endData.rootNode > 0));
  testRunner.log('has root quad: ' + !!endData.root);

  testRunner.log('SUCCESS: found all expected events.');
  testRunner.completeTest();
})
