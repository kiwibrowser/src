function finish() {
    if (self.importScripts)
        postMessage('done');
    else if (window.opener)
        window.opener.postMessage('done', '*');
}

var xhr = new XMLHttpRequest();
xhr.open("GET", "http://localhost:8000/xmlhttprequest/resources/echo-request-origin.php");
xhr.onload = function (e) {
    console.log("PASS: " + xhr.responseText);
    finish();
};
xhr.onerror = function (e) {
    console.log("FAIL: " + xhr.status);
    finish();
};
xhr.send();
