<?php
header("Link: <http://127.0.0.1:8000/resources/square.png>;rel=prefetch",
  false);
?>
<!DOCTYPE html>
<html>
<head></head>
<body>
<script src="/resources/testharness.js"></script>
<script src="/resources/testharnessreport.js"></script>
<script>
  var observer = new PerformanceObserver(function(list) {
    list.getEntries().forEach(function(entry) {
      if (entry.name.indexOf("square.png") != -1) {
        // If we found a resource timing entry of the prefetched image,
        // the test passes.
        t.done();
      }
    });
  });
  observer.observe({entryTypes: ["resource"]});
  var t = async_test('Makes sure that Link headers prefetch resources');
</script>
<script>
  window.addEventListener("load", t.step_func(function() {
    var entries =
      performance.getEntriesByName(
        "http://127.0.0.1:8000/resources/square.png");
    if (entries.length) {
      assert_equals(entries.length, 1);
      t.done();
    }
  }));
</script>
</body>
</html>
