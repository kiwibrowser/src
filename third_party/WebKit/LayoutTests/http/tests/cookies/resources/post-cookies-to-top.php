<!doctype html>
<script>
var from_http = <?php
echo json_encode($_COOKIE);
?>;

window.top.postMessage({
    'http': from_http,
    'document': document.cookie
}, "*");
</script>
