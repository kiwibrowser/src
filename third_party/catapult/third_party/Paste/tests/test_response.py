from paste.response import *

def test_replace_header():
    h = [('content-type', 'text/plain'),
         ('x-blah', 'foobar')]
    replace_header(h, 'content-length', '10')
    assert h[-1] == ('content-length', '10')
    replace_header(h, 'Content-Type', 'text/html')
    assert ('content-type', 'text/html') in h
    assert ('content-type', 'text/plain') not in h

