from paste.fixture import *
from paste.cgitb_catcher import CgitbMiddleware
from paste import lint
from .test_exceptions.test_error_middleware import clear_middleware

def do_request(app, expect_status=500):
    app = lint.middleware(app)
    app = CgitbMiddleware(app, {}, display=True)
    app = clear_middleware(app)
    testapp = TestApp(app)
    res = testapp.get('', status=expect_status,
                      expect_errors=True)
    return res


############################################################
## Applications that raise exceptions
############################################################

def bad_app():
    "No argument list!"
    return None

def start_response_app(environ, start_response):
    "raise error before start_response"
    raise ValueError("hi")

def after_start_response_app(environ, start_response):
    start_response("200 OK", [('Content-type', 'text/plain')])
    raise ValueError('error2')

def iter_app(environ, start_response):
    start_response("200 OK", [('Content-type', 'text/plain')])
    return yielder([b'this', b' is ', b' a', None])

def yielder(args):
    for arg in args:
        if arg is None:
            raise ValueError("None raises error")
        yield arg

############################################################
## Tests
############################################################

def test_makes_exception():
    res = do_request(bad_app)
    print(res)
    if six.PY3:
        assert 'bad_app() takes 0 positional arguments but 2 were given' in res
    else:
        assert 'bad_app() takes no arguments (2 given' in res
    assert 'iterator = application(environ, start_response_wrapper)' in res
    assert 'lint.py' in res
    assert 'cgitb_catcher.py' in res

def test_start_res():
    res = do_request(start_response_app)
    print(res)
    assert 'ValueError: hi' in res
    assert 'test_cgitb_catcher.py' in res
    assert 'line 26, in start_response_app' in res

def test_after_start():
    res = do_request(after_start_response_app, 200)
    print(res)
    assert 'ValueError: error2' in res
    assert 'line 30' in res

def test_iter_app():
    res = do_request(iter_app, 200)
    print(res)
    assert 'None raises error' in res
    assert 'yielder' in res




