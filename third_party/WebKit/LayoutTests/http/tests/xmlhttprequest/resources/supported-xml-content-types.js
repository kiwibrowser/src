function getXMLOfType(testcase)
{
    var request = new XMLHttpRequest();
    function failure()
    {
        testFailed(testcase.xmlType)
        runNextTest();
    }
    function checkResult()
    {
        var statusText = "";
        if (request.responseXML) {
            var typeElement = request.responseXML.firstChild;
            if (testcase.expectPass && typeElement) {
                if (typeElement.textContent !== testcase.xmlType)
                    statusText = "Incorrect content: " + typeElement.textContent;
            } else {
                statusText = "Document type: " + request.getResponseHeader("Content-type");
            }
        } else if (testcase.expectPass) {
            statusText = "Response type: " + request.getResponseHeader("Content-type");
        }
        if (statusText)
            testFailed(testcase.xmlType + " -- " + statusText + "; responseXML: " + new XMLSerializer().serializeToString(request.responseXML));
        else
            testPassed(testcase.xmlType);

        runNextTest();
    }

    var escapedType = escape(testcase.xmlType).replace(/\+/g, "^^PLUS^^"); // Perl CGI module seems replace + with a space
    request.open("GET", "supported-xml-content-types.cgi?type=" + escapedType, true);
    request.onerror = checkResult;
    request.onload = checkResult;
    request.send(null);
}

var tests = [];

function testXMLType(type, expected)
{
    tests.push({xmlType: type, expectPass: expected});
}

function runNextTest()
{
    if (tests.length)
        getXMLOfType(tests.shift());
    else
        finishJSTest();
}
