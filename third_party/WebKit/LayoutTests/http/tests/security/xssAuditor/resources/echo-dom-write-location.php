<?php header("X-XSS-Protection: 1"); ?>
<!DOCTYPE html>
<html>
<head>
</head>
<body>
<script>document.write(unescape(window.location));</script>
</body>
</html>
