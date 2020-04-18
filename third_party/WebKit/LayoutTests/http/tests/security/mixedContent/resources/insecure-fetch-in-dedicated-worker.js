fetch("http://example.test:8000/xmlhttprequest/resources/access-control-allow-lists.php?origin=*")
  .then(res => res.text())
  .then(text => postMessage('LOADED'))
  .catch(e => postMessage('LOAD FAILED'));
