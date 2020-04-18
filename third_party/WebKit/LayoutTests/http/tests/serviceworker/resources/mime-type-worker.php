<?php
if ($_GET['mime']) {
  header("Content-Type: " . $_GET['mime']);
} else {
  // Set a header and remove it to override the default 'text/html'.
  header("Content-Type:");
  header_remove("Content-Type");
}
?>
