if (self.importScripts)
    importScripts("/js-test-resources/js-test.js");

description("Test EventSource states for different status codes.");

self.jsTestIsAsync = true;

function arrayCompare(a1, a2) {
    if (a1.length != a2.length)
        return false;
    for (var i = 0; i < a1.length; i++)
        if (a1[i] != a2[i])
            return false;
    return true;
}

var stateNames = ["CONNECTING", "OPEN", "CLOSED"];
for (var i in stateNames)
    eval("var " + stateNames[i] + " = " + i);

var tests = [{"code": 200, "expectedStates": [CONNECTING, OPEN, OPEN, CONNECTING, CLOSED]},
             {"code": 204, "expectedStates": [CONNECTING,,, CLOSED, CLOSED]},
             {"code": 205, "expectedStates": [CONNECTING,,, CLOSED, CLOSED]},
             {"code": 202, "expectedStates": [CONNECTING,,, CLOSED, CLOSED]}, // other 2xx
             {"code": 301, "expectedStates": [CONNECTING, OPEN, OPEN, CONNECTING, CLOSED]},
             {"code": 302, "expectedStates": [CONNECTING, OPEN, OPEN, CONNECTING, CLOSED]},
             {"code": 303, "expectedStates": [CONNECTING, OPEN, OPEN, CONNECTING, CLOSED]},
             {"code": 307, "expectedStates": [CONNECTING, OPEN, OPEN, CONNECTING, CLOSED]},
             {"code": 404, "expectedStates": [CONNECTING,,, CLOSED, CLOSED]}]; // any other
var count = 0;

var es;
var states = [];

function runTest() {
    if (count >= tests.length) {
        debug("DONE");
        finishJSTest();
        return;
    }

    states = [];
    es = new EventSource("/eventsource/resources/status-codes.php?status-code=" + tests[count].code);
    states[0] = es.readyState;

    es.onopen = function () {
        states[1] = es.readyState;
    };

    es.onmessage = function (evt) {
        states[2] = es.readyState;
    };

    es.onerror = function () {
        states[3] = es.readyState;
        es.close();
        states[4] = es.readyState;

        shouldBeTrue("arrayCompare(states, tests[count].expectedStates)");
        result = "status code " + tests[count].code + " resulted in states ";
        for (var i in states)
            result += (i != 0 ? ", " : "") + stateNames[states[i]];
        testPassed(result);

        count++;
        setTimeout(runTest, 0);
    };
}
runTest();
