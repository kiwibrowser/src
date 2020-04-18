<?php
    header("ACCEPT-CH: DPR, Width, Viewport-Width");
?>
<!DOCTYPE html>
<script src="../../../resources/testharness.js"></script>
<script src="../../../resources/testharnessreport.js"></script>
<script>
    var t = async_test("Verify that hints were not sent on iframe subresources.");
    window.addEventListener("message", t.step_func(function (message) {
        console.log(message.data);
        assert_equals(message.data, "success");
        t.done();
    }));
</script>
<iframe src="/resources/iframe-no-accept-ch.php"></iframe>
