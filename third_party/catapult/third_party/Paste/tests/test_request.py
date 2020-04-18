# (c) 2005 Ben Bangert
# This module is part of the Python Paste Project and is released under
# the MIT License: http://www.opensource.org/licenses/mit-license.php
from paste.fixture import *
from paste.request import *
from paste.wsgiwrappers import WSGIRequest
import six

def simpleapp(environ, start_response):
    status = '200 OK'
    response_headers = [('Content-type','text/plain')]
    start_response(status, response_headers)
    request = WSGIRequest(environ)
    body = [
        'Hello world!\n', 'The get is %s' % str(request.GET),
        ' and Val is %s\n' % request.GET.get('name'),
        'The languages are: %s\n' % request.languages,
        'The accepttypes is: %s\n' % request.match_accept(['text/html', 'application/xml'])]
    if six.PY3:
        body = [line.encode('utf8')  for line in body]
    return body

def test_gets():
    app = TestApp(simpleapp)
    res = app.get('/')
    assert 'Hello' in res
    assert "get is MultiDict([])" in res

    res = app.get('/?name=george')
    res.mustcontain("get is MultiDict([('name', 'george')])")
    res.mustcontain("Val is george")

def test_language_parsing():
    app = TestApp(simpleapp)
    res = app.get('/')
    assert "The languages are: ['en-us']" in res

    res = app.get('/', headers={'Accept-Language':'da, en-gb;q=0.8, en;q=0.7'})
    assert "languages are: ['da', 'en-gb', 'en', 'en-us']" in res

    res = app.get('/', headers={'Accept-Language':'en-gb;q=0.8, da, en;q=0.7'})
    assert "languages are: ['da', 'en-gb', 'en', 'en-us']" in res

def test_mime_parsing():
    app = TestApp(simpleapp)
    res = app.get('/', headers={'Accept':'text/html'})
    assert "accepttypes is: ['text/html']" in res

    res = app.get('/', headers={'Accept':'application/xml'})
    assert "accepttypes is: ['application/xml']" in res

    res = app.get('/', headers={'Accept':'application/xml,*/*'})
    assert "accepttypes is: ['text/html', 'application/xml']" in res

def test_bad_cookie():
    env = {}
    env['HTTP_COOKIE'] = '070-it-:><?0'
    assert get_cookie_dict(env) == {}
    env['HTTP_COOKIE'] = 'foo=bar'
    assert get_cookie_dict(env) == {'foo': 'bar'}
    env['HTTP_COOKIE'] = '...'
    assert get_cookie_dict(env) == {}
    env['HTTP_COOKIE'] = '=foo'
    assert get_cookie_dict(env) == {}
    env['HTTP_COOKIE'] = '?='
    assert get_cookie_dict(env) == {}
