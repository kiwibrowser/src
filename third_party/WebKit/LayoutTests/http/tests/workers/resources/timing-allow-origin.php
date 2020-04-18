<?php
  header("Content-Type: text/plain");
  header("Access-Control-Allow-Origin: *");
  if (isset($_GET["origin"]))
    header("Timing-Allow-Origin: {$_GET['origin']}");
?>
Test file content.
