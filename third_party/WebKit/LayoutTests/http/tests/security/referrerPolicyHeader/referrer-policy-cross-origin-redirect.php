<?php
header("Referrer-Policy: origin-when-cross-origin");
?>
<!DOCTYPE html>
<head>
  <script src="/resources/testharness.js"></script>
  <script src="/resources/testharnessreport.js"></script>
  <script src="/resources/get-host-info.js"></script>
</head>
<body>
</body>
<script>
    // Tests that once a referrer has been stripped while following one leg of a redirected request, the referrer doesn't get reconstructed on subsequent legs.
    async_test(function () {
        var test = this;
        fetch(get_host_info().AUTHENTICATED_ORIGIN + "/security/referrerPolicyHeader/resources/redirect-to.php?location=" + document.location.origin + "/security/referrerPolicyHeader/resources/referrer-and-host.php").then(test.step_func(function(response) {
            response.json().then(test.step_func(function (result) {
                // Sanity check that the request redirect back to the same origin as this page.
                assert_equals(result.host, document.location.host);
                // The origin should have been stripped because the initial leg of the request was cross-origin.
                assert_equals(document.location.origin + "/", result.referrer);
                test.done();
            }));
        }));
    });
</script>
</html>
