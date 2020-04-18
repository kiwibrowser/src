<?php
header("Link: <   http://wut.com.test/>; rel=dns-prefetch");
header("Content-Type: application/javascript");
?>
if (window.testRunner)
    testRunner.notifyDone();
