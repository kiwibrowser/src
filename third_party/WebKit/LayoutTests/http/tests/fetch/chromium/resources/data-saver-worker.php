<?php
header('Content-Type: text/javascript');
echo 'var result = {};' . "\n";
echo 'result["worker_script_header"] = ';
if (isset($_SERVER['HTTP_SAVE_DATA'])) {
    echo '"Save-Data: ' . $_SERVER['HTTP_SAVE_DATA'] . '";' . "\n";
} else {
    echo '"No Save-Data";' . "\n";
}
?>

var CHECK_PATH = './check-save-data-header.php';
var METHODS = ['GET', 'POST', 'PUT'];
var REQUESTS =
    METHODS.map(method => new Request(CHECK_PATH, {method: method}));

if (!self.postMessage) {
  // For Shared Worker
  var postMessagePromise =
      new Promise(resolve => { self.postMessage = resolve; });
  self.addEventListener('connect', event => {
    postMessagePromise.then(data => event.ports[0].postMessage(data));
  });
}

Promise.all(REQUESTS.map(request => fetch(request)))
  .then(responses => Promise.all(responses.map(response => response.text())))
  .then(texts => {
      for (var i = 0; i < METHODS.length; ++i) {
        result[METHODS[i]] = texts[i];
      }
      self.postMessage(result);
    });
