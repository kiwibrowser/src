from .test_errordocument import simple_app
from paste.fixture import *
from paste.recursive import RecursiveMiddleware, ForwardRequestException

def error_docs_app(environ, start_response):
    if environ['PATH_INFO'] == '/not_found':
        start_response("404 Not found", [('Content-type', 'text/plain')])
        return [b'Not found']
    elif environ['PATH_INFO'] == '/error':
        start_response("200 OK", [('Content-type', 'text/plain')])
        return [b'Page not found']
    elif environ['PATH_INFO'] == '/recurse':
        raise ForwardRequestException('/recurse')
    else:
        return simple_app(environ, start_response)

class Middleware(object):
    def __init__(self, app, url='/error'):
        self.app = app
        self.url = url
    def __call__(self, environ, start_response):
        raise ForwardRequestException(self.url)

def forward(app):
    app = TestApp(RecursiveMiddleware(app))
    res = app.get('')
    assert res.header('content-type') == 'text/plain'
    assert res.full_status == '200 OK'
    assert 'requested page returned' in res
    res = app.get('/error')
    assert res.header('content-type') == 'text/plain'
    assert res.full_status == '200 OK'
    assert 'Page not found' in res
    res = app.get('/not_found')
    assert res.header('content-type') == 'text/plain'
    assert res.full_status == '200 OK'
    assert 'Page not found' in res
    try:
        res = app.get('/recurse')
    except AssertionError as e:
        if str(e).startswith('Forwarding loop detected'):
            pass
        else:
            raise AssertionError('Failed to detect forwarding loop')

def test_ForwardRequest_url():
    class TestForwardRequestMiddleware(Middleware):
        def __call__(self, environ, start_response):
            if environ['PATH_INFO'] != '/not_found':
                return self.app(environ, start_response)
            raise ForwardRequestException(self.url)
    forward(TestForwardRequestMiddleware(error_docs_app))

def test_ForwardRequest_environ():
    class TestForwardRequestMiddleware(Middleware):
        def __call__(self, environ, start_response):
            if environ['PATH_INFO'] != '/not_found':
                return self.app(environ, start_response)
            environ['PATH_INFO'] = self.url
            raise ForwardRequestException(environ=environ)
    forward(TestForwardRequestMiddleware(error_docs_app))

def test_ForwardRequest_factory():

    from paste.errordocument import StatusKeeper

    class TestForwardRequestMiddleware(Middleware):
        def __call__(self, environ, start_response):
            if environ['PATH_INFO'] != '/not_found':
                return self.app(environ, start_response)
            environ['PATH_INFO'] = self.url
            def factory(app):
                return StatusKeeper(app, status='404 Not Found', url='/error', headers=[])
            raise ForwardRequestException(factory=factory)

    app = TestForwardRequestMiddleware(error_docs_app)
    app = TestApp(RecursiveMiddleware(app))
    res = app.get('')
    assert res.header('content-type') == 'text/plain'
    assert res.full_status == '200 OK'
    assert 'requested page returned' in res
    res = app.get('/error')
    assert res.header('content-type') == 'text/plain'
    assert res.full_status == '200 OK'
    assert 'Page not found' in res
    res = app.get('/not_found', status=404)
    assert res.header('content-type') == 'text/plain'
    assert res.full_status == '404 Not Found' # Different status
    assert 'Page not found' in res
    try:
        res = app.get('/recurse')
    except AssertionError as e:
        if str(e).startswith('Forwarding loop detected'):
            pass
        else:
            raise AssertionError('Failed to detect forwarding loop')

# Test Deprecated Code
def test_ForwardRequestException():
    class TestForwardRequestExceptionMiddleware(Middleware):
        def __call__(self, environ, start_response):
            if environ['PATH_INFO'] != '/not_found':
                return self.app(environ, start_response)
            raise ForwardRequestException(path_info=self.url)
    forward(TestForwardRequestExceptionMiddleware(error_docs_app))
