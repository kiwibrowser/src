(async function(testRunner) {
  var {page, session, dp} = await testRunner.startBlank(`Tests preview provided by Runtime.getProperties.`);

  function printResult(response) {
    for (var property of response.result.result) {
      if (!property.value || property.name === '__proto__')
        continue;
      if (property.value.preview)
        testRunner.log(property.name + ' : ' + JSON.stringify(property.value.preview, null, 4));
      else
        testRunner.log(property.name + ' : ' + property.value.description);
    }
  }

  var response = await dp.Runtime.evaluate({expression: `({p1: {a:1}, p2: {b:'foo', bb:'bar'}})`});
  printResult(await dp.Runtime.getProperties({objectId: response.result.result.objectId, ownProperties: true }));
  printResult(await dp.Runtime.getProperties({objectId: response.result.result.objectId, ownProperties: true, generatePreview: true }));
  testRunner.completeTest();
})
