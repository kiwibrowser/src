import unittest

from .. import javadoc


class TestJavadoc(unittest.TestCase):
    def test_empty_comment(self):
        javadoc.parse('/** */')
        javadoc.parse('/***/')
        javadoc.parse('/**\n *\n */')
        javadoc.parse('/**\n *\n *\n */')

if __name__ == "__main__":
    unittest.main()
