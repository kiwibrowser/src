// Defined in core/html/shadow/MediaControls.cpp.
// Minimum width is 48px.
var NARROW_VIDEO_WIDTH = 40;
var NORMAL_VIDEO_WIDTH = 200;
// Minimum height is 56px.
var NARROW_VIDEO_HEIGHT = 50;
var NORMAL_VIDEO_HEIGHT = 200;

function assertOverlayPlayButtonVisible(videoElement) {
  assert_true(isVisible(overlayPlayButton(videoElement)),
      "overlay play button should be visible");
}

function assertOverlayPlayButtonNotVisible(videoElement) {
  assert_false(isVisible(overlayPlayButton(videoElement)),
      "overlay play button should not be visible");
}

function overlayPlayButton(videoElement) {
  var controlID = '-webkit-media-controls-overlay-play-button';
  var button = mediaControlsElement(
      internals.shadowRoot(videoElement).firstChild,
      controlID);
  if (!button)
    throw 'Failed to find overlay play button';
  return button;
}

function enableOverlayPlayButtonForTest(t) {
  var mediaControlsOverlayPlayButtonValue =
      internals.runtimeFlags.mediaControlsOverlayPlayButtonEnabled;
  internals.runtimeFlags.mediaControlsOverlayPlayButtonEnabled = true;

  t.add_cleanup(() => {
    internals.runtimeFlags.mediaControlsOverlayPlayButtonEnabled =
        mediaControlsOverlayPlayButtonValue;
  });
}
