description("This tests that ScriptRegexp matching fails when running into an exceptional condition (stack overflow) without crashing.");

function runTest() {
    var input = document.createElement("input");
    input.type = "email";

    function repeatedlySet(r) {
        // Consume stack until failure.
        input.value = r;
        repeatedlySet(r);
    }
    repeatedlySet("an@example.com");
}
shouldThrow("runTest()");
