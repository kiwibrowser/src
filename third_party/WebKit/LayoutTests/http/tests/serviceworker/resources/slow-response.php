<?php
function put_chunk($txt) {
  echo sprintf("%x\r\n", strlen($txt));
  echo "$txt\r\n";
}

header("Content-type: text/html; charset=UTF-8");
header("Transfer-encoding: chunked");
flush();

for ($i = 0; $i < 10000; $i++) {
  put_chunk("$i<br>");
  ob_flush();
  flush();
  usleep(1000000);
}
echo "0\r\n\r\n";

?>
