<?php
if ($_SERVER['HTTP_ORIGIN'] !== "http://127.0.0.1:8000") {
  header("HTTP/1.1 403 Forbidden");
  exit;
}

header("Access-Control-Allow-Origin: http://127.0.0.1:8000");

$type = $_GET["type"];

if ($type === "img") {
  $image = "../../../resources/square100.png";
  header("Content-Type: image/png");
  header("Content-Length: " . filesize($image));
  readfile($image);
} else if ($type === "txt") {
  header("Content-Type: text/plain");
  print("hello");
  exit;
}
?>
