function log(msg) {
    var div = document.createElement('div');
    div.appendChild(document.createTextNode(msg));
    document.getElementById('result').appendChild(div);
}

function extractMessage(e) {
    return 'Received message: data="' + e.data + '" origin="' + e.origin + '"';
}

function recv(e) {
    var msg = extractMessage(e);

    log(msg);

    if (e.data.match(/data="done"/) && window.testRunner)
        testRunner.notifyDone();
}

