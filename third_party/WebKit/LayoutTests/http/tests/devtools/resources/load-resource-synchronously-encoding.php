<?php
    $charset = $_GET["charset"];
    $intCharset = intval($charset);
    if ($intCharset > 0)
        $charset = "windows-" . $intCharset;
    $contentTypeHeader = "Content-Type: text/plain";
    if ($_GET["needEncoding"] == "true")
        $contentTypeHeader .= "; charset=" . $charset;
    header($contentTypeHeader);

    switch ($charset) {
    case "US-ASCII":
        $text = "Test";
        break;
    case "windows-1251":
        $text = "\xD2\xE5\xF1\xF2";
        break;
    case "windows-1252":
        $text = "Test \xC4\xE4\x9A\xDF";
        break;
    case "utf-8":
        $text = "Тест";
        break;
    default:
        $text = "Invalid encoding";
        break;
    }

    echo $charset . ": " . $text;
    flush();
?>
