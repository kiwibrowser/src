<?php
ob_start();
header('Access-Control-Allow-Headers: origin, content-type, accept, authorization');
header('Access-Control-Allow-Methods: GET, POST, PUT, PATCH, DELETE, OPTIONS, HEAD');
header('Access-Control-Allow-Origin: *');
if ($_SERVER['REQUEST_METHOD'] == 'OPTIONS')
  exit;

echo 'Some content';
