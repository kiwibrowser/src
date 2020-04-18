from paste.debug.debugapp import SimpleApplication
from paste.fixture import TestApp

def test_fixture():
    app = TestApp(SimpleApplication())
    res = app.get('/', params={'a': ['1', '2']})
    assert (res.request.environ['QUERY_STRING'] ==
            'a=1&a=2')
    res = app.put('/')
    assert (res.request.environ['REQUEST_METHOD'] ==
            'PUT')
    res = app.delete('/')
    assert (res.request.environ['REQUEST_METHOD'] ==
            'DELETE')
    class FakeDict(object):
        def items(self):
            return [('a', '10'), ('a', '20')]
    res = app.post('/params', params=FakeDict())

    # test multiple cookies in one request
    app.cookies['one'] = 'first';
    app.cookies['two'] = 'second';
    app.cookies['three'] = '';
    res = app.get('/')
    hc = res.request.environ['HTTP_COOKIE'].split('; ');
    assert ('one=first' in hc)
    assert ('two=second' in hc)
    assert ('three=' in hc)
