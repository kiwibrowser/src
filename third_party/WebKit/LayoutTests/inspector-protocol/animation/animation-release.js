(async function(testRunner) {
  var {page, session, dp} = await testRunner.startHTML(`
    <div id='node' style='background-color: red; width: 100px'></div>
  `, 'Tests that the animation is correctly paused.');

  dp.Animation.enable();
  session.evaluate(`
    window.animation = node.animate([{ width: '100px' }, { width: '2000px' }], { duration: 0, fill: 'forwards' });
  `);

  var id = (await dp.Animation.onceAnimationStarted()).params.animation.id;
  testRunner.log('Animation started');
  var width = await session.evaluate('node.offsetWidth');
  testRunner.log('Box is animating: ' + (width != 100).toString());
  dp.Animation.setPaused({ animations: [ id ], paused: true });
  session.evaluate('animation.cancel()');
  width = await session.evaluate('node.offsetWidth');
  testRunner.log('Animation paused');
  testRunner.log('Box is animating: ' + (width != 100).toString());
  dp.Animation.releaseAnimations({ animations: [ id ] });
  width = await session.evaluate('node.offsetWidth');
  testRunner.log('Animation released');
  testRunner.log('Box is animating: ' + (width != 100).toString());
  testRunner.completeTest();
})
