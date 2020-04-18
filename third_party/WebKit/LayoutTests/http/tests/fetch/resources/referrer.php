<?php
$referrer = '[no-referrer]';
if (isset($_SERVER['HTTP_REFERER'])) {
    $referrer = $_SERVER['HTTP_REFERER'];
}
header('Access-Control-Allow-Origin: *');
header('Content-Type: application/json');
$arr = array('referrer' => $referrer);
echo json_encode($arr);

?>
