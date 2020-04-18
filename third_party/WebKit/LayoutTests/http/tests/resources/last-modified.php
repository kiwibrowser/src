<?php
    $date = $_GET['date'];
    $expected = $_GET['expected'];
    header("Last-Modified: $date");
?>
<script>
var expected = '<?=$expected?>';
var lastModified = document.lastModified;

// If expected is 'illformed-date', then the Last-Modified: header
// is assumed to contain an unparseable date. For which we're supposed
// to return the current time per spec. Simply verify that by checking
// that the |lastModified| parses.
if (expected === lastModified ||
    (expected === "illformed-date" && !isNaN(Date.parse(lastModified)))) {
    document.write('PASS');
} else {
    document.write('FAIL: expect ' + expected + ', but got ' + lastModified);
}
</script>
