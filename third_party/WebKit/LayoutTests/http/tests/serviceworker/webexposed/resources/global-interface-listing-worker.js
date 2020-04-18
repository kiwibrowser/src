// Avoid polluting the global scope.
(function(global_object) {

  // Save the list of property names of the global object before loading other scripts.
  var global_property_names = Object.getOwnPropertyNames(global_object);

  importScripts('/js-test-resources/global-interface-listing.js');

  var globals = [];

  globalInterfaceListing(global_object,
                         global_property_names,
                         false,
                         string => globals.push(string));

  self.addEventListener('message', function(event) {
    event.ports[0].postMessage({ result: globals });
  });

})(this);
