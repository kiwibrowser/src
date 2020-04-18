from paste.fixture import *
from paste.exceptions.errormiddleware import ErrorMiddleware
from paste import lint
from paste.util.quoting import strip_html
#
# For some strange reason, these 4 lines cannot be removed or the regression
# test breaks; is it counting the number of lines in the file somehow?
#

def do_request(app, expect_status=500):
    app = lint.middleware(app)
    app = ErrorMiddleware(app, {}, debug=True)
    app = clear_middleware(app)
    testapp = TestApp(app)
    res = testapp.get('', status=expect_status,
                      expect_errors=True)
    return res

def clear_middleware(app):
    """
    The fixture sets paste.throw_errors, which suppresses exactly what
    we want to test in this case. This wrapper also strips exc_info
    on the *first* call to start_response (but not the second, or
    subsequent calls.
    """
    def clear_throw_errors(environ, start_response):
        headers_sent = []
        def replacement(status, headers, exc_info=None):
            if headers_sent:
                return start_response(status, headers, exc_info)
            headers_sent.append(True)
            return start_response(status, headers)
        if 'paste.throw_errors' in environ:
            del environ['paste.throw_errors']
        return app(environ, replacement)
    return clear_throw_errors


############################################################
## Applications that raise exceptions
############################################################

def bad_app():
    "No argument list!"
    return None

def unicode_bad_app(environ, start_response):
    raise ValueError(u"\u1000")

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
    assert '<html' in res
    res = strip_html(str(res))
    if six.PY3:
        assert 'bad_app() takes 0 positional arguments but 2 were given' in res
    else:
        assert 'bad_app() takes no arguments (2 given' in res, repr(res)
    assert 'iterator = application(environ, start_response_wrapper)' in res
    assert 'paste.lint' in res
    assert 'paste.exceptions.errormiddleware' in res

def test_unicode_exception():
    res = do_request(unicode_bad_app)


def test_start_res():
    res = do_request(start_response_app)
    res = strip_html(str(res))
    assert 'ValueError: hi' in res
    assert 'test_error_middleware' in res
    assert ':52 in start_response_app' in res

def test_after_start():
    res = do_request(after_start_response_app, 200)
    res = strip_html(str(res))
    #print res
    assert 'ValueError: error2' in res

def test_iter_app():
    res = do_request(lint.middleware(iter_app), 200)
    #print res
    assert 'None raises error' in res
    assert 'yielder' in res




