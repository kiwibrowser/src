importScripts("/js-test-resources/js-test.js", "../../resources/get-request-header.js");

var challenge1;
var challenge2;

Promise.all([connectAndGetRequestHeader("sec-websocket-key"), connectAndGetRequestHeader("sec-websocket-key")]).then(function(values)
{
    challenge1 = values[0];
    challenge2 = values[1];
    shouldBeFalse('challenge1 === challenge2');
    if (challenge1 === challenge2)
        debug('challenge was ' + challenge1);
    finishJSTest();
}, finishAsFailed);
