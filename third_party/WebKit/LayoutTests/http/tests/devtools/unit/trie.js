(async function() {
    var trie;
    TestRunner.addResult(`Verify "trie" functionality.`);

    TestRunner.runTests([
        function testAddWord()
        {
            var trie = new Common.Trie();
            addWord(trie, "hello");
            hasWord(trie, "he");
            hasWord(trie, "hello");
            hasWord(trie, "helloo");
        },

        function testAddWords()
        {
            var trie = new Common.Trie();
            addWord(trie, "foo");
            addWord(trie, "bar");
            addWord(trie, "bazz");
            hasWord(trie, "f");
            hasWord(trie, "ba");
            hasWord(trie, "baz");
            hasWord(trie, "bar");
            hasWord(trie, "bazz");
        },

        function testRemoveWord()
        {
            var trie = new Common.Trie();
            addWord(trie, "foo");
            removeWord(trie, "f");
            removeWord(trie, "fo");
            removeWord(trie, "fooo");
            hasWord(trie, "foo");
            removeWord(trie, "foo");
            hasWord(trie, "foo");
        },

        function testAddAfterRemove()
        {
            var trie = new Common.Trie();
            addWord(trie, "foo");
            removeWord(trie, "foo");
            addWord(trie, "bar");
            hasWord(trie, "foo");
            hasWord(trie, "bar");
        },

        function testWordOverwrite()
        {
            var trie = new Common.Trie();
            addWord(trie, "foo");
            addWord(trie, "foo");
            removeWord(trie, "foo");
            hasWord(trie, "foo");
        },

        function testRemoveNonExisting()
        {
            var trie = new Common.Trie();
            addWord(trie, "foo");
            removeWord(trie, "bar");
            removeWord(trie, "baz");
            hasWord(trie, "foo");
        },

        function testEmptyWord()
        {
            var trie = new Common.Trie();
            addWord(trie, "");
            hasWord(trie, "");
            removeWord(trie, "");
            hasWord(trie, "");
        },

        function testAllWords()
        {
            var trie = new Common.Trie();
            addWord(trie, "foo");
            addWord(trie, "bar");
            addWord(trie, "bazzz");
            words(trie);
            words(trie, "f");
            words(trie, "g");
            words(trie, "b");
            words(trie, "ba");
            words(trie, "bar");
            words(trie, "barz");
            words(trie, "baz");
        },

        function testOneCharWords()
        {
            var trie = new Common.Trie();
            addWord(trie, "a");
            addWord(trie, "b");
            addWord(trie, "c");
            words(trie);
        },

        function testChainWords()
        {
            var trie = new Common.Trie();
            addWord(trie, "f");
            addWord(trie, "fo");
            addWord(trie, "foo");
            addWord(trie, "foo");
            words(trie);
        },

        function testClearTrie()
        {
            var trie = new Common.Trie();
            addWord(trie, "foo");
            addWord(trie, "bar");
            words(trie);
            clear(trie);
            words(trie);
        },

        function testLongestPrefix()
        {
            var trie = new Common.Trie();
            addWord(trie, "fo");
            addWord(trie, "food");
            longestPrefix(trie, "fear", false);
            longestPrefix(trie, "fear", true);
            longestPrefix(trie, "football", false);
            longestPrefix(trie, "football", true);
            longestPrefix(trie, "bar", false);
            longestPrefix(trie, "bar", true);
            longestPrefix(trie, "foo", false);
            longestPrefix(trie, "foo", true);
        },
    ]);

    function hasWord(trie, word)
    {
        TestRunner.addResult(`trie.has("${word}") = ${trie.has(word)}`);
    }

    function addWord(trie, word)
    {
        TestRunner.addResult(`trie.add("${word}")`);
        trie.add(word);
    }

    function removeWord(trie, word)
    {
        TestRunner.addResult(`trie.remove("${word}") = ${trie.remove(word)}`);
    }

    function words(trie, prefix)
    {
        var title = prefix ? `trie.words("${prefix}")` : `trie.words()`;
        var words = trie.words(prefix);
        var text = words.length ? `[\n    ${words.join(",\n    ")}\n]` : "[]";
        TestRunner.addResult(title + " = " + text);
    }

    function clear(trie)
    {
        trie.clear();
        TestRunner.addResult("trie.clear()");
    }

    function longestPrefix(trie, word, fullWordOnly)
    {
        TestRunner.addResult(`trie.longestPrefix("${word}", ${fullWordOnly}) = "${trie.longestPrefix(word, fullWordOnly)}"`);
    }
})();