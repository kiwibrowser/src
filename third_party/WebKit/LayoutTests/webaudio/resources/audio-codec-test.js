let defaultSampleRate = 44100.0;
let lengthInSeconds = 1;

let context = 0;
let bufferLoader = 0;

// Run test by loading the file specified by |url|.  An optional sample rate can
// be given to select a context with a different sample rate.  The default value
// is |defaultSampleRate|.
function runDecodingTest(url, optionalSampleRate) {
  if (!window.testRunner)
    return;

  let sampleRate = (typeof optionalSampleRate === 'undefined') ?
      defaultSampleRate :
      optionalSampleRate;

  // Create offline audio context.
  context =
      new OfflineAudioContext(1, sampleRate * lengthInSeconds, sampleRate);

  bufferLoader = new BufferLoader(context, [url], finishedLoading);

  bufferLoader.load();
  testRunner.waitUntilDone();
}

function finishedLoading(bufferList) {
  testRunner.setAudioData(createAudioData(bufferList[0]));
  testRunner.notifyDone();
}
