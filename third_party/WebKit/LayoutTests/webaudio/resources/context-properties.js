// The list of the 'own' properties in various AudioContexts. These lists were
// populated by running:
//
//   Object.getOwnPropertyNames(FooAudioContext.prototype);
//
// https://webaudio.github.io/web-audio-api/#BaseAudioContext


let BaseAudioContextOwnProperties = [
  'audioWorklet',
  'constructor',
  'createAnalyser',
  'createBiquadFilter',
  'createBuffer',
  'createBufferSource',
  'createChannelMerger',
  'createChannelSplitter',
  'createConstantSource',
  'createConvolver',
  'createDelay',
  'createDynamicsCompressor',
  'createGain',
  'createIIRFilter',
  'createOscillator',
  'createPanner',
  'createPeriodicWave',
  'createScriptProcessor',
  'createStereoPanner',
  'createWaveShaper',
  'currentTime',
  'decodeAudioData',
  'destination',
  'listener',
  'onstatechange',
  'resume',
  'sampleRate',
  'state',

  // TODO(hongchan): these belong to AudioContext.
  'createMediaElementSource',
  'createMediaStreamDestination',
  'createMediaStreamSource',
];


let AudioContextOwnProperties = [
  'close', 'constructor', 'suspend', 'getOutputTimestamp', 'baseLatency',

  // TODO(hongchan): Not implemented yet.
  // 'outputLatency',
];


let OfflineAudioContextOwnProperties = [
  'constructor',
  'length',
  'oncomplete',
  'startRendering',
  'suspend',
];


/**
 * Verify properties in the prototype with the pre-populated list. This is a
 * 2-way comparison to detect the missing and the unexpected property at the
 * same time.
 * @param  {Object} targetPrototype           Target prototype.
 * @param  {Array} populatedList              Property dictionary.
 * @param  {Function} should                  |Should| assertion function.
 * @return {Map}                              Verification result map.
 */
function verifyPrototypeOwnProperties(targetPrototype, populatedList, should) {
  let propertyMap = new Map();
  let generatedList = Object.getOwnPropertyNames(targetPrototype);

  for (let index in populatedList) {
    propertyMap.set(populatedList[index], {actual: false, expected: true});
  }

  for (let index in generatedList) {
    if (propertyMap.has(generatedList[index])) {
      propertyMap.get(generatedList[index]).actual = true;
    } else {
      propertyMap.set(generatedList[index], {actual: true, expected: false});
    }
  }

  for (let [property, result] of propertyMap) {
    let prefix = 'The property "' + property + '"';
    if (result.expected && result.actual) {
      // The test meets the expectation.
      should(true, prefix).message('was expected and found successfully', '');
    } else if (result.expected && !result.actual) {
      // The expected property is missing.
      should(false, prefix).message('', 'was expected but not found.');
    } else if (!result.expected && result.actual) {
      // Something unexpected was found.
      should(false, prefix).message('', 'was not expected but found.');
    }
  }
}
