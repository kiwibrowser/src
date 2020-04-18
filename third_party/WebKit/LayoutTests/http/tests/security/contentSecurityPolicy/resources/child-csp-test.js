var CROSS_ORIGIN_URL_ACF_HEADERS = "http://localhost:8000/security/contentSecurityPolicy/resources/respond-with-allow-csp-from-header.php";
var SAME_ORIGIN_URL_ACF_HEADERS  = "http://127.0.0.1:8000/security/contentSecurityPolicy/resources/respond-with-allow-csp-from-header.php";

var CROSS_ORIGIN_URL_CSP_HEADERS = "http://localhost:8000/security/contentSecurityPolicy/resources/respond-with-multiple-csp-headers.php";
var SAME_ORIGIN_URL_CSP_HEADERS  = "http://127.0.0.1:8000/security/contentSecurityPolicy/resources/respond-with-multiple-csp-headers.php";

var EXPECT_BLOCK = true;
var EXPECT_LOAD = false;

var CROSS_ORIGIN = true;
var SAME_ORIGIN = false;

function injectIframeWithCSP(url, shouldBlock, csp, t, urlId) {
    var i = document.createElement('iframe');
    i.src = url + "&id=" + urlId;
    i.csp = csp;

    var loaded = {};
    window.addEventListener("message", function (e) {
        if (e.source != i.contentWindow)
            return;
        if (e.data["loaded"])
            loaded[e.data["id"]] = true;
    });

    if (shouldBlock) {
        window.onmessage = function (e) {
            if (e.source != i.contentWindow)
                return;
            t.unreached_func('No message should be sent from the frame.');
        }
        i.onload = t.step_func(function () {
            // Delay the check until after the postMessage has a chance to execute.
            setTimeout(t.step_func_done(function () {
                 assert_equals(loaded[urlId], undefined);
            }), 1);
        });
    } else {
        document.addEventListener("securitypolicyviolation",
            t.unreached_func("There should not be any violations."));
        i.onload = t.step_func(function () {
            // Delay the check until after the postMessage has a chance to execute.
            setTimeout(t.step_func_done(function () {
                 assert_true(loaded[urlId]);
            }), 1);
        });
    }
    document.body.appendChild(i);
}

function generateUrlWithAllowCSPFrom(useCrossOrigin, allowCspFrom) {
    var url = useCrossOrigin ? CROSS_ORIGIN_URL_ACF_HEADERS : SAME_ORIGIN_URL_ACF_HEADERS;
    return url + "?allow_csp_from=" + allowCspFrom;
}

function generateUrlWithCSP(useCrossOrigin, csp) {
    var url = useCrossOrigin ? CROSS_ORIGIN_URL_CSP_HEADERS : SAME_ORIGIN_URL_CSP_HEADERS;
    return url + "?csp=" + csp;
}

function generateUrlWithCSPMultiple(useCrossOrigin, csp, csp2, cspReportOnly) {
    var url = useCrossOrigin ? CROSS_ORIGIN_URL_CSP_HEADERS : SAME_ORIGIN_URL_CSP_HEADERS;
    return url + "?csp=" + csp + "?csp2=" + csp2 + "?csp_report_only=" + cspReportOnly;
}
