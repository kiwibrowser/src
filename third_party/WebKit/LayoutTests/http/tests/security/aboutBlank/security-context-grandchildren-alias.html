<!DOCTYPE html>
<html>
<head>
    <script src="/resources/testharness.js"></script>
    <script src="/resources/testharnessreport.js"></script>
</head>
<body>
    <script>
        if (document.location.hostname == "127.0.0.1") {
            document.location.hostname = "subdomain.example.test";
        } else {
            var t = async_test("document.domain behavior of open grandchildren 'about:blank' matches grandparent.");

            t.step(function () {
                var i = document.createElement('iframe');
                i.src = 'resources/iframe-with-about-blank-children.html';
                i.onload = t.step_func(function () {
                    var doc0 = frames[0].frames[0].document;
                    var doc1 = frames[0].frames[1].document;

                    assert_equals(doc0.domain, document.domain);
                    assert_equals(doc1.domain, document.domain);

                    doc0.open();
                    doc1.open();
                    assert_equals(doc0.domain, document.domain);
                    assert_equals(doc1.domain, document.domain);

                    document.domain = 'example.test';
                    assert_equals(doc0.domain, document.domain);
                    assert_equals(doc1.domain, document.domain);

                    doc0.close();
                    doc1.close();
                    assert_equals(doc0.domain, document.domain);
                    assert_equals(doc1.domain, document.domain);

                    t.done();
                });
                document.body.appendChild(i);
            });
        }
    </script>
</body>
</html>
