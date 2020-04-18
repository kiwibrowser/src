var jsTestIsAsync = true;

iframe = document.createElement("IFRAME");
iframe.src = 'about:blank';
document.body.appendChild(iframe);

storageEventList = new Array();

function runAfterNStorageEvents(callback, expectedNumEvents)
{
    countStorageEvents(callback, expectedNumEvents, 0);
}

function countStorageEvents(callback, expectedNumEvents, times)
{
    function onTimeout()
    {
        var currentCount = storageEventList.length;
        if (currentCount == expectedNumEvents)
            callback();
        else if (currentCount > expectedNumEvents) {
            testFailed("got at least " + currentCount + ", expected only " + expectedNumEvents + " events");
            callback();
        } else if (times > 50) {
            testFailed("Timeout: only got " + currentCount + ", expected " + expectedNumEvents + " events");
            callback();
        } else {
            countStorageEvents(callback, expectedNumEvents, times+1);
        }
    }
    setTimeout(onTimeout, 20);
}

function testStorages(testCallback)
{
    // When we're done testing with SessionStorage, this is run.
    function runLocalStorage()
    {
        debug("");
        debug("");
        testCallback("localStorage", finishJSTest);
    }

  // First run the test with SessionStorage.
    testCallback("sessionStorage", runLocalStorage);
}
