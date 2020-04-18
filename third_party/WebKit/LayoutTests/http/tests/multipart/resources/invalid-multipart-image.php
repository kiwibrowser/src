<?php
    $boundary = "cutHere";

    function sendPart($data)
    {
        global $boundary;
        echo("Content-Type: image/png\r\n\r\n");
        echo($data);
        echo("--$boundary\r\n");
        flush();
    }

    header("Content-Type: multipart/x-mixed-replace; boundary=$boundary");
    echo("--$boundary\r\n");
    ob_end_flush();

    $invalidImage = "Invalid PNG data";
    sendPart($invalidImage);

    $validImage = file_get_contents("2x2-green.png");
    sendPart($validImage);
?>
