// Requests that |video| suspends upon reaching or exceeding |expectedState|;
// |callback| will be called once the suspend is detected.
function suspendMediaElement(video, expectedState, callback) {
  var pollSuspendState = function() {
    if (!internals.isMediaElementSuspended(video)) {
      window.requestAnimationFrame(pollSuspendState);
      return;
    }

    callback();
  };

  window.requestAnimationFrame(pollSuspendState);
  internals.forceStaleStateForMediaElement(video, expectedState);
}

// Calls play() on |video| and executes t.done() when currentTime > 0.
function completeTestUponPlayback(t, video) {
  var timeWatcher = t.step_func(function() {
    if (video.currentTime > 0) {
      assert_false(internals.isMediaElementSuspended(video));
      t.done();
    } else {
      window.requestAnimationFrame(timeWatcher);
    }
  });

  window.requestAnimationFrame(timeWatcher);
  video.play();
}

function preloadMetadataSuspendTest(t, video, src, expectSuspend) {
  assert_true(!!window.internals, 'This test requires windows.internals.');
  video.onerror = t.unreached_func();

  var eventListener = t.step_func(function() {
    assert_equals(expectSuspend,
                  internals.isMediaElementSuspended(video));
    if (!expectSuspend) {
      t.done();
      return;
    }

    completeTestUponPlayback(t, video);
  });

  video.addEventListener('loadedmetadata', eventListener, false);
  video.src = src;
}

function suspendTest(t, video, src, expectedState) {
  assert_true(!!window.internals, 'This test requires windows.internals.');
  video.onerror = t.unreached_func();

  // We can't force a suspend state until loading has started.
  video.addEventListener('loadstart', t.step_func(function() {
    suspendMediaElement(video, expectedState, t.step_func(function() {
      assert_true(internals.isMediaElementSuspended(video));
      completeTestUponPlayback(t, video);
    }));
  }), false);

  video.src = src;
}
