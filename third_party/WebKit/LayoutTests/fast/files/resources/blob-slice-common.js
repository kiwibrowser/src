var blob, file; // Populated by runTests() in individual tests.
var sliceParams = []; // Populated by individual tests.
var testIndex = 0;

function testSlicing(start, end, expectedResult, blob, doneCallback)
{
    var blobClass = blob.constructor.name;
    var sliced;
    var reader = new FileReader();
    var message = ".slice";
    if (start === null && end === null) {
        message += "()";
        sliced = blob.slice();
    } else if (end == undefined) {
        message += "(" + start + ")";
        sliced = blob.slice(start);
    } else {
        message += "(" + start + ", " + end + ")";
        sliced = blob.slice(start, end);
    }
    reader.onloadend = function(event) {
        var error = event.target.error;
        if (error) {
            testFailed("File read error " + message + error);
            doneCallback();
            return;
        }
        var blobContentsVar = blobClass.toLowerCase() + "Contents";
        window[blobContentsVar] = event.target.result;
        shouldBeEqualToString(blobContentsVar, expectedResult);
        doneCallback();
    };
    debug(blobClass + " " + message);
    reader.readAsText(sliced);
}

function runNextTest()
{
    if (testIndex >= sliceTestCases.length) {
        finishJSTest();
        return;
    }

    var testCase = sliceTestCases[testIndex];
    testIndex++;
    testSlicing(testCase[0], testCase[1], testCase[2], blob, function() {
        testSlicing(testCase[0], testCase[1], testCase[2], file, runNextTest);
    });
}
