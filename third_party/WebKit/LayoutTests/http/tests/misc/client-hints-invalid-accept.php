<?php
    header("ACCEPT-CH: DPRW");
?>
<!DOCTYPE html>
<script src="../resources/testharness.js"></script>
<script src="../resources/testharnessreport.js"></script>
<body>
    <script>
        var t = async_test('Client-Hints not sent when Accept-CH header is present but invalid');
        var unreached = function() {
            assert_unreached("Image should not have loaded.");
        };

        var loadDeviceMemoryImage = function() {
            var img = new Image();
            img.src = 'resources/image-checks-for-device-memory.php';
            img.onload = t.step_func(unreached);
            img.onerror = t.step_func(function(){ t.done(); });
            document.body.appendChild(img);
        };
        var loadRWImage = function() {
            var img = new Image();
            img.src = 'resources/image-checks-for-rw.php';
            img.onload = t.step_func(unreached);
            img.onerror = t.step_func(loadDeviceMemoryImage);
            document.body.appendChild(img);
        };
        t.step(function() {
            var img = new Image();
            img.src = 'resources/image-checks-for-dpr.php';
            img.onload = t.step_func(unreached);
            img.onerror = t.step_func(loadRWImage);
            document.body.appendChild(img);
        });
    </script>
</body>
