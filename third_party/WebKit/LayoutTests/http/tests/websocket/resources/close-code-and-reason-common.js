// Common code for both the plain and Worker variants of close-code-and-reason.

var test;
var closeEvent;
var codeNormalClosure = 1000;
var codeNoStatusRcvd = 1005;
var codeAbnormalClosure = 1006;
var emptyString = "";

function closeDuringOpen()
{
  var ws = new WebSocket("ws://127.0.0.1:8880/echo");

  ws.onopen = function(event)
  {
    testFailed("ws.onopen() was called. (message = \"" + event.data + "\")");
  };

  ws.onclose = function(event)
  {
    debug("ws.onclose() was called.");
    closeEvent = event;
    shouldBeFalse("closeEvent.wasClean");
    shouldBe("closeEvent.code", "codeAbnormalClosure");
    shouldBe("closeEvent.reason", "emptyString");
  };

  ws.close();
}

var testId = 0;
var testNum = 9;
var sendData = [
    "-", // request close frame without code and reason
    "--", // request close frame with invalid body which size is 1
    "1000 ok",  // request close frame with code 1000 and reason
    "1005 foo",  // request close frame with forbidden code 1005 and reason
    "1006 bar",  // request close frame with forbidden code 1006 and reason
    "1015 baz",  // request close frame with forbidden code 1015 and reason
    "0 good bye", // request close frame with specified code and reason
    "10 good bye", // request close frame with specified code and reason
    "65535 good bye", // request close frame with specified code and reason
];
var expectedCode = [
    codeNoStatusRcvd,
    codeAbnormalClosure,
    codeNormalClosure,
    codeAbnormalClosure,
    codeAbnormalClosure,
    codeAbnormalClosure,
    0,
    10,
    65535,
];
var expectedReason = [
    "''",
    "''",
    "'ok'",
    "''",
    "''",
    "''",
    "'good bye'",
    "'good bye'",
    "'good bye'",
];
var expectedWasClean = [
    true,
    false,
    true,
    false,
    false,
    false,
    true,
    true,
    true,
];

WebSocketTest = function()
{
    this.ws = new WebSocket("ws://127.0.0.1:8880/close-code-and-reason");
    this.ws.onopen = this.onopen;
    this.ws.onmessage = this.onmessage;
    this.ws.onclose = this.onclose.bind(this);
    this.timeoutID = setTimeout(this.ontimeout.bind(this), 400);
};

WebSocketTest.prototype.onopen = function()
{
    debug("WebSocketTest.onopen() was called with testId = " + testId + ".");
    this.send(sendData[testId]);
};

WebSocketTest.prototype.onmessage = function(event)
{
    testFailed("WebSocketTest.onmessage() was called. (message = \"" + event.data + "\")");
};

WebSocketTest.prototype.onclose = function(event)
{
    closeEvent = event;
    debug("WebSocketTest.onclose() was called with testId = " + testId + ".");

    shouldEvaluateTo("closeEvent.wasClean", expectedWasClean[testId]);
    shouldEvaluateTo("closeEvent.code", expectedCode[testId]);
    shouldEvaluateTo("closeEvent.reason", expectedReason[testId]);

    // Test that the attributes of the CloseEvent are readonly.
    closeEvent.code = 0;
    closeEvent.reason = "readonly";
    closeEvent.wasClean = !closeEvent.wasClean;
    shouldEvaluateTo("closeEvent.wasClean", expectedWasClean[testId]);
    shouldEvaluateTo("closeEvent.code", expectedCode[testId]);
    shouldEvaluateTo("closeEvent.reason", expectedReason[testId]);

    clearTimeout(this.timeoutID);
    this.ws = null;
    testId++;
    if (testId < testNum)
        test = new WebSocketTest();
    else
        finishJSTest();
};

WebSocketTest.prototype.ontimeout = function()
{
    testFailed("WebSocketTest.ontimeout() was called. (testId = " + testId + ")");
    // Ensure that none of the WebSocket handlers run after finishJSTest().
    var ignoreEvent = function(event) {};
    this.ws.onopen = ignoreEvent;
    this.ws.onmessage = ignoreEvent;
    this.ws.onclose = ignoreEvent;
    this.ws = null;

    finishJSTest();
};

function testCloseCodeAndReason()
{
    closeDuringOpen();
    test = new WebSocketTest();
}
