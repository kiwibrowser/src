<?php
header("Content-Security-Policy: script-src 'self'; report-uri save-report.php?test=generate-csp-report.php");
?>
<script>
// This script block will trigger a violation report.
alert('FAIL');
</script>
<script src="go-to-echo-report.js"></script>
