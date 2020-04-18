if (this.importScripts) {
    importScripts('../resources/fs-worker-common.js');
    importScripts('../../../resources/js-test.js');
    importScripts('../resources/fs-test-util.js');
}

description("DirectoryReaderSync.readEntries() test with many entries.");

var fileSystem = webkitRequestFileSystemSync(this.TEMPORARY, 100);
removeAllInDirectorySync(fileSystem.root);

// Prepare entries.
var numFilesExpected = 150;
for (var i = 0; i < numFilesExpected; ++i)
    fileSystem.root.getFile('file' + i, {create: true});

var numDirectoriesExpected = 150;
for (var i = 0; i < numDirectoriesExpected; ++i)
    fileSystem.root.getDirectory('directory' + i, {create: true});

// Read entries.
var resultEntries = [];
var reader = fileSystem.root.createReader();
var entries;
do {
    entries = reader.readEntries();
    resultEntries.push.apply(resultEntries, entries);
} while (entries.length);

// Verify
var numFiles = 0, numDirectories = 0;
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

removeAllInDirectorySync(fileSystem.root);
finishJSTest();
