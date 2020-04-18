// Test MediaStreamAudioSourceNode's with different URLs.
//
var context = 0;
var lengthInSeconds = 1;
var sampleRate = 44100;
var source = 0;
var audio = 0;
var spn = 0;

// Create an MediaElementSource node with the given |url| and connect it to webaudio.
// |oncomplete| is given the completion event to check the result.
function runTest (url, oncomplete, tester)
{
    if (window.testRunner) {
        testRunner.dumpAsText();
        testRunner.waitUntilDone();
    }

    window.jsTestIsAsync = true;

    context = new AudioContext();
    context.suspend();

    audio = document.createElement('audio');
    audio.autoplay = true;

    source = context.createMediaElementSource(audio);
    spn = context.createScriptProcessor(16384, 1, 1);
    source.connect(spn).connect(context.destination);

    // Note: In practice this is not a reliable way to ensure the media element
    // is ready to provide samples; unfortunately if the element is not ready
    // yet, the offline context may produce silence in a spin-loop.
    //
    // With some knowledge of the internals we can make this test work by
    // marking the element as autoplay above; this mostly ensures that the
    // pipeline is ready to provide samples.
    audio.addEventListener("playing", function(e) {
        // If we receive multiple playing events, we still can't invoke
        // startRendering multiple times.
        context.resume().then(() => {
                              spn.onaudioprocess = function(e) {
                                checkResult(e.inputBuffer);
                                // Stop the context so we don't keep getting called anymore.
                                context.close();
                                finishJSTest();
                              }
          });
    });


    if (tester) {
        tester();
    } else {
        audio.src = url;
    }
}
