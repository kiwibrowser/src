importScripts("/js-test-resources/js-test.js");
importScripts("worker-pre.js");

function init() {
    var global = self;

    try {
        global.req = new XMLHttpRequest;
        req.responseType = "blob";
        req.open("GET", "../../resources/get.txt", false);
        req.send(null);

        shouldBe("req.status", "200");
        shouldBe("req.response.size", "4");
        var reader = new FileReader();
        reader.onload = function (evt) {
            global.buffer = evt.target.result;
            shouldBeEqualToString("buffer", "PASS");
            testPassed("Sync XHR with responseType=\"blob\" succeeded");
            finishJSTest();
        };
        reader.readAsText(req.response, "utf-8");
    } catch (e) {
        testFailed("failed to create XMLHttpRequest with exception: " + e.message);
        finishJSTest();
    }
}
