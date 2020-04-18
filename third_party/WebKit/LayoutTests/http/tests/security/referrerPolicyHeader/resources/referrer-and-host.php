<?
header('Access-Control-Allow-Origin: *');
header('Content-Type: text/json');
?>
{
  "referrer": "<? echo $_SERVER['HTTP_REFERER']; ?>",
  "host": "<? echo $_SERVER['HTTP_HOST']; ?>"
}