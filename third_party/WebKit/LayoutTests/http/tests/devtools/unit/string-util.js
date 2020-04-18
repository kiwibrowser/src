(async function() {
    TestRunner.runTests([
        function testEmptyPrefixSuffix()
        {
            const tokens = String.tokenizeFormatString(`%c%s`, {
                c: () => {},
                s: () => {}
            });
            TestRunner.addObject(tokens);
        },
    ]);
})();
