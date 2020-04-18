<?php
header('HTTP/1.1 200');
header('Content-Type: application/json');
echo json_encode(getallheaders());
?>