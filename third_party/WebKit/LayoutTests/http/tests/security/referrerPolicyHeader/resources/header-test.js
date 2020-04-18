async_test(function () {
    var test = this;
    window.addEventListener("message", test.step_func(function (event) {
        assert_equals(event.data, expectedReferrer);
        test.done();
    }));

    var iframe = document.createElement("iframe");
    if (navigateTo === "downgrade")
        iframe.src = get_host_info().HTTP_ORIGIN + "/security/referrerPolicyHeader/resources/postmessage-referrer.html";
    else if (navigateTo === "same-origin")
        iframe.src = "/security/referrerPolicyHeader/resources/postmessage-referrer.html";
    else if (navigateTo === "cross-origin-no-downgrade")
        iframe.src = get_host_info().AUTHENTICATED_ORIGIN + "/security/referrerPolicyHeader/resources/postmessage-referrer.html";
    else if (navigateTo === "no-referrer-on-redirect")
        iframe.src = "/security/referrerPolicyHeader/resources/no-referrer-on-redirect.php";
    document.body.appendChild(iframe);
}, "Referrer policy header - " + policy);
