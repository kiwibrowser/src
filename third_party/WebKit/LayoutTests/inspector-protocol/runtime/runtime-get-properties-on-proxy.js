(async function(testRunner) {
  var {page, session, dp} = await testRunner.startBlank(`Check that while Runtime.getProperties call on proxy object no user defined trap will be executed.`);

  function testFunction() {
    window.counter = 0;
    var handler = {
      get: function(target, name) {
        window.counter++;
        return Reflect.get.apply(this, arguments);
      },
      set: function(target, name) {
        window.counter++;
        return Reflect.set.apply(this, arguments);
      },
      getPrototypeOf: function(target) {
        window.counter++;
        return Reflect.getPrototypeOf.apply(this, arguments);
      },
      setPrototypeOf: function(target) {
        window.counter++;
        return Reflect.setPrototypeOf.apply(this, arguments);
      },
      isExtensible: function(target) {
        window.counter++;
        return Reflect.isExtensible.apply(this, arguments);
      },
      isExtensible: function(target) {
        window.counter++;
        return Reflect.isExtensible.apply(this, arguments);
      },
      isExtensible: function(target) {
        window.counter++;
        return Reflect.isExtensible.apply(this, arguments);
      },
      preventExtensions: function() {
        window.counter++;
        return Reflect.preventExtensions.apply(this, arguments);
      },
      getOwnPropertyDescriptor: function() {
        window.counter++;
        return Reflect.getOwnPropertyDescriptor.apply(this, arguments);
      },
      defineProperty: function() {
        window.counter++;
        return Reflect.defineProperty.apply(this, arguments);
      },
      has: function() {
        window.counter++;
        return Reflect.has.apply(this, arguments);
      },
      get: function() {
        window.counter++;
        return Reflect.get.apply(this, arguments);
      },
      set: function() {
        window.counter++;
        return Reflect.set.apply(this, arguments);
      },
      deleteProperty: function() {
        window.counter++;
        return Reflect.deleteProperty.apply(this, arguments);
      },
      ownKeys: function() {
        window.counter++;
        return Reflect.ownKeys.apply(this, arguments);
      },
      apply: function() {
        window.counter++;
        return Reflect.apply.apply(this, arguments);
      },
      construct: function() {
        window.counter++;
        return Reflect.construct.apply(this, arguments);
      }
    };
    return new Proxy({ a : 1}, handler);
  }

  var response = await dp.Runtime.evaluate({expression: '(' + testFunction.toString() + ')()'});
  await dp.Runtime.getProperties({ objectId: response.result.result.objectId, generatePreview: true });
  response = await dp.Runtime.evaluate({ expression: 'window.counter' });
  testRunner.log(response.result);
  testRunner.completeTest();
})
