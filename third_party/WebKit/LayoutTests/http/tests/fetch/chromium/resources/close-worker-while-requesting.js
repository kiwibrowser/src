fetch('/fetch/slow-failure.cgi?id=1').then(res => {
    fetch('/fetch/slow-failure.cgi?id=2');
    postMessage('PASS');
    self.close();
  }).catch(e => {
    postMessage('FAIL: ' + e);
  });
