function assert_allowed_image_in_document(test, doc, url) {
  doc.addEventListener('securitypolicyviolation', test.step_func(e => {
    if (e.blockedURI == url)
      assert_unreached("No violation expected for " + url);
  }));

  var i = doc.createElement('img');
  i.onerror = test.unreached_func("onerror fired for " + url);
  i.onload = test.step_func_done();
  i.src = url;
}


function assert_blocked_image_in_document(test, doc, url) {
  var cspEvent = false;
  var errorEvent = false;
  doc.addEventListener('securitypolicyviolation', test.step_func(e => {
    if (e.blockedURI != url)
      return;

    cspEvent = true;
    if (errorEvent)
      test.done();
  }));

  var i = doc.createElement('img');
  i.onload = test.unreached_func("onload fired for " + url);
  i.onerror = test.step_func(e => {
    errorEvent = true;
    if (cspEvent)
      test.done();
  });
  i.src = url;
}
