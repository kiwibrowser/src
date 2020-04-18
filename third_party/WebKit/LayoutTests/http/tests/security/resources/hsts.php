<?php
header("Strict-Transport-Security: max-age=10");
header("Access-Control-Allow-Origin: *");
?>
<script>
  top.postMessage("hsts", "*");
</script>
