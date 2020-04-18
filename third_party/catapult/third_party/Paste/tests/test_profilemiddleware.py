from paste.fixture import *
try:
    from paste.debug.profile import *
    disable = False
except ImportError:
    disable = True

if not disable:
    def simple_app(environ, start_response):
        start_response('200 OK', [('content-type', 'text/html')])
        return ['all ok']

    def long_func():
        for i in range(1000):
            pass
        return 'test'

    def test_profile():
        app = TestApp(ProfileMiddleware(simple_app, {}))
        res = app.get('/')
        # The original app:
        res.mustcontain('all ok')
        # The profile information:
        res.mustcontain('<pre')

    def test_decorator():
        value = profile_decorator()(long_func)()
        assert value == 'test'

