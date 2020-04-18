description("Test verifies that FormData is sent correctly when using " +
            "<a href='http://www.w3.org/TR/XMLHttpRequest/#the-send-method'>XMLHttpRequest asynchronously.</a>");

var xhrFormDataTestUrl = '/xmlhttprequest/resources/multipart-post-echo.php';
var xhrFormDataTestCases = [{
    data: { string: 'string value' },
    result: "string=string value"
}, {
    data: { bareBlob: new Blob(['blob-value']) },
    result: 'bareBlob=blob:application/octet-stream:blob-value'
}, {
    data: { mimeBlob: new Blob(['blob-value'], { type: 'text/html' }) },
    result: 'mimeBlob=blob:text/html:blob-value'
}, {
    data: {
        namedBlob: {
            value: new Blob(['blob-value']),
            fileName: 'blob-file.txt'
        }
    },
    result: 'namedBlob=blob-file.txt:application/octet-stream:blob-value'
}, {
    data: { bareFile: new File(['file-value'], 'file-name.txt') },
    result: 'bareFile=file-name.txt:application/octet-stream:file-value'
}, {
    data: {
        mimeFile: new File(['file-value'], 'file-name.html', { type: 'text/html' })
    },
    result: 'mimeFile=file-name.html:text/html:file-value'
}, {
    data: {
        renamedFile: {
            value: new File(['file-value'], 'file-name.html', { type: 'text/html' }),
            fileName: 'file-name-override.html'
        }
    },
    result: 'renamedFile=file-name-override.html:text/html:file-value'
}];

var xhr;
var expectedMimeType;

self.jsTestIsAsync = true;
var asyncTestCase = 0;

function runNextAsyncTest() {
    asyncTestCase++;
    runAsyncTests();
}

function reportResult(e) {
    var testCase = xhrFormDataTestCases[asyncTestCase];
    if (xhr.status === 200) {
        echoResult = xhr.response;
        shouldBeEqualToString("echoResult", testCase.result);
    } else {
        testFailed("Unknown error");
    }

    runNextAsyncTest();
}

function runAsyncTests() {
    if (asyncTestCase >= xhrFormDataTestCases.length) {
        finishJSTest();
        return;
    }

    var testCase = xhrFormDataTestCases[asyncTestCase];
    var formData = new FormData();
    if (testCase.beforeConstruct)
        testCase.beforeConstruct(testCase);
    for (var fieldName in testCase.data) {
        fieldValue = testCase.data[fieldName];
        if (fieldValue.constructor === Object)
            formData.append(fieldName, fieldValue.value, fieldValue.fileName);
        else
            formData.append(fieldName, fieldValue);
    }

    xhr = new XMLHttpRequest();
    xhr.onloadend = reportResult;
    xhr.open("POST", xhrFormDataTestUrl, true);
    if (testCase.beforeSend)
        testCase.beforeSend(testCase);
    xhr.send(formData);
}

runAsyncTests();
