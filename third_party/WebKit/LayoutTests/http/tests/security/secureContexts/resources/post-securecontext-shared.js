self.onconnect = e => {
    e.ports[0].postMessage({ "isSecureContext": self.isSecureContext });
};
