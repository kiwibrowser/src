var _port = self;

// Returns whether the test is being ran in a dedicated or a shared worker. The
// testharness.js framework requires done() to be called when this is the case.
function isDedicatedOrSharedWorker()
{
    return self.importScripts && !self.scope;
}

// Shared Workers will receive their message port once the first page connects to it.
self.addEventListener('connect', function(event) {
    _port = event.ports[0];
});

var testRunner = {
    simulateWebNotificationClick: function(title, action_index)
    {
        if (_port)
            _port.postMessage({ type: 'simulateWebNotificationClick', title, action_index });
    }
};
