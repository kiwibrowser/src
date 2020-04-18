from paste.util import quoting
import six
import unittest

class TestQuoting(unittest.TestCase):
    def test_html_unquote(self):
        self.assertEqual(quoting.html_unquote(b'&lt;hey&nbsp;you&gt;'),
                         u'<hey\xa0you>')
        self.assertEqual(quoting.html_unquote(b''),
                         u'')
        self.assertEqual(quoting.html_unquote(b'&blahblah;'),
                         u'&blahblah;')
        self.assertEqual(quoting.html_unquote(b'\xe1\x80\xa9'),
                         u'\u1029')

    def test_html_quote(self):
        self.assertEqual(quoting.html_quote(1),
                         '1')
        self.assertEqual(quoting.html_quote(None),
                         '')
        self.assertEqual(quoting.html_quote('<hey!>'),
                         '&lt;hey!&gt;')
        if six.PY3:
            self.assertEqual(quoting.html_quote(u'<\u1029>'),
                             u'&lt;\u1029&gt;')
        else:
            self.assertEqual(quoting.html_quote(u'<\u1029>'),
                             '&lt;\xe1\x80\xa9&gt;')
