<?php header("X-XSS-Protection: 1"); ?>
<!DOCTYPE html>
<html>
<head>
</head>
<body>
<script>document.write(window.location);</script>
</body>
</html>
