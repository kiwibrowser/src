# (c) 2005 Ian Bicking, Clark C. Evans and contributors
# This module is part of the Python Paste Project and is released under
# the MIT License: http://www.opensource.org/licenses/mit-license.php
import time
import random
import os
import tempfile
try:
    # Python 3
    from email.utils import parsedate_tz, mktime_tz
except ImportError:
    # Python 2
    from rfc822 import parsedate_tz, mktime_tz
import six

from paste import fileapp
from paste.fileapp import *
from paste.fixture import *

# NOTE(haypo): don't use string.letters because the order of lower and upper
# case letters changes when locale.setlocale() is called for the first time
LETTERS = 'abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ'

def test_data():
    harness = TestApp(DataApp(b'mycontent'))
    res = harness.get("/")
    assert 'application/octet-stream' == res.header('content-type')
    assert '9' == res.header('content-length')
    assert "<Response 200 OK 'mycontent'>" == repr(res)
    harness.app.set_content(b"bingles")
    assert "<Response 200 OK 'bingles'>" == repr(harness.get("/"))

def test_cache():
    def build(*args,**kwargs):
        app = DataApp(b"SomeContent")
        app.cache_control(*args,**kwargs)
        return TestApp(app).get("/")
    res = build()
    assert 'public' == res.header('cache-control')
    assert not res.header('expires',None)
    res = build(private=True)
    assert 'private' == res.header('cache-control')
    assert mktime_tz(parsedate_tz(res.header('expires'))) < time.time()
    res = build(no_cache=True)
    assert 'no-cache' == res.header('cache-control')
    assert mktime_tz(parsedate_tz(res.header('expires'))) < time.time()
    res = build(max_age=60,s_maxage=30)
    assert 'public, max-age=60, s-maxage=30' == res.header('cache-control')
    expires = mktime_tz(parsedate_tz(res.header('expires')))
    assert expires > time.time()+58 and expires < time.time()+61
    res = build(private=True, max_age=60, no_transform=True, no_store=True)
    assert 'private, no-store, no-transform, max-age=60' == \
           res.header('cache-control')
    expires = mktime_tz(parsedate_tz(res.header('expires')))
    assert mktime_tz(parsedate_tz(res.header('expires'))) < time.time()

def test_disposition():
    def build(*args,**kwargs):
        app = DataApp(b"SomeContent")
        app.content_disposition(*args,**kwargs)
        return TestApp(app).get("/")
    res = build()
    assert 'attachment' == res.header('content-disposition')
    assert 'application/octet-stream' == res.header('content-type')
    res = build(filename="bing.txt")
    assert 'attachment; filename="bing.txt"' == \
            res.header('content-disposition')
    assert 'text/plain' == res.header('content-type')
    res = build(inline=True)
    assert 'inline' == res.header('content-disposition')
    assert 'application/octet-stream' == res.header('content-type')
    res = build(inline=True, filename="/some/path/bing.txt")
    assert 'inline; filename="bing.txt"' == \
            res.header('content-disposition')
    assert 'text/plain' == res.header('content-type')
    try:
       res = build(inline=True,attachment=True)
    except AssertionError:
        pass
    else:
        assert False, "should be an exception"

def test_modified():
    harness = TestApp(DataApp(b'mycontent'))
    res = harness.get("/")
    assert "<Response 200 OK 'mycontent'>" == repr(res)
    last_modified = res.header('last-modified')
    res = harness.get("/",headers={'if-modified-since': last_modified})
    assert "<Response 304 Not Modified ''>" == repr(res)
    res = harness.get("/",headers={'if-modified-since': last_modified + \
                                   '; length=1506'})
    assert "<Response 304 Not Modified ''>" == repr(res)
    res = harness.get("/",status=400,
            headers={'if-modified-since': 'garbage'})
    assert 400 == res.status and b"ill-formed timestamp" in res.body
    res = harness.get("/",status=400,
            headers={'if-modified-since':
                'Thu, 22 Dec 2030 01:01:01 GMT'})
    assert 400 == res.status and b"check your system clock" in res.body

def test_file():
    tempfile = "test_fileapp.%s.txt" % (random.random())
    content = LETTERS * 20
    if six.PY3:
        content = content.encode('utf8')
    with open(tempfile, "wb") as fp:
        fp.write(content)
    try:
        app = fileapp.FileApp(tempfile)
        res = TestApp(app).get("/")
        assert len(content) == int(res.header('content-length'))
        assert 'text/plain' == res.header('content-type')
        assert content == res.body
        assert content == app.content  # this is cashed
        lastmod = res.header('last-modified')
        print("updating", tempfile)
        file = open(tempfile,"a+")
        file.write("0123456789")
        file.close()
        res = TestApp(app).get("/",headers={'Cache-Control': 'max-age=0'})
        assert len(content)+10 == int(res.header('content-length'))
        assert 'text/plain' == res.header('content-type')
        assert content + b"0123456789" == res.body
        assert app.content # we are still cached
        file = open(tempfile,"a+")
        file.write("X" * fileapp.CACHE_SIZE) # exceed the cashe size
        file.write("YZ")
        file.close()
        res = TestApp(app).get("/",headers={'Cache-Control': 'max-age=0'})
        newsize = fileapp.CACHE_SIZE + len(content)+12
        assert newsize == int(res.header('content-length'))
        assert newsize == len(res.body)
        assert res.body.startswith(content) and res.body.endswith(b'XYZ')
        assert not app.content # we are no longer cached
    finally:
        os.unlink(tempfile)

