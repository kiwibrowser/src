from paste.httpheaders import *
import time

def _test_generic(collection):
    assert 'bing' == VIA(collection)
    REFERER.update(collection,'internal:/some/path')
    assert 'internal:/some/path' == REFERER(collection)
    CACHE_CONTROL.update(collection,max_age=1234)
    CONTENT_DISPOSITION.update(collection,filename="bingles.txt")
    PRAGMA.update(collection,"test","multi",'valued="items"')
    assert 'public, max-age=1234' == CACHE_CONTROL(collection)
    assert 'attachment; filename="bingles.txt"' == \
            CONTENT_DISPOSITION(collection)
    assert 'test, multi, valued="items"' == PRAGMA(collection)
    VIA.delete(collection)


def test_environ():
    collection = {'HTTP_VIA':'bing', 'wsgi.version': '1.0' }
    _test_generic(collection)
    assert collection == {'wsgi.version': '1.0',
      'HTTP_PRAGMA': 'test, multi, valued="items"',
      'HTTP_REFERER': 'internal:/some/path',
      'HTTP_CONTENT_DISPOSITION': 'attachment; filename="bingles.txt"',
      'HTTP_CACHE_CONTROL': 'public, max-age=1234'
    }

def test_environ_cgi():
    environ = {'CONTENT_TYPE': 'text/plain', 'wsgi.version': '1.0',
               'HTTP_CONTENT_TYPE': 'ignored/invalid',
               'CONTENT_LENGTH': '200'}
    assert 'text/plain' == CONTENT_TYPE(environ)
    assert '200' == CONTENT_LENGTH(environ)
    CONTENT_TYPE.update(environ,'new/type')
    assert 'new/type' == CONTENT_TYPE(environ)
    CONTENT_TYPE.delete(environ)
    assert '' == CONTENT_TYPE(environ)
    assert 'ignored/invalid' == environ['HTTP_CONTENT_TYPE']

def test_response_headers():
    collection = [('via', 'bing')]
    _test_generic(collection)
    normalize_headers(collection)
    assert collection == [
        ('Cache-Control', 'public, max-age=1234'),
        ('Pragma', 'test, multi, valued="items"'),
        ('Referer', 'internal:/some/path'),
        ('Content-Disposition', 'attachment; filename="bingles.txt"')
    ]

def test_cache_control():
    assert 'public' == CACHE_CONTROL()
    assert 'public' == CACHE_CONTROL(public=True)
    assert 'private' == CACHE_CONTROL(private=True)
    assert 'no-cache' == CACHE_CONTROL(no_cache=True)
    assert 'private, no-store' == CACHE_CONTROL(private=True, no_store=True)
    assert 'public, max-age=60' == CACHE_CONTROL(max_age=60)
    assert 'public, max-age=86400' == \
            CACHE_CONTROL(max_age=CACHE_CONTROL.ONE_DAY)
    CACHE_CONTROL.extensions['community'] = str
    assert 'public, community="bingles"' == \
            CACHE_CONTROL(community="bingles")
    headers = []
    CACHE_CONTROL.apply(headers,max_age=60)
    assert 'public, max-age=60' == CACHE_CONTROL(headers)
    assert EXPIRES.parse(headers) > time.time()
    assert EXPIRES.parse(headers) < time.time() + 60

def test_content_disposition():
    assert 'attachment' == CONTENT_DISPOSITION()
    assert 'attachment' == CONTENT_DISPOSITION(attachment=True)
    assert 'inline' == CONTENT_DISPOSITION(inline=True)
    assert 'inline; filename="test.txt"' == \
            CONTENT_DISPOSITION(inline=True, filename="test.txt")
    assert 'attachment; filename="test.txt"' == \
            CONTENT_DISPOSITION(filename="/some/path/test.txt")
    headers = []
    CONTENT_DISPOSITION.apply(headers,filename="test.txt")
    assert 'text/plain' == CONTENT_TYPE(headers)
    CONTENT_DISPOSITION.apply(headers,filename="test")
    assert 'text/plain' == CONTENT_TYPE(headers)
    CONTENT_DISPOSITION.apply(headers,filename="test.html")
    assert 'text/plain' == CONTENT_TYPE(headers)
    headers = [('Content-Type', 'application/octet-stream')]
    CONTENT_DISPOSITION.apply(headers,filename="test.txt")
    assert 'text/plain' == CONTENT_TYPE(headers)
    assert headers == [
      ('Content-Type', 'text/plain'),
      ('Content-Disposition', 'attachment; filename="test.txt"')
    ]

