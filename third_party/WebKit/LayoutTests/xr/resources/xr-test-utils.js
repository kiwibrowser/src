// func: A function that takes a session and optionally a test object and
// performs tests. If func returns a promise, test will only pass if the promise
// resolves.
function xr_session_promise_test(
    func, device, sessionOptions, name, properties) {
  if (document.getElementById('webgl-canvas') ||
      document.getElementById('webgl2-canvas')) {
    webglCanvasSetup();
  }

  addFakeDevice(device);

  promise_test((t) => {
    return navigator.xr.requestDevice()
        .then((device) => {
          if (gl) {
            return gl.setCompatibleXRDevice(device).then(
                () => Promise.resolve(device));
          } else {
            return Promise.resolve(device);
          }
        })
        .then(
            (device) => new Promise((resolve, reject) => {
              // Run the test with each set of sessionOptions from the array one
              // at a time.
              function nextSessionTest(i) {
                if (i == sessionOptions.length) {
                  if (sessionOptions.length == 0) {
                    reject('No option for the session. Test Did not run.');
                  } else {
                    resolve();
                  }
                }
                // Perform the session request in a user gesture.
                runWithUserGesture(() => {
                  let nextOptions = sessionOptions[i];
                  let testSession = null;
                  device.requestSession(nextOptions)
                      .then((session) => {
                        testSession = session;
                        return func(session, t);
                      })
                      .then(() => {
                        // Wrap in a try in case the session was ended in the
                        // test itself.
                        try {
                          // If there's another test to run after this one make
                          // sure to end the session so that we don't
                          // accidentally try to have, for example, two
                          // exclusive sessions at once.
                          if (i < sessionOptions.length - 1) {
                            testSession.end();
                          }
                        } finally {
                          nextSessionTest(++i);
                        }
                      })
                      .catch((err) => {
                        let optionsString = '{';
                        let firstOption = true;
                        for (let option in nextOptions) {
                          if (!firstOption) {
                            optionsString += ',';
                          }
                          optionsString += ` ${option}: ${nextOptions[option]}`;
                          firstOption = false;
                        }
                        optionsString += ' }';
                        reject(
                            `Test failed while running with the following options:
                            ${optionsString} ${err}`);
                      });
                });
              }

              nextSessionTest(0);
            }));
  }, name, properties);
}

let webglCanvas, gl;
function webglCanvasSetup() {
  let webgl2 = false;
  webglCanvas = document.getElementById('webgl-canvas');
  if (!webglCanvas) {
    webglCanvas = document.getElementById('webgl2-canvas');
    webgl2 = true;
  }
  let glAttributes = {
    alpha: false,
    antialias: false,
  };
  gl = webglCanvas.getContext(webgl2 ? 'webgl2' : 'webgl', glAttributes);
}

function getOutputContext() {
  let outputCanvas = document.createElement('canvas');
  document.body.appendChild(outputCanvas);
  return outputCanvas.getContext('xrpresent');
}

// TODO(offenwanger): eventSender cannot be used with WPTs. Find another way to
// fake use gestures.
// https://chromium.googlesource.com/chromium/src/+/lkgr/docs/testing/web_platform_tests.md#tests-that-require-testing-apis
function runWithUserGesture(fn) {
  function thunk() {
    document.removeEventListener('keypress', thunk, false);
    fn()
  }
  document.addEventListener('keypress', thunk, false);
  eventSender.keyDown(' ', []);
}