<?php
header("Content-Security-Policy: sandbox allow-scripts");
?>
<script>
if (window.testRunner)
    testRunner.dumpAsText();
</script>
This test passes if it does alert pass.
<script>
console.log('PASS');
</script>
