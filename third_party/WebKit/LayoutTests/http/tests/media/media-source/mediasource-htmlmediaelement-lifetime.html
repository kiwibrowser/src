<!DOCTYPE html>
<title>Tests that the MediaSource keeps the HTMLMediaElement alive.</title>
<script src="/w3c/resources/testharness.js"></script>
<script src="/w3c/resources/testharnessreport.js"></script>
<script>
async_test(function(test) {
    var video = document.createElement("video");
    var mediaSource = new MediaSource();

    mediaSource.onsourceopen = test.step_func(function() {
        // sourceOpened called.
        var buffer = mediaSource.addSourceBuffer('video/webm; codecs="vorbis,vp8"');

        // Running the garbage collector.
        video = null;
        gc();

        setTimeout(test.step_func(function() {
            assert_equals(mediaSource.readyState, "open", "MediaSource object is open.");
            // Setting MediaSource duration.
            mediaSource.duration = 100;
        }), 0);
    });

    video.ondurationchange = test.step_func_done();
    video.src = URL.createObjectURL(mediaSource);
});
</script>