onconnect = function(event) {
    function update() {
        onerror = undefined;
    }
    for (var i = 0; i < 8; ++i) {
        update();
    }
    event.ports[0].postMessage("'onerror' repeatedly updated ok.");
    event.ports[0].postMessage("DONE:");
};
