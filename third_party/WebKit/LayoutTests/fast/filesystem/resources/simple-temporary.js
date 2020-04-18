if (this.importScripts) {
    importScripts('../resources/fs-worker-common.js');
    importScripts('../../../resources/js-test.js');
    importScripts('../resources/fs-test-util.js');
}

description("requestFileSystem TEMPORARY test.");

var fileSystem = null;

function errorCallback(error) {
    debug('Error occurred while requesting a TEMPORARY file system:' + error.name);
    finishJSTest();
}

function successCallback(fs) {
    fileSystem = fs;
    debug("Successfully obtained TEMPORARY FileSystem:" + fileSystem.name);
    shouldBeTrue("fileSystem.name.length > 0");
    shouldBe("fileSystem.root.fullPath", '"/"');
    finishJSTest();
}

var jsTestIsAsync = true;
webkitRequestFileSystem(TEMPORARY, 100, successCallback, errorCallback);
