<?php header("X-XSS-Protection: 1"); ?>
<!DOCTYPE html>
<html>
<p>This is a page with a static script matching a post variable</p>
<script>yourname=hunter</script>
<script>
if (window.testRunner)
    testRunner.notifyDone();
</script>
</html>
