function output(msg) {
  chrome.devtools.inspectedWindow.eval("console.log(unescape('" +
      escape(msg) + "'));")
}

function test() {
  var expectedAPIs = [
    "inspectedWindow",
    "network",
    "panels"
  ];

  for (var i = 0; i < expectedAPIs.length; ++i) {
    var api = expectedAPIs[i];
    if (typeof chrome.devtools[api] !== "object") {
      output("FAIL: API " + api + " is missing");
      return;
    }
  }
  if (typeof chrome.devtools.inspectedWindow.tabId !== "number") {
    output("FAIL: chrome.inspectedWindow.tabId is not a number");
    return;
  }
  if (chrome.experimental && chrome.experimental.devtools) {
    output("FAIL: chrome.experimental.devtools should not be defined");
    return;
  }
  output("PASS");
}

test();
