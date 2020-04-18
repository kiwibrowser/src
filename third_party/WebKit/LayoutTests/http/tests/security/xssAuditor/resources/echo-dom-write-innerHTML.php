<?php header("X-XSS-Protection: 1"); ?>
<!DOCTYPE html>
<html>
<body>
<script>
document.body.innerHTML = unescape(window.location);
</script>
</body>
</html>
