onmessage = function(event)
{
    var count = 10000;
    var chunk = new Uint8Array(1);
    chunk[0] = 0;
    var blob = new Blob([chunk]);
    for (var i = 0; i < count; i++) {
        var reader = new FileReader();
        reader.readAsArrayBuffer(blob);
    }
    postMessage({});
    for (var i = 0; i < count; i++) {
        var reader = new FileReader();
        reader.readAsArrayBuffer(blob);
    }
}
