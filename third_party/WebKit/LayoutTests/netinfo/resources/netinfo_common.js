window.jsTestIsAsync = true;

var connection = navigator.connection;
var initialType = "bluetooth";
var initialDownlinkMax = 1.0;
var newConnectionType = "ethernet";
var newDownlinkMax = 2.0;
var initialEffectiveType = "3g";
var initialRtt = 50.0;
// Up to 10% noise  may be added to rtt and downlink. Use 11% as the buffer
// below to include any mismatches due to floating point calculations.
// Add 50 (bucket size used) to account for the cases when the sample may spill
// over to the next bucket due to the added noise of 10%. For example, if sample
// is 300, after adding noise, it may become 330, and after rounding off, it
// would spill over to the next bucket of 350.
var initialRttMaxNoise = initialRtt * 0.11 + 50;
var initialDownlink = 5.0;
var initialDownlinkMaxNoise = initialDownlink * 0.11 + 50;
var newEffectiveType = "4g";
var newRtt = 50.0;
var newRttMaxNoise = newRtt * 0.11 + 50;
var newDownlink = 10.0;
var newDownlinkMaxNoise = newDownlink * 0.11 + 50;

// Suppress connection messages information from the host.
if (window.internals) {
    internals.setNetworkConnectionInfoOverride(true, initialType,
        initialEffectiveType, initialRtt, initialDownlinkMax);

    // Reset the state of the singleton network state notifier.
    window.addEventListener('beforeunload', function() {
        internals.clearNetworkConnectionInfoOverride();
    }, false);
}

function isTypeOnline(type) {
    return type != 'none';
}

function verifyOnChangeMessage(message, type, downlinkMax, effectiveType, rtt, downlink) {
    var parsed = message.toString().split(",");
    if(parsed[0] != type)
        testFailed("type mismatch");
    if(parsed[2] != effectiveType)
        testFailed("effectiveType mismatch");
    if(parsed[3] != rtt)
        testFailed("rtt mismatch");
    if(parsed[4] != downlink)
        testFailed("downlink mismatch");
}