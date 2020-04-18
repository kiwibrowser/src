<?php
header("Content-Security-Policy: img-src http://* https://*; script-src 'self' 'unsafe-inline';, img-src http://*; script-src 'self' 'unsafe-inline';");
?>
<!DOCTYPE html>
<html>
<head>
<script>
if (window.testRunner)
    testRunner.dumpAsText();
</script>
</head>
<body>
    <img src="ftp://blah.test" />
</body>
</html>