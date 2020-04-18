<?php
require_once '../../resources/portabilityLayer.php';

$pingFilename = sys_get_temp_dir() . "/ping." . $_GET["test"];
while (!file_exists($pingFilename)) {
    usleep(10000);
    // file_exists() caches results, we want to invalidate the cache.
    clearstatcache();
}

echo "<html><body>\n";
$pingFile = fopen($pingFilename, 'r');
if ($pingFile) {
    echo "Ping sent successfully";
    while ($line = fgets($pingFile)) {
        echo "<br>";
        echo trim($line);
    }
    fclose($pingFile);
    unlink($pingFilename);
} else {
    echo "Ping not sent";
}
echo "<script>";
echo "if (window.testRunner)";
echo "    testRunner.notifyDone();";
echo "</script>";
echo "</body></html>";
?>
