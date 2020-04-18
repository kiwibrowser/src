onmessage = function(evt) {
    new Promise(function(res,rej) { res(); }).then(function() {
        var xhr = new XMLHttpRequest();
        xhr.open("GET", "http://example.test", false);
        try {
            xhr.send();
            console.log("FAIL: xhr.send() didn't throw");
        } catch (e) {
            console.log("PASS: xhr.send() throws as expected");
        }
        postMessage("done");
    });
};
