import os
from paste.urlparser import *
from paste.fixture import *
from pkg_resources import get_distribution

def relative_path(name):
    here = os.path.join(os.path.dirname(os.path.abspath(__file__)),
                        'urlparser_data')
    f = os.path.join('urlparser_data', '..', 'urlparser_data', name)
    return os.path.join(here, f)

def path(name):
    return os.path.join(os.path.dirname(os.path.abspath(__file__)),
                        'urlparser_data', name)

def make_app(name):
    app = URLParser({}, path(name), name, index_names=['index', 'Main'])
    testapp = TestApp(app)
    return testapp

def test_find_file():
    app = make_app('find_file')
    res = app.get('/')
    assert 'index1' in res
    assert res.header('content-type') == 'text/plain'
    res = app.get('/index')
    assert 'index1' in res
    assert res.header('content-type') == 'text/plain'
    res = app.get('/index.txt')
    assert 'index1' in res
    assert res.header('content-type') == 'text/plain'
    res = app.get('/test2.html')
    assert 'test2' in res
    assert res.header('content-type') == 'text/html'
    res = app.get('/test 3.html')
    assert 'test 3' in res
    assert res.header('content-type') == 'text/html'
    res = app.get('/test%203.html')
    assert 'test 3' in res
    assert res.header('content-type') == 'text/html'
    res = app.get('/dir with spaces/test 4.html')
    assert 'test 4' in res
    assert res.header('content-type') == 'text/html'
    res = app.get('/dir%20with%20spaces/test%204.html')
    assert 'test 4' in res
    assert res.header('content-type') == 'text/html'
    # Ensure only data under the app's root directory is accessible
    res = app.get('/../secured.txt', status=404)
    res = app.get('/dir with spaces/../../secured.txt', status=404)
    res = app.get('/%2e%2e/secured.txt', status=404)
    res = app.get('/%2e%2e%3fsecured.txt', status=404)
    res = app.get('/..%3fsecured.txt', status=404)
    res = app.get('/dir%20with%20spaces/%2e%2e/%2e%2e/secured.txt', status=404)

def test_deep():
    app = make_app('deep')
    res = app.get('/')
    assert 'index2' in res
    res = app.get('/sub')
    assert res.status == 301
    print(res)
    assert res.header('location') == 'http://localhost/sub/'
    assert 'http://localhost/sub/' in res
    res = app.get('/sub/')
    assert 'index3' in res

def test_python():
    app = make_app('python')
    res = app.get('/simpleapp')
    assert 'test1' in res
    assert res.header('test-header') == 'TEST!'
    assert res.header('content-type') == 'text/html'
    res = app.get('/stream')
    assert 'test2' in res
    res = app.get('/sub/simpleapp')
    assert 'subsimple' in res

def test_hook():
    app = make_app('hook')
    res = app.get('/bob/app')
    assert 'user: bob' in res
    res = app.get('/tim/')
    assert 'index: tim' in res

def test_not_found_hook():
    app = make_app('not_found')
    res = app.get('/simple/notfound')
    assert res.status == 200
    assert 'not found' in res
    res = app.get('/simple/found')
    assert 'is found' in res
    res = app.get('/recur/__notfound', status=404)
    # @@: It's unfortunate that the original path doesn't actually show up
    assert '/recur/notfound' in res
    res = app.get('/recur/__isfound')
    assert res.status == 200
    assert 'is found' in res
    res = app.get('/user/list')
    assert 'user: None' in res
    res = app.get('/user/bob/list')
    assert res.status == 200
    assert 'user: bob' in res

def test_relative_path_in_static_parser():
    x = relative_path('find_file')
    app = StaticURLParser(relative_path('find_file'))
    assert '..' not in app.root_directory

def test_xss():
    app = TestApp(StaticURLParser(relative_path('find_file')),
                  extra_environ={'HTTP_ACCEPT': 'text/html'})
    res = app.get("/-->%0D<script>alert('xss')</script>", status=404)
    assert b'--><script>' not in res.body

def test_static_parser():
    app = StaticURLParser(path('find_file'))
    testapp = TestApp(app)
    res = testapp.get('', status=301)
    res = testapp.get('/', status=404)
    res = testapp.get('/index.txt')
    assert res.body.strip() == b'index1'
    res = testapp.get('/index.txt/foo', status=404)
    res = testapp.get('/test 3.html')
    assert res.body.strip() == b'test 3'
    res = testapp.get('/test%203.html')
    assert res.body.strip() == b'test 3'
    res = testapp.get('/dir with spaces/test 4.html')
    assert res.body.strip() == b'test 4'
    res = testapp.get('/dir%20with%20spaces/test%204.html')
    assert res.body.strip() == b'test 4'
    # Ensure only data under the app's root directory is accessible
    res = testapp.get('/../secured.txt', status=404)
    res = testapp.get('/dir with spaces/../../secured.txt', status=404)
    res = testapp.get('/%2e%2e/secured.txt', status=404)
    res = testapp.get('/dir%20with%20spaces/%2e%2e/%2e%2e/secured.txt', status=404)
    res = testapp.get('/dir%20with%20spaces/', status=404)

def test_egg_parser():
    app = PkgResourcesParser('Paste', 'paste')
    testapp = TestApp(app)
    res = testapp.get('', status=301)
    res = testapp.get('/', status=404)
    res = testapp.get('/flup_session', status=404)
    res = testapp.get('/util/classinit.py')
    assert 'ClassInitMeta' in res
    res = testapp.get('/util/classinit', status=404)
    res = testapp.get('/util', status=301)
    res = testapp.get('/util/classinit.py/foo', status=404)

    # Find a readable file in the Paste pkg's root directory (or upwards the
    # directory tree). Ensure it's not accessible via the URLParser
    unreachable_test_file = None
    search_path = pkg_root_path = get_distribution('Paste').location
    level = 0
    # We might not find any readable files in the pkg's root directory (this
    # is likely when Paste is installed as a .egg in site-packages). We
    # (hopefully) can prevent this by traversing up the directory tree until
    # a usable file is found
    while unreachable_test_file is None and \
            os.path.normpath(search_path) != os.path.sep:
        for file in os.listdir(search_path):
            full_path = os.path.join(search_path, file)
            if os.path.isfile(full_path) and os.access(full_path, os.R_OK):
                unreachable_test_file = file
                break

        search_path = os.path.dirname(search_path)
        level += 1
    assert unreachable_test_file is not None, \
           'test_egg_parser requires a readable file in a parent dir of the\n' \
           'Paste pkg\'s root dir:\n%s' % pkg_root_path

    unreachable_path = '/' + '../'*level + unreachable_test_file
    unreachable_path_quoted = '/' + '%2e%2e/'*level + unreachable_test_file
    res = testapp.get(unreachable_path, status=404)
    res = testapp.get('/util/..' + unreachable_path, status=404)
    res = testapp.get(unreachable_path_quoted, status=404)
    res = testapp.get('/util/%2e%2e' + unreachable_path_quoted, status=404)
