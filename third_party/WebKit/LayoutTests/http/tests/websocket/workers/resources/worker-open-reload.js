new WebSocket('ws://localhost:8880/workers/resources/echo').onopen = function (evt) {
    postMessage("opened");
};
