self.addEventListener('message', e => {
    e.ports[0].postMessage({
        "origin": self.location.origin,
        "addressSpace": self.addressSpace
    });
    self.registration.active.postMessage({
        "origin": self.location.origin,
        "addressSpace": self.addressSpace
    });
});
