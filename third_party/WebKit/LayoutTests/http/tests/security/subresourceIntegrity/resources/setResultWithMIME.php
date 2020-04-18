<?php
$mime = $_GET["mime"];
$value = $_GET["value"];
header("Content-Type: $mime");
echo "result = $value";
?>
