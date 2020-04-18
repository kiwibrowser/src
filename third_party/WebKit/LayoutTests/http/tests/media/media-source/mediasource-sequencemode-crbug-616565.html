<!DOCTYPE html>
<html>
    <head>
        <script src="/w3c/resources/testharness.js"></script>
        <script src="/w3c/resources/testharnessreport.js"></script>
        <script src="mediasource-util.js"></script>
    </head>
    <body>
        <script>
            mediasource_testafterdataloaded(function(test, mediaElement, mediaSource, segmentInfo, sourceBuffer, mediaData)
            {
                assert_greater_than(segmentInfo.media.length, 3, "at least 3 media segments for supported type");
                test.failOnEvent(mediaElement, "error");
                sourceBuffer.mode = "sequence";
                assert_equals(sourceBuffer.mode, "sequence", "mode after setting it to 'sequence'");

                // Append all media segments, preceded by a (now redundant) initialization segment.
                test.expectEvent(sourceBuffer, "updatestart", "media segment append started.");
                test.expectEvent(sourceBuffer, "update", "media segment append success.");
                test.expectEvent(sourceBuffer, "updateend", "media segment append ended.");
                sourceBuffer.appendBuffer(mediaData);

                test.waitForExpectedEvents(function()
                {
                    // Remove much of what we just appended.
                    test.expectEvent(sourceBuffer, "updatestart", "remove started.");
                    test.expectEvent(sourceBuffer, "update", "remove success.");
                    test.expectEvent(sourceBuffer, "updateend", "remove ended.");
                    sourceBuffer.remove(segmentInfo.media[3].timecode, mediaSource.duration);
                });

                test.waitForExpectedEvents(function()
                {
                    // Set the timestampOffset to the beginning of the region we just removed.
                    sourceBuffer.timestampOffset = segmentInfo.media[3].timecode;

                    // Remove everything.
                    test.expectEvent(sourceBuffer, "updatestart", "remove started.");
                    test.expectEvent(sourceBuffer, "update", "remove success.");
                    test.expectEvent(sourceBuffer, "updateend", "remove ended.");
                    sourceBuffer.remove(0, mediaSource.duration);
                });

                test.waitForExpectedEvents(function()
                {
                    // Set the timestampOffset back to the beginning.
                    sourceBuffer.timestampOffset = 0;

                    // Re-append everything.
                    test.expectEvent(sourceBuffer, "updatestart", "media segment append started.");
                    test.expectEvent(sourceBuffer, "update", "media segment append success.");
                    test.expectEvent(sourceBuffer, "updateend", "media segment append ended.");
                    sourceBuffer.appendBuffer(mediaData);
                });

                test.waitForExpectedEvents(function()
                {
                    // Use EOS to get a more precisely verifiable buffered range given our segmentInfo.
                    test.expectEvent(mediaSource, "sourceended", "mediaSource endOfStream");
                    mediaSource.endOfStream();
                });

                test.waitForExpectedEvents(function()
                {
                    assertBufferedEquals(sourceBuffer, "{ [0.000, " + segmentInfo.duration.toFixed(3) + ") }");
                    test.done();
                });
            }, "Test no error for sequence mode remove and timestampOffset scenario in bug 616565");
        </script>
    </body>
</html>
