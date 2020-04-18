<?php
   header('Content-Type: text/html');
?>
<script>
var worker = new Worker('third-party-cookie-blocking-worker-worker.php')
worker.addEventListener('message', msg => {
  fetch('/cookies/resources/echo-json.php?from_iframe',
        {credentials: 'include'})
    .then(r => r.json())
    .then(j => {
      var data = msg.data;
      data.iframe_request_cookie = <?php echo json_encode($_COOKIE); ?>;
      data.fetch_in_iframe_cookie = j;
      window.parent.postMessage(data, '*');
    });
});
</script>
