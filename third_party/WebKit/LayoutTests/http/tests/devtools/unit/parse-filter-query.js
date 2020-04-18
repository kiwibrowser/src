(async function () {
    TestRunner.addResult("This tests a utility's ability to parse filter queries.");

    var keys = ["key1", "key2"];
    var queries = [
        "text",
        "text with spaces",
        "-",
        "-text",
        "//",
        "/regex/",
        "/regex/ /another/",
        "/complex\/regex/",
        "/regex/ text",
        "key1:foo",
        "-key1:foo",
        "key1:foo key2:bar",
        "-key1:foo key2:bar",
        "key1:foo -key2:bar",
        "-key1:foo -key2:bar",
        "key1:/regex/",
        "key1:foo innerText key2:bar",
        "bar key1 foo",
        "bar key1:foo",
        "bar key1:foo baz",
        "bar key1:foo yek:roo baz",
        "bar key1:foo -yek:roo baz",
        "bar baz key1:foo goo zoo",
        "bar key1:key1:foo",
        "bar :key1:foo baz",
        "bar -:key1:foo baz",
        "bar key1:-foo baz",
    ];

    var parser = new TextUtils.FilterParser(keys);
    for (var query of queries) {
        var result = parser.parse(query);
        TestRunner.addResult("\nQuery: " + query);
        for (var descriptor of result) {
            if (descriptor.regex)
                descriptor.regex = descriptor.regex.source;
            TestRunner.addResult(JSON.stringify(descriptor));
        }
    }
    TestRunner.completeTest();
})();