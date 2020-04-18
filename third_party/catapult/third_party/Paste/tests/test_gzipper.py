from paste.fixture import TestApp
from paste.gzipper import middleware
import gzip
import six

def simple_app(environ, start_response):
    start_response('200 OK', [('content-type', 'text/plain')])
    return [b'this is a test']

wsgi_app = middleware(simple_app)
app = TestApp(wsgi_app)

def test_gzip():
    res = app.get(
        '/', extra_environ=dict(HTTP_ACCEPT_ENCODING='gzip'))
    assert int(res.header('content-length')) == len(res.body)
    assert res.body != b'this is a test'
    actual = gzip.GzipFile(fileobj=six.BytesIO(res.body)).read()
    assert actual == b'this is a test'
