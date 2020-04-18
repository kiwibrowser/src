# -*- coding: utf-8 -*-
# (c) 2007 Philip Jenvey; written for Paste (http://pythonpaste.org)
# Licensed under the MIT license: http://www.opensource.org/licenses/mit-license.php
import cgi
from paste.fixture import TestApp
from paste.wsgiwrappers import WSGIRequest, WSGIResponse
import six

class AssertApp(object):
    def __init__(self, assertfunc):
        self.assertfunc = assertfunc

    def __call__(self, environ, start_response):
        start_response('200 OK', [('Content-type','text/plain')])
        self.assertfunc(environ)
        return ['Passed']

no_encoding = object()
def valid_name(name, encoding=no_encoding, post=False):
    def assert_valid_name(environ):
        if encoding is not no_encoding:
            WSGIRequest.defaults._push_object(dict(content_type='text/html',
                                                   charset=encoding))
        try:
            request = WSGIRequest(environ)
            if post:
                params = request.POST
            else:
                params = request.GET
            assert params['name'] == name
            assert request.params['name'] == name
        finally:
            if encoding is not no_encoding:
                WSGIRequest.defaults._pop_object()
    return assert_valid_name

def test_wsgirequest_charset():
    # Jose, 'José'
    app = TestApp(AssertApp(assertfunc=valid_name(u'José', encoding='UTF-8')))
    res = app.get('/?name=Jos%C3%A9')

    # Tanaka, '田中'
    app = TestApp(AssertApp(assertfunc=valid_name(u'田中', encoding='UTF-8')))
    res = app.get('/?name=%E7%94%B0%E4%B8%AD')

    # Nippon (Japan), '日本'
    app = TestApp(AssertApp(assertfunc=valid_name(u'日本', encoding='UTF-8',
                                                  post=True)))
    res = app.post('/', params=dict(name='日本'))

    # WSGIRequest will determine the charset from the Content-Type header when
    # unicode is expected.
    # No encoding specified: not expecting unicode
    app = TestApp(AssertApp(assertfunc=valid_name('日本', post=True)))
    content_type = 'application/x-www-form-urlencoded; charset=%s'
    res = app.post('/', params=dict(name='日本'),
                   headers={'content-type': content_type % 'UTF-8'})

    # Encoding specified: expect unicode. Shiftjis is the default encoding, but
    # params become UTF-8 because the browser specified so
    app = TestApp(AssertApp(assertfunc=valid_name(u'日本', post=True,
                                                  encoding='shiftjis')))
    res = app.post('/', params=dict(name='日本'),
                   headers={'content-type': content_type % 'UTF-8'})

    # Browser did not specify: parse params as the fallback shiftjis
    app = TestApp(AssertApp(assertfunc=valid_name(u'日本', post=True,
                                                  encoding='shiftjis')))
    res = app.post('/', params=dict(name=u'日本'.encode('shiftjis')))

def test_wsgirequest_charset_fileupload():
    def handle_fileupload(environ, start_response):
        start_response('200 OK', [('Content-type','text/plain')])
        request = WSGIRequest(environ)

        assert len(request.POST) == 1
        assert isinstance(request.POST.keys()[0], str)
        fs = request.POST['thefile']
        assert isinstance(fs, cgi.FieldStorage)
        assert isinstance(fs.filename, str)
        assert fs.filename == '寿司.txt'
        assert fs.value == b'Sushi'

        request.charset = 'UTF-8'
        assert len(request.POST) == 1
        assert isinstance(request.POST.keys()[0], str)
        fs = request.POST['thefile']
        assert isinstance(fs, cgi.FieldStorage)
        assert isinstance(fs.filename, six.text_type)
        assert fs.filename == u'寿司.txt'
        assert fs.value == b'Sushi'

        request.charset = None
        assert fs.value == b'Sushi'
        return []

    app = TestApp(handle_fileupload)
    res = app.post('/', upload_files=[('thefile', '寿司.txt', b'Sushi')])

def test_wsgiresponse_charset():
    response = WSGIResponse(mimetype='text/html; charset=UTF-8')
    assert response.content_type == 'text/html'
    assert response.charset == 'UTF-8'
    response.write(u'test')
    response.write(u'test2')
    response.write('test3')
    status, headers, content = response.wsgi_response()
    for data in content:
        assert isinstance(data, six.binary_type)

    WSGIResponse.defaults._push_object(dict(content_type='text/html',
                                            charset='iso-8859-1'))
    try:
        response = WSGIResponse()
        response.write(u'test')
        response.write(u'test2')
        response.write('test3')
        status, headers, content = response.wsgi_response()
        for data in content:
            assert isinstance(data, six.binary_type)
    finally:
        WSGIResponse.defaults._pop_object()

    # WSGIResponse will allow unicode to pass through when no charset is
    # set
    WSGIResponse.defaults._push_object(dict(content_type='text/html',
                                            charset=None))
    try:
        response = WSGIResponse(u'test')
        response.write(u'test1')
        status, headers, content = response.wsgi_response()
        for data in content:
            assert isinstance(data, six.text_type)
    finally:
        WSGIResponse.defaults._pop_object()

    WSGIResponse.defaults._push_object(dict(content_type='text/html',
                                            charset=''))
    try:
        response = WSGIResponse(u'test')
        response.write(u'test1')
        status, headers, content = response.wsgi_response()
        for data in content:
            assert isinstance(data, six.text_type)
    finally:
        WSGIResponse.defaults._pop_object()
