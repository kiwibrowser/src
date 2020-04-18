import six

from paste.request import *
from paste.util.multidict import MultiDict

def test_parse_querystring():
    e = {'QUERY_STRING': 'a=1&b=2&c=3&b=4'}
    d = parse_querystring(e)
    assert d == [('a', '1'), ('b', '2'), ('c', '3'), ('b', '4')]
    assert e['paste.parsed_querystring'] == (
        (d, e['QUERY_STRING']))
    e = {'QUERY_STRING': 'a&b&c=&d=1'}
    d = parse_querystring(e)
    assert d == [('a', ''), ('b', ''), ('c', ''), ('d', '1')]

def make_post(body):
    e = {
        'CONTENT_TYPE': 'application/x-www-form-urlencoded',
        'CONTENT_LENGTH': str(len(body)),
        'REQUEST_METHOD': 'POST',
        'wsgi.input': six.BytesIO(body),
        }
    return e

def test_parsevars():
    e = make_post(b'a=1&b=2&c=3&b=4')
    #cur_input = e['wsgi.input']
    d = parse_formvars(e)
    assert isinstance(d, MultiDict)
    assert d == MultiDict([('a', '1'), ('b', '2'), ('c', '3'), ('b', '4')])
    assert e['paste.parsed_formvars'] == (
        (d, e['wsgi.input']))
    # XXX: http://trac.pythonpaste.org/pythonpaste/ticket/125
    #assert e['wsgi.input'] is not cur_input
    #cur_input.seek(0)
    #assert e['wsgi.input'].read() == cur_input.read()
