<?php
header("Link: <   http://wut.com.test/>; rel=dns-prefetch");
?>
<!DOCTYPE html>
I'm an iframe with a Link header
<script>
    if (window.testRunner)
        testRunner.notifyDone();
</script>
