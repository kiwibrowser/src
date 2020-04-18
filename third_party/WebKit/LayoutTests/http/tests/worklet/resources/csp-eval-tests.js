function openWindow(url) {
  return new Promise(resolve => {
    const win = window.open(url, '_blank');
    add_result_callback(() => win.close());
    window.onmessage = e => {
      assert_equals(e.data, 'LOADED');
      resolve(win);
    };
  });
}

function openWindowAndExpectResult(windowURL, scriptURL, type, expectation) {
  return openWindow(windowURL).then(win => {
    const promise = new Promise(r => window.onmessage = r);
    win.postMessage({ type: type, script_url: scriptURL }, '*');
    return promise;
  }).then(msg_event => assert_equals(msg_event.data, expectation));
}

// Runs a series of tests related to content security policy for eval() on a
// worklet.
//
// Usage:
// runContentSecurityPolicyEvalTests("paint");
//
// These tests should not be upstreamed to WPT because this tests console
// outputs.
function runContentSecurityPolicyEvalTests(workletType) {
  promise_test(t => {
    const kWindowURL = 'resources/addmodule-window.html';
    const kScriptURL = 'eval-worklet-script.js';
    // Note that evaluation failure by disallowed eval() call does not reject
    // the addModule() promise.
    return openWindowAndExpectResult(
        kWindowURL, kScriptURL, workletType, 'RESOLVED');
  }, 'eval() call on the worklet should be blocked because the script-src ' +
     'unsafe-eval directive is not specified.');

  promise_test(t => {
    const kWindowURL = 'resources/addmodule-window-with-unsafe-eval.html';
    const kScriptURL = 'eval-worklet-script.js';
    return openWindowAndExpectResult(
        kWindowURL, kScriptURL, workletType, 'RESOLVED');
  }, 'eval() call on the worklet should not be blocked because the ' +
     'script-src unsafe-eval directive allows it.');
}
