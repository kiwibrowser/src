addEventListener('message', function(e) {
    self.postMessage(navigator.connection.type + ',' + navigator.connection.downlinkMax + ',' + navigator.connection.effectiveType + ',' + navigator.connection.rtt + ',' + navigator.connection.downlink);
}, false);

navigator.connection.addEventListener('change', function() {
    self.postMessage(navigator.connection.type + ',' + navigator.connection.downlinkMax + ',' + navigator.connection.effectiveType + ',' + navigator.connection.rtt + ',' + navigator.connection.downlink);
}, false);