<?php
// TODO(iclelland): Generate this sample token during the build. The token
// below will expire in 2033, but it would be better to always have a token which
// is guaranteed to be valid when the tests are run.
// Generate these token with the commands:
// generate_token.py http://127.0.0.1:8000 Grokalyze -expire-timestamp=2000000000
// generate_token.py http://127.0.0.1:8000 Frobulate -expire-timestamp=2000000000
// generate_token.py http://127.0.0.1:8000 EnableMarqueeTag -expire-timestamp=2000000000
header("Origin-Trial: Am5nmX0C/DpiE2xRlcZ6D4wvTiO5ydK4ATI0aUDxpIXD4MIoll0XEsIT8Qgdq4+tEYxOeJo/Y0QtkpYwRNWScw4AAABReyJvcmlnaW4iOiAiaHR0cDovLzEyNy4wLjAuMTo4MDAwIiwgImZlYXR1cmUiOiAiR3Jva2FseXplIiwgImV4cGlyeSI6IDIwMDAwMDAwMDB9");
header("Origin-Trial: \"AlCoOPbezqtrGMzSzbLQC4c+oPqO6yuioemcBPjgcXajF8jtmZr4B8tJRPAARPbsX6hDeVyXCKHzEJfpBXvZgQEAAABReyJvcmlnaW4iOiAiaHR0cDovLzEyNy4wLjAuMTo4MDAwIiwgImZlYXR1cmUiOiAiRnJvYnVsYXRlIiwgImV4cGlyeSI6IDIwMDAwMDAwMDB9\", Avs0tgQGv771bXLAScGOi5VpjYdIW/nbb00qk3rH8T6/+7NVTlBosCz05fCg9Yb3N3P9h2IuadgfNtPTMMpirQwAAABYeyJvcmlnaW4iOiAiaHR0cDovLzEyNy4wLjAuMTo4MDAwIiwgImZlYXR1cmUiOiAiRW5hYmxlTWFycXVlZVRhZyIsICJleHBpcnkiOiAyMDAwMDAwMDAwfQ==", false);
?>
<!DOCTYPE html>
<meta charset="utf-8">
<title>Test Sample API when trial is enabled</title>
<script src="../resources/testharness.js"></script>
<script src="../resources/testharnessreport.js"></script>
<script src="resources/origintrials.js"></script>
<script>

// The trial is enabled by the token above in header.
expect_success();

</script>
