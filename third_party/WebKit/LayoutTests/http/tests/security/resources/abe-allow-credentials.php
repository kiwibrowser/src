<?php
$cookie_check = $_GET["Cookie"];
if (isset($cookie_check)) {
  if ($cookie_check == "NotSet") {
    if (isset($_COOKIE['TestCookie'])) {
      header("HTTP/1.0 404 Not Found");
      exit;
    }
  } else if ($cookie_check != $_COOKIE['TestCookie']) {
    header("HTTP/1.0 404 Not Found");
    exit;
  }
}

header("Access-Control-Allow-Origin: http://127.0.0.1:8000");
header("Access-Control-Allow-Credentials: true");

$name = 'abe.png';
$fp = fopen($name, 'rb');
header("Content-Type: image/png");
header("Content-Length: " . filesize($name));

fpassthru($fp);
exit;
?>
