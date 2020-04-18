<?php
header("Referrer-Policy: no-referrer,origin-when-crossorigin");
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
    var policy = "no-referrer,origin-when-crossorigin";
    var expectedReferrer = "";
    var navigateTo = "cross-origin-no-downgrade";
</script>
<script src="resources/header-test.js"></script>
</html>