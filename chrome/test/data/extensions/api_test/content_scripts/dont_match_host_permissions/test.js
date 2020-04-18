chrome.extension.sendRequest({
  source: location.hostname,
  modified: window.title == 'Hello'
});
