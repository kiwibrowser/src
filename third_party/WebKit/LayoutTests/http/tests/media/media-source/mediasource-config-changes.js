// Extract & return the resolution string from a filename, if any.
function resolutionFromFilename(filename)
{
    resolution = filename.replace(/^.*[^0-9]([0-9]+x[0-9]+)[^0-9].*$/, "$1");
    if (resolution != filename) {
        return resolution;
    }
    return "";
}

function appendBuffer(test, sourceBuffer, data)
{
    test.expectEvent(sourceBuffer, "update");
    test.expectEvent(sourceBuffer, "updateend");
    sourceBuffer.appendBuffer(data);
}

function mediaSourceConfigChangeTest(directory, idA, idB, description)
{
    var manifestFilenameA = directory + "/test-" + idA + "-manifest.json";
    var manifestFilenameB = directory + "/test-" + idB + "-manifest.json";
    mediasource_test(function(test, mediaElement, mediaSource)
    {
        mediaElement.pause();
        test.failOnEvent(mediaElement, 'error');
        var expectResizeEvents = resolutionFromFilename(manifestFilenameA) != resolutionFromFilename(manifestFilenameB);
        var expectedResizeEventCount = 0;

        MediaSourceUtil.fetchManifestAndData(test, manifestFilenameA, function(typeA, dataA)
        {
            MediaSourceUtil.fetchManifestAndData(test, manifestFilenameB, function(typeB, dataB)
            {
                assert_equals(typeA, typeB, "Media format types match");

                var sourceBuffer = mediaSource.addSourceBuffer(typeA);

                appendBuffer(test, sourceBuffer, dataA);
                ++expectedResizeEventCount;

                test.waitForExpectedEvents(function()
                {
                    // Add the second buffer starting at 0.5 second.
                    sourceBuffer.timestampOffset = 0.5;
                    appendBuffer(test, sourceBuffer, dataB);
                    ++expectedResizeEventCount;
                });

                test.waitForExpectedEvents(function()
                {
                    // Add the first buffer starting at 1 second.
                    sourceBuffer.timestampOffset = 1;
                    appendBuffer(test, sourceBuffer, dataA);
                    ++expectedResizeEventCount;
                });

                test.waitForExpectedEvents(function()
                {
                    // Add the second buffer starting at 1.5 second.
                    sourceBuffer.timestampOffset = 1.5;
                    appendBuffer(test, sourceBuffer, dataB);
                    ++expectedResizeEventCount;
                });

                test.waitForExpectedEvents(function()
                {
                    // Truncate the presentation to a duration of 2 seconds.
                    // First, explicitly remove the media beyond 2 seconds.
                    assert_false(sourceBuffer.updating, "sourceBuffer.updating before range removal");
                    sourceBuffer.remove(2, mediaSource.duration);

                    assert_true(sourceBuffer.updating, "sourceBuffer.updating during range removal");
                    test.expectEvent(sourceBuffer, "updatestart");
                    test.expectEvent(sourceBuffer, "update");
                    test.expectEvent(sourceBuffer, "updateend");
                });

                test.waitForExpectedEvents(function()
                {
                    // Complete the truncation of presentation to 2 second
                    // duration.
                    assert_false(sourceBuffer.updating, "sourceBuffer.updating prior to duration reduction");
                    mediaSource.duration = 2;
                    assert_false(sourceBuffer.updating, "sourceBuffer.updating synchronously after duration reduction");

                    test.expectEvent(mediaElement, "durationchange");
                });

                test.waitForExpectedEvents(function()
                {
                    mediaSource.endOfStream();

                    if (expectResizeEvents) {
                        for (var i = 0; i < expectedResizeEventCount; ++i) {
                            test.expectEvent(mediaElement, "resize");
                        }
                    }
                    test.expectEvent(mediaElement, "ended");
                    mediaElement.play();
                });

                test.waitForExpectedEvents(function() {
                    // TODO(wolenetz): Remove this hacky console warning once
                    // desktop and android expectations match. It allows a
                    // passing platform-specific expectation to override a
                    // failing non-platform-specific expectation.
                    console.warn('Ignore this warning. See https://crbug.com/568704#c2');

                    test.done();
                });
            });
        });
    }, description, { timeout: 10000 } );
};
