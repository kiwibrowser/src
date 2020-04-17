import unittest
from .. import tokenizer


class TestTokenizer(unittest.TestCase):

    def test_tokenizer_annotation(self):
        # Given
        code = "    @Override"

        # When
        tokens = list(tokenizer.tokenize(code))

        # Then
        self.assertEqual(len(tokens), 2)
        self.assertEqual(tokens[0].value, "@")
        self.assertEqual(tokens[1].value, "Override")
        self.assertEqual(type(tokens[0]), tokenizer.Annotation)
        self.assertEqual(type(tokens[1]), tokenizer.Identifier)

    def test_tokenizer_javadoc(self):
        # Given
        code = "/**\n" \
               " * See {@link BlockTokenSecretManager#setKeys(ExportedBlockKeys)}\n" \
               " */"

        # When
        tokens = list(tokenizer.tokenize(code))

        # Then
        self.assertEqual(len(tokens), 0)

    def test_tokenize_ignore_errors(self):
        # Given
        # character '#' was supposed to trigger an error of unknown token with a single line of javadoc
        code = " * See {@link BlockTokenSecretManager#setKeys(ExportedBlockKeys)}"

        # When
        tokens = list(tokenizer.tokenize(code, ignore_errors=True))

        # Then
        self.assertEqual(len(tokens), 11)

    def test_tokenize_line_comment_eof(self):
        # Given
        code = "  // This line comment at the end of the file has no newline"

        # When
        tokens = list(tokenizer.tokenize(code))

        # Then
        self.assertEqual(len(tokens), 0)

    def test_tokenize_comment_line_with_period(self):
        # Given
        code = "   * all of the servlets resistant to cross-site scripting attacks."

        # When
        tokens = list(tokenizer.tokenize(code))

        # Then
        self.assertEqual(len(tokens), 13)

    def test_tokenize_integer_at_end(self):
        # Given
        code = "nextKey = new BlockKey(serialNo, System.currentTimeMillis() + 3"

        # When
        tokens = list(tokenizer.tokenize(code, ignore_errors=True))

        # Then
        self.assertEqual(len(tokens), 14)

    def test_tokenize_float_at_end(self):
        # Given
        code = "nextKey = new BlockKey(serialNo, System.currentTimeMillis() + 3.0"

        # When
        tokens = list(tokenizer.tokenize(code, ignore_errors=True))

        # Then
        self.assertEqual(len(tokens), 14)

    def test_tokenize_hex_integer_at_end(self):
        # Given
        code = "nextKey = new BlockKey(serialNo, System.currentTimeMillis() + 0x3"

        # When
        tokens = list(tokenizer.tokenize(code, ignore_errors=True))

        # Then
        self.assertEqual(len(tokens), 14)

    def test_tokenize_token_position_after_comment(self):
        # Given
        code = """
public int function() {
    int a = 10;
    // some comment
    int b = 10;
}
        """

        # When
        tokens = list(tokenizer.tokenize(code))

        # Then
        # both token 6 and 11 are the "int" tokens of line 2 and 4
        self.assertEqual(tokens[6].position[1], 5)
        self.assertEqual(tokens[6].position[1], tokens[11].position[1])

    def test_tokenize_hex_float_integer_at_end(self):
        # Given
        code = "nextKey = new BlockKey(serialNo, System.currentTimeMillis() + 0x3.2p2"

        # When
        tokens = list(tokenizer.tokenize(code, ignore_errors=True))

        # Then
        self.assertEqual(len(tokens), 14)

    def test_string_delim_within_comment(self):

        # Given
        code = "* Returns 0 if it can't find the end \
                if (*itr == '\r') { \
                        int status;"

        # When
        tokens = list(tokenizer.tokenize(code, ignore_errors=True))

        # Then
        self.assertEqual(len(tokens), 8)

    def test_inline_comment_position(self):
        # Columns
        #                11111111112
        #       12345678901234567890
        code = "int /* comment */ j;"
        tokens = list(tokenizer.tokenize(code))

        int_token = tokens[0]
        j_token = tokens[1]
        semi_token = tokens[2]

        self.assertEqual(int_token.position.line, 1)
        self.assertEqual(int_token.position.column, 1)

        self.assertEqual(j_token.position.line, 1)
        self.assertEqual(j_token.position.column, 19)

        self.assertEqual(semi_token.position.line, 1)
        self.assertEqual(semi_token.position.column, 20)

    def test_multiline_inline_comment(self):
        code = """int /*
hello
world
*/ j;"""

        tokens = list(tokenizer.tokenize(code))
        token_int = tokens[0]
        token_j = tokens[1]

        self.assertEqual(token_int.position.line, 1)
        self.assertEqual(token_int.position.column, 1)

        self.assertEqual(token_j.position.line, 4)
        self.assertEqual(token_j.position.column, 4)

    def test_multiline_inline_comment_end_of_input(self):
        code = """int /*
hello
world
*/"""

        tokens = list(tokenizer.tokenize(code))
        token_int = tokens[0]

        self.assertEqual(token_int.position.line, 1)
        self.assertEqual(token_int.position.column, 1)

    def test_column_starts_at_one(self):
        code = """int j;
int k;
"""
        token = list(tokenizer.tokenize(code))
        self.assertEqual(token[0].position.column, 1)
        self.assertEqual(token[3].position.column, 1)

if __name__=="__main__":
    unittest.main()
