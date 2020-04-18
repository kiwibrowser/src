function generateURL(type) {
  return 'http://localhost:8000/security/contentSecurityPolicy/resources/loaded.js?' + type;
}

function assert_script_failed(t, url, line) {
  window.addEventListener('message', t.step_func(e => {
    if (e.data == url)
      t.unreached_func("Script should not have loaded.");
  }));
  document.addEventListener('securitypolicyviolation', t.step_func(e => {
    if (e.lineNumber == line)
      t.done();
  }));
}

function assert_script_loads(t, url, line) {
  window.addEventListener('message', t.step_func(e => {
    if (e.data == url)
      t.done();
  }));
  document.addEventListener('securitypolicyviolation', t.step_func(e => {
    if (e.lineNumber == line)
      t.unreached_func("Script should not have failed.");
  }));
}
