<?php
header("Referrer-Policy: origin-when-cross-origin");
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
    var policy = "origin-when-cross-origin";
    var expectedReferrer = document.location.href;
    var navigateTo = "same-origin";
</script>
<script src="resources/header-test.js"></script>
</html>
