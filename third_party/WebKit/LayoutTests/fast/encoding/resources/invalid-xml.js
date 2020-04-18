// test 70: XML encoding test
// q.v. XML 1.0, section 4.3.3 Character Encoding in Entities
// this only tests one of a large number of conditions that should cause fatal errors

function runEncodingTest(event)
{
    debug("Testing: " + encodingTests[currentTest-1]);
    shouldBeEqualToString("iframe.contentDocument.documentElement.tagName", "root");
    shouldBeTrue("iframe.contentDocument.documentElement.getElementsByTagName('test').length < 1");
    setTimeout(runNextTest, 0);
}

var currentTest = 0;
var encodingTests = [
    "invalid-xml-utf8.xml",
    "invalid-xml-utf16.xml",
    "invalid-xml-shift-jis.xml",
    "invalid-xml-x-mac-thai.xml",
];

function runNextTest()
{
    if (currentTest >= encodingTests.length) {
        iframe.parentNode.removeChild(iframe);
        finishJSTest();
        return;
    }
    iframe.src = "resources/" + encodingTests[currentTest++];
}

window.jsTestIsAsync = true;
if (window.testRunner)
    testRunner.waitUntilDone();

var iframe = document.createElement("iframe");
document.body.appendChild(iframe);
iframe.onload = runEncodingTest;
runNextTest();
