(async function(testRunner) {
  var {page, session, dp} = await testRunner.startBlank(`Test Runtime.getProperties with different flag combinations.`);

  // A helper function that dumps object properties and internal properties in sorted order.
  function logGetPropertiesResult(title, protocolResult) {
    function hasGetterSetter(property, fieldName) {
      var v = property[fieldName];
      if (!v)
        return false;
      return v.type !== 'undefined';
    }

    testRunner.log('Properties of ' + title);
    var propertyArray = protocolResult.result;
    propertyArray.sort(NamedThingComparator);
    for (var p of propertyArray) {
      var v = p.value;
      var own = p.isOwn ? 'own' : 'inherited';
      if (v) {
        testRunner.log('  ' + p.name + ' ' + own + ' ' + v.type + ' ' + v.value);
      } else {
        testRunner.log('  ' + p.name + ' ' + own + ' no value' +
            (hasGetterSetter(p, 'get') ? ', getter' : '') + (hasGetterSetter(p, 'set') ? ', setter' : ''));
      }
    }
    var internalPropertyArray = protocolResult.internalProperties;
    if (internalPropertyArray) {
      testRunner.log('Internal properties');
      internalPropertyArray.sort(NamedThingComparator);
      for (var p of internalPropertyArray) {
        var v = p.value;
        testRunner.log('  ' + p.name + ' ' + v.type + ' ' + v.value);
      }
    }

    function NamedThingComparator(o1, o2) {
      return o1.name === o2.name ? 0 : (o1.name < o2.name ? -1 : 1);
    }
  }

  async function testExpression(title, expression, ownProperties, accessorPropertiesOnly) {
    var response = await dp.Runtime.evaluate({expression});
    var id = response.result.result.objectId;
    response = await dp.Runtime.getProperties({objectId: id, ownProperties, accessorPropertiesOnly});
    logGetPropertiesResult(title, response.result);
  }

  // 'Object5' section -- check properties of '5' wrapped as object  (has an internal property).
  // Create an wrapper object with additional property.
  await testExpression(
      'Object(5)',
      `(function(){var r = Object(5); r.foo = 'cat';return r;})()`,
      true);

  // 'Not own' section -- check all properties of the object, including ones from it prototype chain.
  // Create an wrapper object with additional property.
  await testExpression(
      'Not own properties',
      `({ a: 2, set b(_) {}, get b() {return 5;}, __proto__: { a: 3, c: 4, get d() {return 6;} }})`,
      false);

  // 'Accessors only' section -- check only accessor properties of the object.
  // Create an wrapper object with additional property.
  await testExpression(
      'Accessor only properties',
      `({ a: 2, set b(_) {}, get b() {return 5;}, c: 'c', set d(_){} })`,
      true,
      true);

  // 'Array' section -- check properties of an array.
  await testExpression(
      'array',
      `['red', 'green', 'blue']`,
      true);

  // 'Bound' section -- check properties of a bound function (has a bunch of internal properties).
  await testExpression(
      'Bound function',
      `Number.bind({}, 5)`,
      true);

  await testExpression(
      'Event with user defined property',
      `var e = document.createEvent('Event'); Object.defineProperty(e, 'eventPhase', { get: function() { return 239; } })`,
      true);

  testRunner.completeTest();
})
