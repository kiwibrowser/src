function output(msg) {
  chrome.devtools.inspectedWindow.eval("console.log(unescape('" +
      escape(msg) + "'));");
}

function test() {
  if (!chrome.experimental || !chrome.experimental.devtools) {
    output("FAIL: chrome.experimental.devtools should be defined");
    return;
  }
  output("PASS");
}

test();
