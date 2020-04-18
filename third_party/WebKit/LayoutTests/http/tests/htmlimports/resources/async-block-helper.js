
var loadedImports = [];

function importLoaded(loadedDocument) {
    var name = loadedDocument.URL.substr(loadedDocument.URL.lastIndexOf("/") + 1);
    loadedImports.push(name);
    if (window.notifyImportLoaded)
        window.notifyImportLoaded(name);
}

function isImportLoaded(url) {
    return 0 <= loadedImports.indexOf(url);
}