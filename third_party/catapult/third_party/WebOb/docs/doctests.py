import unittest
import doctest

def test_suite():
    flags = doctest.ELLIPSIS|doctest.NORMALIZE_WHITESPACE
    return unittest.TestSuite((
        doctest.DocFileSuite('test_request.txt', optionflags=flags),
        doctest.DocFileSuite('test_response.txt', optionflags=flags),
        doctest.DocFileSuite('test_dec.txt', optionflags=flags),
        doctest.DocFileSuite('do-it-yourself.txt', optionflags=flags),
        doctest.DocFileSuite('file-example.txt', optionflags=flags),
        doctest.DocFileSuite('index.txt', optionflags=flags),
        doctest.DocFileSuite('reference.txt', optionflags=flags),
        ))

if __name__ == '__main__':
    unittest.main(defaultTest='test_suite')
