if (self.importScripts)
    importScripts("/js-test-resources/js-test.js");

self.jsTestIsAsync = true;

description("Test cross-origin XHRs to CORS-unsupported protocol schemes in the URL.");

var xhr;
var errorEvent;
function issueRequest(url, contentType)
{
    xhr = new XMLHttpRequest();
    // async = false
    xhr.open('POST', url, false);
    xhr.onerror = () => testFailed("onerror callback should not be called.");
    // Assumed a Content-Type that turns it into a non-simple CORS request.
    if (contentType)
        xhr.setRequestHeader('Content-Type', contentType);
    try {
        xhr.send();
    } catch(e) {
        errorEvent = e;
        shouldBeEqualToString("errorEvent.name", "NetworkError");
    }

    xhr = new XMLHttpRequest();
    // async = true
    xhr.open('POST', url, true);
    xhr.onerror = function (a) {
        errorEvent = a;
        shouldBeEqualToString("errorEvent.type", "error");
        setTimeout(runTest, 0);
    };
    // Assumed a Content-Type that turns it into a non-simple CORS request.
    if (contentType)
        xhr.setRequestHeader('Content-Type', contentType);

    shouldNotThrow('xhr.send()');
}

var withContentType = true;
var tests = [ 'ftp://127.0.0.1',
              'localhost:8080/',
              'tel:1234' ];

function runTest()
{
    if (!tests.length && withContentType) {
        finishJSTest();
        return;
    }
    withContentType = !withContentType;
    if (!withContentType)
        issueRequest(tests[0]);
    else
        issueRequest(tests.shift(), 'application/json');
}
runTest();
