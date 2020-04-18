<?php
    $csp = isset($_GET['csp']) ? $_GET['csp'] : null;
    if ($csp)
        header('Content-Security-Policy: ' . $csp);
    $csp2 = isset($_GET['csp2']) ? $_GET['csp2'] : null;
    if ($csp2)
        header('Content-Security-Policy: ' . $csp2);
    $csp_report_only = isset($_GET['csp_report_only']) ? $_GET['csp_report_only'] : null;
    if ($csp_report_only)
        header('Content-Security-Policy-Report-Only: ' . $csp_report_only);
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