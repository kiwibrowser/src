<?php

if (isset($_GET['origin'])) {
    header("Access-Control-Allow-Origin: " . $_GET['origin']);
} else if (isset($_GET['origins'])) {
    $origins = explode(',', $_GET['origins']);
    for ($i = 0; $i < sizeof($origins); ++$i)
         header("Access-Control-Allow-Origin: " . $origins[$i], false);
}

if (isset($_GET['headers']))
    header("Access-Control-Allow-Headers: {$_GET['headers']}");
if (isset($_GET['methods']))
    header("Access-Control-Allow-Methods: {$_GET['methods']}");

foreach ($_SERVER as $name => $value)
{
    if (substr($name, 0, 5) == 'HTTP_')
    {
        $name = strtolower(str_replace('_', '-', substr($name, 5)));
        $headers[$name] = $value;
    } else if ($name == "CONTENT_TYPE") {
        $headers["content-type"] = $value;
    } else if ($name == "CONTENT_LENGTH") {
        $headers["content-length"] = $value;
    }
}

echo json_encode( $headers );
