<?php
header("Content-Security-Policy-Report-Only: script-src 'unsafe-inline';");
?>
<!DOCTYPE html>
<html>
<head>
    <script>
        if (window.testRunner) {
          testRunner.dumpAsText();
          testRunner.dumpPingLoaderCallbacks();
        }
        if (window.internals)
          internals.settings.setExperimentalContentSecurityPolicyFeaturesEnabled(false);
    </script>
</head>
<body>
    <p>This test passes if a console message is present, warning about the missing 'report-uri' directive.</p>
</body>
</html>
