<?php
   header('Content-Type: text/javascript');
?>
fetch('/cookies/resources/echo-json.php?from_worker', {credentials: 'include'})
  .then(r => r.json())
  .then(j => {
    self.postMessage({
      worker_request_cookie: <?php echo json_encode($_COOKIE); ?>,
      fetch_in_worker_cookie: j
    })
  });
