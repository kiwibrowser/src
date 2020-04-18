<?php
header("Content-Security-Policy-Report-Only: script-src 'self' 'unsafe-inline'");
?>
<!DOCTYPE html>
<html>
<head>
    <script>
        if (window.testRunner)
            testRunner.dumpAsText();
    </script>
</head>
<body>
    <script>
        try {
            eval("alert('PASS: eval() executed as expected.');");
        } catch(e) {
            alert("FAIL: eval() threw an exception.");
        }
    </script>
</body>
</html>
