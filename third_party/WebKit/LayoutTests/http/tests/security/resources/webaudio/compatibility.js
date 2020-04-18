// We want to be able to run these tests with chrome that only supports the prefixed
// AudioContext. This is useful in case a regression has occurred.

if (window.hasOwnProperty('webkitAudioContext') &&
    !window.hasOwnProperty('AudioContext')) {
    window.AudioContext = webkitAudioContext;
    window.OfflineAudioContext = webkitOfflineAudioContext;
    console.log("Using deprecated prefixed AudioContext or OfflineAudioContext");
}
