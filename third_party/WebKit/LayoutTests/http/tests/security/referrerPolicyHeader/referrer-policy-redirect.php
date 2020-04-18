<?php
header("Referrer-Policy: origin");
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
    var policy = "no-referrer on redirect";
    var expectedReferrer = "";
    var navigateTo = "no-referrer-on-redirect";
</script>
<script src="resources/header-test.js"></script>
</html>