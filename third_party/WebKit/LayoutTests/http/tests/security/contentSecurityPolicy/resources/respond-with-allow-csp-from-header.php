<?php
    $allow_csp_from = isset($_GET['allow_csp_from']) ? $_GET['allow_csp_from'] : null;
    if ($allow_csp_from)
        header('Allow-CSP-From: ' . $allow_csp_from);
    $csp = isset($_GET['csp']) ? $_GET['csp'] : null;
    if ($csp)
        header('Content-Security-Policy: ' . $csp);
    $msg = isset($_GET['id']) ? $_GET['id'] : null;
?>

<!DOCTYPE html>
<html>
<head>
    <title>This page enforces embedder's policies</title>
    <script nonce="123">
        document.addEventListener("securitypolicyviolation", function(e) {
            var response = {};
            response["id"] = "<?php echo $msg; ?>";
            response["securitypolicyviolation"] = true;
            response["blockedURI"] = e.blockedURI;
            response["lineNumber"] = e.lineNumber;
            window.top.postMessage(response, '*');
        });
    </script>
</head>
<body>
    Hello World.
    <iframe src="/cross-site/b.com/title2.html"></iframe>
    <img src="green250x50.png" />
    <script nonce="abc"> 
        var response = {};
        response["loaded"] = true;
        response["id"] = "<?php echo $msg; ?>";
        window.onload =  window.top.postMessage(response, '*');
    </script>
</body>
</html>