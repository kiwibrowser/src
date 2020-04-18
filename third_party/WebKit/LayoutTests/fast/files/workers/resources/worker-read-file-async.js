importScripts("../../resources/read-common.js", "../../resources/read-file-test-cases.js", "worker-read-common.js");

onmessage = function(event) {
    var testFiles = event.data;
    log("Received files in worker");
    if (event.data[0] instanceof FileList) {
        log("Received a FileList");
        // The tests expects a name => File map; construct the
        // mapping from the FileList and the test names that
        // were posted (as a pair, using an Array.)
        var files = event.data[0];
        var names = event.data[1];
        var tests = {};
        for (var i = 0; i < files.length; i++)
            tests[names[i]] = files[i];

        testFiles = tests;
    }
    runNextTest(testFiles);
};

function isReadAsAsync()
{
    return true;
}
