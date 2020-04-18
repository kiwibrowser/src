
chrome.browserAction.onClicked.addListener(function() {
  var url = chrome.runtime.getURL("missing.txt");
  console.log("trying to fetch " + url);
  var xhr = new XMLHttpRequest();
  xhr.open("GET", url);
  xhr.send();
  xhr.onReadyStateChange = function() {
    console.log("onReadyStateChange : " + xhr.readyState);
    console.dir(xhr);
  };
  xhr.onerror = function() {
    console.log("error");
    console.dir(xhr);
  };
});
