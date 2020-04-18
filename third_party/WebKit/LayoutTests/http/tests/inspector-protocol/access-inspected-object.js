(async function(testRunner) {
  var {page, session, dp} = await testRunner.startBlank(
      `Test that code evaluated in the main frame cannot access $0 that resolves into a node in a frame from a different domain. Bug 105423.`);

  await session.evaluateAsync(`
    var frame = document.createElement('iframe');
    frame.id = 'myframe';
    frame.src = 'http://localhost:8000/inspector-protocol/resources/test-page.html';
    document.body.appendChild(frame);
    new Promise(f => frame.onload = f)
  `);

  var documentNodeId = (await dp.DOM.getDocument()).result.root.nodeId;

  dp.DOM.onSetChildNodes(onSetChildNodes);
  async function onSetChildNodes(messageObject) {
    var node = messageObject.params.nodes[0];
    if (!node || node.nodeName !== 'IFRAME')
      return;
    dp.DOM.offSetChildNodes(onSetChildNodes);
    var response = await dp.DOM.querySelector({nodeId: node.contentDocument.nodeId, selector: 'div#rootDiv'});
    await dp.DOM.setInspectedNode({nodeId: response.result.nodeId});
    testRunner.log(await dp.Runtime.evaluate({expression: '$0', includeCommandLineAPI: true}));
    testRunner.completeTest();
  }

  var response = await dp.DOM.querySelector({nodeId: documentNodeId, selector: 'iframe#myframe'});
  if (response.error) {
    testRunner.fail(response.error);
    testRunner.completeTest();
  }
})
