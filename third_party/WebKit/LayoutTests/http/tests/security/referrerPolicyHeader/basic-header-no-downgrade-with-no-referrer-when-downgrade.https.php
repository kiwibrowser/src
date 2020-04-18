<?php
header("Referrer-Policy: no-referrer-when-downgrade");
?>
<!DOCTYPE html>
<head>
  <script src="/resources/testharness.js"></script>
  <script src="/resources/testharnessreport.js"></script>
  <script src="/resources/get-host-info.js"></script>
  <script>
      if (window.testRunner) {
          testRunner.overridePreference("WebKitAllowRunningInsecureContent", true);
          testRunner.setAllowRunningOfInsecureContent(true);
      }
  </script>
</head>
<body>
</body>
<script>
    var policy = "no-referrer-when-downgrade";
    var expectedReferrer = document.location.href;
    var navigateTo = "cross-origin-no-downgrade";
</script>
<script src="resources/header-test.js"></script>
</html>