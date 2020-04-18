<?php

    $fileName = "incrementer.wasm";
    if (isset($_GET['name'])) {
       $fileName = $_GET['name'];
    }

    header("Content-Type: application/wasm");
    require($fileName);
?>
