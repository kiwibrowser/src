description(
'This test checks if CSS string values are correctly serialized when they contain binary characters.'
);

var inputs = ["'\\0\\1\\2\\3\\4\\5\\6\\7\\8\\9\\a\\b\\c\\d\\e\\f'",
              "'\\10\\11\\12\\13\\14\\15\\16\\17\\18\\19\\1a\\1b\\1c\\1d\\1e\\1f\\7f'",
              "'\\A\\B\\C\\D\\E\\F\\1A\\1B\\1C\\1D\\1E\\1F\\7F'",
              "'\\3 \\1 \\2 '",
              "'\\3  \\1  \\2  '",
              "'\\3   \\1   \\2   '",
              "'\\00000f\\00000g'",
              "'\\1 0\\1 1\\1 2\\1 3\\1 4\\1 5\\1 6\\1 7\\1 8\\1 9'",
              "'\\1 A\\1 B\\1 C\\1 D\\1 E\\1 F\\1 G'",
              "'\\1 a\\1 b\\1 c\\1 d\\1 e\\1 f\\1 g'"];
// Null is replaced with U+FFFD as per css-syntax
var expected = ["\"\uFFFD\\1 \\2 \\3 \\4 \\5 \\6 \\7 \\8 \\9 \\a \\b \\c \\d \\e \\f \"",
                "\"\\10 \\11 \\12 \\13 \\14 \\15 \\16 \\17 \\18 \\19 \\1a \\1b \\1c \\1d \\1e \\1f \\7f \"",
                "\"\\a \\b \\c \\d \\e \\f \\1a \\1b \\1c \\1d \\1e \\1f \\7f \"",
                "\"\\3 \\1 \\2 \"",          // No space after each control character.
                "\"\\3  \\1  \\2  \"",    // One space delimiter (that will be ignored by the CSS parser), plus one actual space.
                "\"\\3   \\1   \\2   \"", // One space delimiter, plus two actual spaces.
                "\"\\f \uFFFDg\"",
                "\"\\1 0\\1 1\\1 2\\1 3\\1 4\\1 5\\1 6\\1 7\\1 8\\1 9\"", // Need a space before [0-9A-Fa-f], but not before [Gg].
                "\"\\1 A\\1 B\\1 C\\1 D\\1 E\\1 F\\1 G\"",
                "\"\\1 a\\1 b\\1 c\\1 d\\1 e\\1 f\\1 g\""];

var testElement = document.createElement('div');
for (var i = 0; i < inputs.length; ++i) {
    testElement.style.fontFamily = inputs[i];
    shouldBeEqualToString('testElement.style.fontFamily', expected[i]);
}
