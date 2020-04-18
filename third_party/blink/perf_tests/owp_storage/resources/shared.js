function deleteThenOpen(dbName, upgradeFunc, bodyFunc) {
  const deleteRequest = indexedDB.deleteDatabase(dbName);
  deleteRequest.onerror = PerfTestRunner.logFatalError.bind('deleteDatabase should not fail');
  deleteRequest.onsuccess = (e) => {
    const openRequest = indexedDB.open(dbName);
    openRequest.onupgradeneeded = () => {
      upgradeFunc(openRequest.result, openRequest);
    }
    openRequest.onsuccess = () => {
      bodyFunc(openRequest.result, openRequest);
    }
    openRequest.onerror = (e) => {
      window.PerfTestRunner.logFatalError("Error setting up database " + dbName + ". Error: " + e.type);
    }
  }
}

// Non-performant on purpose - should cause relayouts.
function logToDocumentBody(stringOrStrings) {
  let div = document.createElement("div");
  document.body.appendChild(div);
  if (Array.isArray(stringOrStrings)) {
    for (let string of stringOrStrings) {
      div.innerHTML += string;
    }
  } else {
    div.innerHTML = stringOrStrings;
  }
  return div;
}

function createIncrementalBarrier(callback) {
  let count = 0;
  let called = false;
  return () => {
    if (called)
      PerfTestRunner.logFatalError("Barrier already used.");
    ++count;
    return () => {
      --count;
      if (count === 0) {
        if (called)
          PerfTestRunner.logFatalError("Barrier already used.");
        called = true;
        callback();
      }
    }
  }
}

function transactionCompletePromise(txn) {
  return new Promise((resolve, reject) => {
    txn.oncomplete = resolve;
    txn.onabort = reject;
  });
}

function reportDone() {
  window.parent.postMessage({
    message: "done"
  }, "*");
}

function reportError(event) {
  console.log(event);
  window.parent.postMessage({
    message: "error",
    data: event
  }, "*", );
}

if (window.PerfTestRunner) {
  // The file loaded here will signal a 'done' or 'error' message (see
  // reportDone or reportError) which signifies the end of a test run.
  window.PerfTestRunner.measurePageLoadTimeAfterDoneMessage = function(test) {

    let isDone = false;
    let outerDone = test.done;
    test.done = (done) => {
      isDone = true;
      if (outerDone)
        done();
    }

    test.run = () => {
      let file = PerfTestRunner.loadFile(test.path);

      let runOnce = function(finishedCallback) {
        let startTime;

        PerfTestRunner.logInfo("Testing " + file.length + " byte document.");

        let iframe = document.createElement("iframe");
        test.iframe = iframe;
        document.body.appendChild(iframe);

        iframe.sandbox = '';
        // Prevent external loads which could cause write() to return before
        // completing the parse.
        iframe.style.width = "600px";
        // Have a reasonable size so we're not line-breaking on every
        // character.
        iframe.style.height = "800px";
        iframe.contentDocument.open();

        let eventHandler = (event)=>{
          if (event.data.message == undefined) {
            console.log("Unknown message: ", event);
          } else if (event.data.message == "done") {
            PerfTestRunner.measureValueAsync(PerfTestRunner.now() - startTime);
            PerfTestRunner.addRunTestEndMarker();
            document.body.removeChild(test.iframe);
            finishedCallback();
          } else if (event.data.message == "error") {
            console.log("Error in page", event.data.data);
            PerfTestRunner.logFatalError("error in page: " + event.data.data.type);
          } else {
            console.log("Unknown message: ", event);
          }
          window.removeEventListener("message", eventHandler);
        }
        window.addEventListener("message", eventHandler, false);

        PerfTestRunner.addRunTestStartMarker();
        startTime = PerfTestRunner.now();

        if (test.params)
          iframe.contentWindow.params = test.params;

        iframe.contentDocument.write(file);
        PerfTestRunner.forceLayout(iframe.contentDocument);

        iframe.contentDocument.close();
      }

      let iterationCallback = () => {
        if (!isDone)
          runOnce(iterationCallback);
      }

      runOnce(iterationCallback);
    }

    PerfTestRunner.startMeasureValuesAsync(test)
  }
}
