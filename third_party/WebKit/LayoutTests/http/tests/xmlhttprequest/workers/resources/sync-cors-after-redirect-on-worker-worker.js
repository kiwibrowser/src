const url =
    '/xmlhttprequest/resources/redirect.php?url=' +
    'http://localhost:8000/xmlhttprequest/resources/access-control-basic-allow.cgi';
try {
  const xhr = new XMLHttpRequest;
  xhr.open('GET', url, false);
  xhr.send(null);
  self.postMessage(xhr.responseText);
} catch (e) {
  self.postMessage("Failed");
}
