<?php
// Set max-age to non-zero value so the next update will be served from cache.
header('Cache-Control: max-age=3600');
header('Content-Type:application/javascript');
// Return a different script for each access.
echo '// ' . microtime();
?>
