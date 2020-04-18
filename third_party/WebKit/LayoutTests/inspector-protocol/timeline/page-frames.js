(async function(testRunner) {
  var {page, session, dp} = await testRunner.startHTML(`
    <iframe src='data:text/html,<script>window.foo = 42</script>' name='frame0'></iframe>
  `, 'Tests certain trace events in iframes.');

  function performActions() {
    var frame1 = document.createElement('iframe');
    frame1.name = 'Frame No. 1';
    document.body.appendChild(frame1);
    frame1.contentWindow.document.write('console.log("frame2")');

    var frame2 = document.createElement('iframe');
    frame2.src = 'blank.html';
    document.body.appendChild(frame2);

    return new Promise(fulfill => { frame2.addEventListener('load', fulfill, false) });
  }

  var TracingHelper = await testRunner.loadScript('../resources/tracing-test.js');
  var tracingHelper = new TracingHelper(testRunner, session);
  var data = await tracingHelper.invokeAsyncWithTracing(performActions);

  testRunner.log('Frames in TracingStartedInPage');
  var tracingStarted = tracingHelper.findEvent('TracingStartedInPage', 'I');
  for (var frame of tracingStarted.args['data']['frames'] || [])
    dumpFrame(frame);

  testRunner.log('Frames in CommitLoad events');
  var commitLoads = tracingHelper.findEvents('CommitLoad', 'X');
  for (var event of commitLoads)
    dumpFrame(event.args['data']);
  testRunner.completeTest();

  function dumpFrame(frame) {
    var url = frame.url.replace(/.*\/(([^/]*\/){2}[^/]*$)/, '$1');
    testRunner.log(`url: ${url} name: ${frame.name} parent: ${typeof frame.parent} nodeId: ${typeof frame.nodeId}`);
  }
})
