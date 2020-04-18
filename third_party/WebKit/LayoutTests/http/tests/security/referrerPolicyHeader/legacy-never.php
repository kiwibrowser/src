<?php
header("Referrer-Policy: origin-when-cross-origin,never");
?>
<!DOCTYPE html>
<head>
  <script src="/resources/testharness.js"></script>
  <script src="/resources/testharnessreport.js"></script>
  <script src="/resources/get-host-info.js"></script>
</head>
<body>
</body>
<script>
    var policy = "origin-when-cross-origin,never";
    var expectedReferrer = document.location.origin + "/";
    var navigateTo = "cross-origin-no-downgrade";
</script>
<script src="resources/header-test.js"></script>
</html>