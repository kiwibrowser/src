<?php
$etag = "must-have-some-etag";
if (isset($_SERVER['HTTP_IF_NONE_MATCH']) && stripslashes($_SERVER['HTTP_IF_NONE_MATCH']) == $etag) {
    header("HTTP/1.0 304 Not Modified");
    die;
}
header("ETag: " . $etag);
header("Cache-Control: no-cache");
header("Content-Type: text/css");
?>
#markred { color: red; }
