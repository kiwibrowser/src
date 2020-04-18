<?php

    $fileName = $_GET["name"];
    $type = $_GET["type"];

    $fileSize = filesize($fileName);
    $start = 0;
    $end = $fileSize - 1;
    $contentRange = $_SERVER["HTTP_RANGE"];
    if (isset($contentRange) && empty($_GET["norange"])) {
        $range = explode("-", substr($contentRange, strlen("bytes=")));
        $start = intval($range[0]);
        if (!empty($range[1]))
            $end = intval($range[1]);
        $httpStatus = "206 Partial Content";
    } else
        $httpStatus = "200 OK";

    header("Status: " . $httpStatus);
    header("HTTP/1.1 " . $httpStatus);
    header("Last-Modified: " . gmdate("D, d M Y H:i:s") . " GMT");
    header("Pragma: no-cache");
    header("Etag: " . '"' . $fileSize . "-" . filemtime($fileName) . '"');
    header("Content-Type: " . $type);
    header("Content-Length: " . ($end - $start) + 1);
    if ($contentRange && empty($_GET["norange"])) {
        header("Accept-Ranges: bytes");
        header("Content-Range: bytes " . $start . "-" . $end . "/" . $fileSize);
    }
    if (isset($_GET['cors_allow_origin'])) {
        header("Access-Control-Allow-Origin: " . $_GET['cors_allow_origin']);
    }
    header("Connection: close");

    $chunkSize = 1024 * 256;
    $offset = $start;

    $fn = fopen($fileName, "rb");
    fseek($fn, $offset, 0);

    while (!feof($fn) && $offset <= $end && connection_status() == 0) {
        $readSize = min($chunkSize, ($end - $offset) + 1);
        $buffer = fread($fn, $readSize);
        print($buffer);
        flush();
        $offset += $chunkSize;
    }
    fclose($fn);

    exit;
?>
