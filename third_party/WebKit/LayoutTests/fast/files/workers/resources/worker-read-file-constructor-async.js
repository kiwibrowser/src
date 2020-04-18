importScripts("../../resources/read-common.js")

function log(message)
{
    postMessage(message);
}

function readFiles(index, files)
{
    if (index >= files.length) {
        log("DONE");
        return;
    }

    log("Reading: '" + files[index].name + "'");
    log("Last modified: '" + (new Date(files[index].lastModified)).toUTCString() + "'");

    var reader = new FileReader();
    var isText = files[index].type.indexOf("text") > -1;
    reader.onload = function (e) {
        if (isText) {
            log("Contents: '" + reader.result + "'");
            log("Length: " + reader.result.length);
        } else
            log("Length: " + reader.result.byteLength);
        readFiles(index + 1, files);
    };
    if (isText)
        reader.readAsText(files[index]);
    else
        reader.readAsArrayBuffer(files[index])
}

onmessage = function (e) {
    log("Received files in worker");
    readFiles(0, e.data);
};