def test_range():
    assert ('bytes',[(0,300)]) == RANGE.parse("bytes=0-300")
    assert ('bytes',[(0,300)]) == RANGE.parse("bytes   =  -300")
    assert ('bytes',[(0,None)]) == RANGE.parse("bytes=  -")
    assert ('bytes',[(0,None)]) == RANGE.parse("bytes=0   -   ")
    assert ('bytes',[(300,None)]) == RANGE.parse("   BYTES=300-")
    assert ('bytes',[(4,5),(6,7)]) == RANGE.parse(" Bytes = 4 - 5,6 - 07  ")
    assert ('bytes',[(0,5),(7,None)]) == RANGE.parse(" bytes=-5,7-")
    assert ('bytes',[(0,5),(7,None)]) == RANGE.parse(" bytes=-5,7-")
    assert ('bytes',[(0,5),(7,None)]) == RANGE.parse(" bytes=-5,7-")
    assert None == RANGE.parse("")
    assert None == RANGE.parse("bytes=0,300")
    assert None == RANGE.parse("bytes=-7,5-")

def test_copy():
    environ = {'HTTP_VIA':'bing', 'wsgi.version': '1.0' }
    response_headers = []
    VIA.update(response_headers,environ)
    assert response_headers == [('Via', 'bing')]

def test_sorting():
    # verify the HTTP_HEADERS are set with their canonical form
    sample = [WWW_AUTHENTICATE, VIA, ACCEPT, DATE,
    ACCEPT_CHARSET, AGE, ALLOW, CACHE_CONTROL,
    CONTENT_ENCODING, ETAG, CONTENT_TYPE, FROM,
    EXPIRES, RANGE, UPGRADE, VARY, ALLOW]
    sample.sort()
    sample = [str(x) for x in sample]
    assert sample == [
     # general headers first
     'Cache-Control', 'Date', 'Upgrade', 'Via',
     # request headers next
     'Accept', 'Accept-Charset', 'From', 'Range',
     # response headers following
     'Age', 'ETag', 'Vary', 'WWW-Authenticate',
     # entity headers (/w expected duplicate)
     'Allow', 'Allow', 'Content-Encoding', 'Content-Type', 'Expires'
    ]

def test_normalize():
    response_headers = [
       ('www-authenticate','Response AuthMessage'),
       ('unknown-header','Unknown Sorted Last'),
       ('Via','General Bingles'),
       ('aLLoW','Entity Allow Something'),
       ('ETAG','Response 34234'),
       ('expires','Entity An-Expiration-Date'),
       ('date','General A-Date')]
    normalize_headers(response_headers, strict=False)
    assert response_headers == [
     ('Date', 'General A-Date'),
     ('Via', 'General Bingles'),
     ('ETag', 'Response 34234'),
     ('WWW-Authenticate', 'Response AuthMessage'),
     ('Allow', 'Entity Allow Something'),
     ('Expires', 'Entity An-Expiration-Date'),
     ('Unknown-Header', 'Unknown Sorted Last')]

def test_if_modified_since():
    from paste.httpexceptions import HTTPBadRequest
    date = 'Thu, 34 Jul 3119 29:34:18 GMT'
    try:
        x = IF_MODIFIED_SINCE.parse({'HTTP_IF_MODIFIED_SINCE': date,
                                     'wsgi.version': (1, 0)})
    except HTTPBadRequest:
        pass
    else:
        assert 0
