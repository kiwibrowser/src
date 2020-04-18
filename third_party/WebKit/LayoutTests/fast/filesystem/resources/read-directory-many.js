description("DirectoryReader.readEntries() test with many entries.");
var fileSystem = null;
var reader = null;
var numFilesExpected = 150;
var numDirectoriesExpected = 150;
var resultEntries = [];

function endTest()
{
    removeAllInDirectory(fileSystem.root, finishJSTest, errorCallback);
}

function errorCallback(error)
{
    debug("Error occurred:" + error.name);
    endTest();
}

var numFiles = 0, numDirectories = 0;
function verifyTestResult()
{
    for (var i = 0; i < resultEntries.length; ++i) {
        var entry = resultEntries[i];
        if (entry.isDirectory) {
            ++numDirectories;
        } else {
            ++numFiles;
        }
    }
    shouldBe('numFiles', 'numFilesExpected');
    shouldBe('numDirectories', 'numDirectoriesExpected');
}

function entriesCallback(entries)
{
    resultEntries.push.apply(resultEntries, entries);

    if (entries.length) {
        reader.readEntries(entriesCallback, errorCallback);
    } else {
        // Must have read all the entries.
        verifyTestResult();
        endTest();
    }
}

function runReadDirectoryTest()
{
    reader = fileSystem.root.createReader();
    reader.readEntries(entriesCallback, errorCallback);
}

function prepareForTest()
{
    var helper = new JoinHelper();
    var done = function() { helper.done(); };

    for (var i = 0; i < numFilesExpected; ++i) {
        var name = 'file' + i;
        helper.run(function() { fileSystem.root.getFile(name, {create: true}, done, errorCallback); });
    }
    for (var i = 0; i < numDirectoriesExpected; ++i) {
        var name = 'directory' + i;
        helper.run(function() { fileSystem.root.getDirectory(name, {create: true}, done, errorCallback); });
    }
    helper.join(runReadDirectoryTest);
}

function successCallback(fs)
{
    fileSystem = fs;
    debug("Successfully obtained TEMPORARY FileSystem:" + fileSystem.name);
    removeAllInDirectory(fileSystem.root, prepareForTest, errorCallback);
}

jsTestIsAsync = true;
webkitRequestFileSystem(TEMPORARY, 100, successCallback, errorCallback);
