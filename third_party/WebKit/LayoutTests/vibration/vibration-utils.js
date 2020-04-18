/**
 * Creates a new array with length equal to [entries]. Each entry has the
 * value [milliseconds].
 */
function newPattern(milliseconds, entries) {
  var pattern = new Array(entries);
  for (var i = 0; i < entries; i++) {
    pattern[i] = milliseconds;
  }
  return pattern;
}

/**
 * Stop any currently running vibration. Call this to clean up internal state.
 */
function stopVibration() {
  navigator.vibrate(0);
}
