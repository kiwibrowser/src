// Dedicated workers.
onmessage = function () {
    postMessage({
        "origin": self.location.origin,
        "addressSpace": self.addressSpace
    });
}

// Shared workers.
onconnect = function (e) {
    var port = e.ports[0];
    port.onmessage = function () {
        port.postMessage({
            "origin": self.location.origin,
            "addressSpace": self.addressSpace
        });
    }
}
