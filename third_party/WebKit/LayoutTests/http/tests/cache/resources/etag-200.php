<?
// Returns response headers that cause revalidation, and returns 200 for
// revalidating requests to test failed revalidation.
header('ETag: foo');
header('Cache-control: max-age=0');
echo rand();
?>
