from paste import proxy
from paste.fixture import TestApp

def test_paste_website():
    # Not the most robust test...
    # need to test things like POSTing to pages, and getting from pages
    # that don't set content-length.
    app = proxy.Proxy('http://pythonpaste.org')
    app = TestApp(app)
    res = app.get('/')
    assert 'documentation' in res

