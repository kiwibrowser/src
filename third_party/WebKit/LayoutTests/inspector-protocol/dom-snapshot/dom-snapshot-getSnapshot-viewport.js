(async function(testRunner) {
  var {page, session, dp} = await testRunner.startBlank('Tests DOMSnapshot.getSnapshot method on a mobile page.');
  var DeviceEmulator = await testRunner.loadScript('../resources/device-emulator.js');
  var deviceEmulator = new DeviceEmulator(testRunner, session);
  await deviceEmulator.emulate(600, 600, 1);

  // The viewport width is 300px, half the device width.
  await session.navigate('../resources/dom-snapshot-viewport.html');

  function stabilize(key, value) {
    var unstableKeys = ['documentURL', 'baseURL', 'frameId', 'backendNodeId'];
    if (unstableKeys.indexOf(key) !== -1)
      return '<' + typeof(value) + '>';
    if (typeof value === 'string' && value.indexOf('/dom-snapshot/') !== -1)
      value = '<value>';
    return value;
  }

  var whitelist = [];
  var response = await dp.DOMSnapshot.getSnapshot({'computedStyleWhitelist': whitelist});
  if (response.error)
    testRunner.log(response);
  else
    testRunner.log(JSON.stringify(response.result, stabilize, 2));
  testRunner.completeTest();
})
