<?php
$pingFilename = sys_get_temp_dir() . "/ping." . $_GET["test"];
unlink($pingFilename);
?>