def test_dir():
    tmpdir = tempfile.mkdtemp()
    try:
        tmpfile = os.path.join(tmpdir, 'file')
        tmpsubdir = os.path.join(tmpdir, 'dir')
        fp = open(tmpfile, 'w')
        fp.write('abcd')
        fp.close()
        os.mkdir(tmpsubdir)
        try:
            app = fileapp.DirectoryApp(tmpdir)
            for path in ['/', '', '//', '/..', '/.', '/../..']:
                assert TestApp(app).get(path, status=403).status == 403, ValueError(path)
            for path in ['/~', '/foo', '/dir', '/dir/']:
                assert TestApp(app).get(path, status=404).status == 404, ValueError(path)
            assert TestApp(app).get('/file').body == b'abcd'
        finally:
            os.remove(tmpfile)
            os.rmdir(tmpsubdir)
    finally:
        os.rmdir(tmpdir)

def _excercize_range(build,content):
    # full content request, but using ranges'
    res = build("bytes=0-%d" % (len(content)-1))
    assert res.header('accept-ranges') == 'bytes'
    assert res.body == content
    assert res.header('content-length') == str(len(content))
    res = build("bytes=-%d" % (len(content)-1))
    assert res.body == content
    assert res.header('content-length') == str(len(content))
    res = build("bytes=0-")
    assert res.body == content
    assert res.header('content-length') == str(len(content))
    # partial content requests
    res = build("bytes=0-9", status=206)
    assert res.body == content[:10]
    assert res.header('content-length') == '10'
    res = build("bytes=%d-" % (len(content)-1), status=206)
    assert res.body == b'Z'
    assert res.header('content-length') == '1'
    res = build("bytes=%d-%d" % (3,17), status=206)
    assert res.body == content[3:18]
    assert res.header('content-length') == '15'

def test_range():
    content = LETTERS * 5
    if six.PY3:
        content = content.encode('utf8')
    def build(range, status=206):
        app = DataApp(content)
        return TestApp(app).get("/",headers={'Range': range}, status=status)
    _excercize_range(build,content)
    build('bytes=0-%d' % (len(content)+1), 416)

def test_file_range():
    tempfile = "test_fileapp.%s.txt" % (random.random())
    content = LETTERS * (1+(fileapp.CACHE_SIZE // len(LETTERS)))
    if six.PY3:
        content = content.encode('utf8')
    assert len(content) > fileapp.CACHE_SIZE
    with open(tempfile, "wb") as fp:
        fp.write(content)
    try:
        def build(range, status=206):
            app = fileapp.FileApp(tempfile)
            return TestApp(app).get("/",headers={'Range': range},
                                        status=status)
        _excercize_range(build,content)
        for size in (13,len(LETTERS), len(LETTERS)-1):
            fileapp.BLOCK_SIZE = size
            _excercize_range(build,content)
    finally:
        os.unlink(tempfile)

def test_file_cache():
    filename = os.path.join(os.path.dirname(__file__),
                            'urlparser_data', 'secured.txt')
    app = TestApp(fileapp.FileApp(filename))
    res = app.get('/')
    etag = res.header('ETag')
    last_mod = res.header('Last-Modified')
    res = app.get('/', headers={'If-Modified-Since': last_mod},
                  status=304)
    res = app.get('/', headers={'If-None-Match': etag},
                  status=304)
    res = app.get('/', headers={'If-None-Match': 'asdf'},
                  status=200)
    res = app.get('/', headers={'If-Modified-Since': 'Sat, 1 Jan 2005 12:00:00 GMT'},
                  status=200)
    res = app.get('/', headers={'If-Modified-Since': last_mod + '; length=100'},
                  status=304)
    res = app.get('/', headers={'If-Modified-Since': 'invalid date'},
                  status=400)

def test_methods():
    filename = os.path.join(os.path.dirname(__file__),
                            'urlparser_data', 'secured.txt')
    app = TestApp(fileapp.FileApp(filename))
    get_res = app.get('')
    res = app.get('', extra_environ={'REQUEST_METHOD': 'HEAD'})
    assert res.headers == get_res.headers
    assert not res.body
    app.post('', status=405) # Method Not Allowed

