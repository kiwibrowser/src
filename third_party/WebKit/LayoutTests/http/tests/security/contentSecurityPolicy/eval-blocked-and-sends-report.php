<?php
header("Content-Security-Policy: script-src 'self' 'unsafe-inline'; report-uri resources/save-report.php?test=eval-blocked-and-sends-report.php");
?>
<!DOCTYPE html>
<html>
<head>
</head>
<body>
    <script>
        try {
            eval("alert('FAIL')");
        } catch (e) {
            console.log(e);
            console.log('PASS: eval() blocked.');
        }
    </script>
    <script src="resources/go-to-echo-report.js"></script>
</body>
</html>
