<?php
if ($_SERVER["HTTP_ORIGIN"] == "http://example.com") {
  header("Access-Control-Allow-Origin: http://example.com");

  if ($_SERVER["REQUEST_METHOD"] == "OPTIONS") {
    header("Access-Control-Allow-Headers: X-Custom-Header");
  } else {
    print $_SERVER["HTTP_X_CUSTOM_HEADER"];
  }
} else {
  header("HTTP/1.1 400");
}
?>
