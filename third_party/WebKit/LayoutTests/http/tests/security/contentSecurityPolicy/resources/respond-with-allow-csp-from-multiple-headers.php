<?php
    $allow_csp_from = isset($_GET['allow_csp_from']) ? $_GET['allow_csp_from'] : null;
    if ($allow_csp_from)
        header('Allow-CSP-From: ' . $allow_csp_from, false);
    $allow_csp_from_2 = isset($_GET['allow_csp_from_2']) ? $_GET['allow_csp_from_2'] : null;
    if ($allow_csp_from_2)
        header('Allow-CSP-From: ' . $allow_csp_from_2, false);
?>
<!DOCTYPE html>
<html>
<head>
    <title>This page enforces embedder's policies</title>
</head>
<body>
    Hello World.
    <iframe src="/cross-site/b.com/title2.html"></iframe>
    <img src="green250x50.png" />
    <script> alert("Hello from iframe");</script>
    <script nonce="abc"> 
        var response = {};
        response["loaded"] = true;
        response["id"] = "<?php echo $msg; ?>";
        window.onload =  window.top.postMessage(response, '*');
    </script>
</body>
</html>
