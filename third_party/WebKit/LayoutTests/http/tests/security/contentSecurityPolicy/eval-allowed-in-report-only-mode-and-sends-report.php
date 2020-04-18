<?php
header("Content-Security-Policy-Report-Only: script-src 'self' 'unsafe-inline'; report-uri resources/save-report.php?test=eval-allowed-in-report-only-mode-and-sends-report.php");
?>
<!DOCTYPE html>
<html>
<head>
    <script>
        if (window.internals)
            internals.settings.setExperimentalContentSecurityPolicyFeaturesEnabled(false);
    </script>
</head>
<body>
    <script>
        try {
            eval("alert('PASS: eval() allowed!')");
        } catch (e) {
            console.log('FAIL: eval() blocked!');
        }
    </script>
    <script src="resources/go-to-echo-report.js"></script>
</body>
</html>
