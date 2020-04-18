(async function(testRunner) {
  var {page, session, dp} = await testRunner.startBlank('Tests DOM.getFrameOwner method.');
  await dp.Target.setDiscoverTargets({discover: true});
  await session.evaluate(`
    var iframe = document.createElement('iframe');
    iframe.id = 'outer_frame';
    iframe.src = 'data:text/html,<iframe id=inner_frame src="http://devtools.oopif.test:8000/resources/dummy.html">';
    document.body.appendChild(iframe);
  `);

  var frameId = (await dp.Target.onceTargetCreated()).params.targetInfo.targetId;
  await dp.DOM.enable();
  await dp.DOM.getDocument();
  var r = await dp.DOM.getFrameOwner({frameId});
  r = await dp.DOM.describeNode({nodeId: r.result.nodeId});
  testRunner.log(r.result.node.nodeName + ' ' + r.result.node.attributes);
  testRunner.completeTest();
})
