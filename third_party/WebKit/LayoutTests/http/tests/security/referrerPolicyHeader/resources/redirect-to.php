<?php
header('Access-Control-Allow-Origin: *');
header("Location: " . $_GET["location"]);
if ($_GET["referrerpolicy"]) {
  header("Referrer-Policy: " . $_GET["referrerpolicy"]);
}
?>