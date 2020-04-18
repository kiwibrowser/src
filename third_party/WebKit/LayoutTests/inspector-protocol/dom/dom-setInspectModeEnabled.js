(async function(testRunner) {
  var {page, session, dp} = await testRunner.startHTML(`
    <div style="position:absolute;top:100;left:100;width:100;height:100;background:black"></div>
  `, 'Tests inspect mode.');
  var NodeTracker = await testRunner.loadScript('../resources/node-tracker.js');
  var nodeTracker = new NodeTracker(dp);
  dp.DOM.enable();
  dp.Overlay.enable();
  var message = await dp.Overlay.setInspectMode({ mode: 'searchForNode', highlightConfig: {} });
  if (message.error) {
    testRunner.die(message.error.message);
    return;
  }
  dp.Input.dispatchMouseEvent({type: 'mouseMoved', button: 'left', clickCount: 1, x: 150, y: 150 });
  dp.Input.dispatchMouseEvent({type: 'mousePressed', button: 'left', clickCount: 1, x: 150, y: 150 });
  dp.Input.dispatchMouseEvent({type: 'mouseReleased', button: 'left', clickCount: 1, x: 150, y: 150 });

  var message = await dp.Overlay.onceInspectNodeRequested();
  message = await dp.DOM.pushNodesByBackendIdsToFrontend({backendNodeIds: [message.params.backendNodeId]});
  testRunner.log('DOM.inspectNodeRequested: ' + nodeTracker.nodeForId(message.result.nodeIds[0]).localName);
  testRunner.completeTest();
})

