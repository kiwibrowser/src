<?php
if (isset($_SERVER['HTTP_SAVE_DATA'])) {
    echo 'Save-Data: ' . $_SERVER['HTTP_SAVE_DATA'];
} else {
    echo 'No Save-Data';
}
?>
