// Avoid polluting the global scope.
(function(globalObject) {

  // Save the list of property names of the global object before loading other scripts.
  var propertyNamesInGlobal = Object.getOwnPropertyNames(globalObject);

  importScripts('../../resources/js-test.js');
  importScripts('../../resources/global-interface-listing.js');

  if (!self.postMessage) {
    // Shared worker.  Make postMessage send to the newest client, which in
    // our tests is the only client.

    // Store messages for sending until we have somewhere to send them.
    self.postMessage = function(message) {
      if (typeof self.pendingMessages === "undefined")
        self.pendingMessages = [];
      self.pendingMessages.push(message);
    };
    self.onconnect = function(event) {
      self.postMessage = function(message) {
        event.ports[0].postMessage(message);
      };
      // Offload any stored messages now that someone has connected to us.
      if (typeof self.pendingMessages === "undefined")
        return;
      while (self.pendingMessages.length)
        event.ports[0].postMessage(self.pendingMessages.shift());
    };
  }

  // Note that this test is not split into platform-specific
  // vs. platform-neutral versions like the non-worker global-interface-listing
  // tests are. This is because it should be unlikely that there would be an
  // interface that is both worker-specific and platform-specific. This can be
  // reconsidered in the future if that does become the case however.

  globalInterfaceListing(globalObject, propertyNamesInGlobal, false /* platformSpecific */, debug);

  finishJSTest();

})(this);
