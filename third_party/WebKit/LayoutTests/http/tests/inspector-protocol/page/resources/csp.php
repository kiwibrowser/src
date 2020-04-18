<?php
ob_start();
header("Content-Security-Policy: default-src 'self'");
echo "<script type='text/javascript'>window.__injected = 42;</script>";
