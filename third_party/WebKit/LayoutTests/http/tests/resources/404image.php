<?
header("HTTP/1.1 404 Not Found");
header("Content-Type: image/png");
header("Last-Modified: Thu, 01 Jun 2006 06:09:43 GMT");
echo(file_get_contents("square100.png"));
?>
