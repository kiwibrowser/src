
var importedDocumentList = [];

function recordImported()
{
    var url = document.currentScript.ownerDocument.URL;
    var name = url.substr(url.lastIndexOf('/') + 1);
    importedDocumentList.push(name);
}