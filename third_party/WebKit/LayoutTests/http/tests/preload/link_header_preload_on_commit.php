<?php
    header("Link: </resources/dummy.css>;rel=preload;as=style", false);
    header("Link: </resources/square.png>;rel=preload;as=image;media=(min-width: 1px)", false);
?>
<!DOCTYPE html>
<html>
<body>
<script src="/resources/testharness.js"></script>
<script src="/resources/testharnessreport.js"></script>
<link rel="preload" href="/resources/dummy.js" as="script">
<script>
    var t = async_test('Makes sure that Link headers preload resources');
</script>
<script src="/resources/slow-script.pl?delay=200"></script>
<script>
    window.addEventListener("load", t.step_func(function() {
      var nonMediaHeader = performance.getEntriesByName(
        "http://127.0.0.1:8000/resources/dummy.css")[0];
      var mediaHeader = performance.getEntriesByName(
        "http://127.0.0.1:8000/resources/square.png")[0];
      var markup = performance.getEntriesByName(
        "http://127.0.0.1:8000/resources/dummy.js")[0];
      var normalResource = performance.getEntriesByName(
        "http://127.0.0.1:8000/resources/testharness.js")[0];
      // The non-media header can be processed at commit time.
      // The media header needs to tokenize the first chunk of html, and the
      // markup still needs to be tokenized.
      // Note: the link preloads can be preloaded before the normal resource
      // because they don't need to wait for the document element to be
      // available (ApplicationCache initialization time).
      assert_greater_than(markup.startTime, nonMediaHeader.startTime);
      assert_greater_than(mediaHeader.startTime, nonMediaHeader.startTime);
      assert_greater_than(normalResource.startTime, mediaHeader.startTime);
      assert_greater_than(normalResource.startTime, markup.startTime);
      t.done();
    }));
</script>
</body>
</html>

