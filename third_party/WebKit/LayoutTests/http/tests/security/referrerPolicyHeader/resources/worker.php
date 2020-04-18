<?
header('Content-Type: application/javascript');
header('Referrer-Policy: origin');
?>
importScripts('save-referrer.php');

// When loaded as a shared worker, send the referrer on connect.
onconnect = function (e) {
  var port = e.ports[0];
  port.postMessage(referrer);
}

// When loaded as a dedicated worker, send the referrer to the document immediately.
postMessage(referrer);