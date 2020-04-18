<?php
header("Content-Security-Policy: sandbox allow-scripts");
?>
<!DOCTYPE html>
<html>
<head>
    <script src="/resources/testharness.js"></script>
    <script src="/resources/testharnessreport.js"></script>
</head>
<body>
    <script>
        test(function () {
            var result = alert("Yay!");
            assert_equals(result, undefined);
        }, "alert() returns synchronously in a sandboxed page without blocking on user input.");

        test(function () {
            var result = print();
            assert_equals(result, undefined);
        }, "print() returns synchronously in a sandboxed page without blocking on user input.");

        test(function () {
            var result = document.execCommand('print', false, null);
            assert_false(result);
        }, "execCommand('print', ...) returns synchronously in a sandboxed page without blocking on user input.");

        test(function () {
            var result = confirm("Question?");
            assert_equals(result, false);
        }, "confirm() returns 'false' synchronously in a sandboxed page without blocking on user input.");

        test(function () {
            var result = prompt("Question?");
            assert_equals(result, null);
        }, "prompt() returns 'null' synchronously in a sandboxed page without blocking on user input.");
    </script>
</body>
</html>
