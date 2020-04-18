(async function(testRunner) {
  var {page, session, dp} = await testRunner.startBlank(
      `Test that a detached iframe will be marked as detached.`);
  dp.Page.enable();
  session.evaluate(`
    var frame = document.createElement('iframe');
    frame.src = 'data:text/html,<script>class Leak{}; var x = new Leak();<'+
                      '/script>';
    document.body.appendChild(frame);
    frame.addEventListener("load", function() {
      var iframeWindow  = this.contentWindow;
      function retainingListener() {
        // This is leaking the iframe.
        console.log(iframeWindow);
      }
      document.body.addEventListener('click', retainingListener, true);
      document.body.removeChild(frame);
    });
  `);
  await dp.Page.onceFrameStoppedLoading();
  var Helper = await testRunner.loadScript('resources/heap-snapshot-common.js');
  var helper = await Helper(testRunner, session);
  var snapshot = await helper.takeHeapSnapshot();
  var node;
  for (var it = snapshot._allNodes(); it.hasNext(); it.next()) {
    if (it.node.className() === 'Leak') {
      node = it.node;
      break;
    }
  }
  if (node)
    testRunner.log('SUCCESS: found ' + node.name());
  else
    return testRunner.fail('cannot find leaking node');

  var retainers = helper.firstRetainingPath(node).map(node => node.name());

  if (retainers.includes('Detached Window'))
    testRunner.log('SUCCESS: detached iframe in retaining path');
  else
    return testRunner.fail('no detached iframe in retaining path');
  testRunner.completeTest();
})