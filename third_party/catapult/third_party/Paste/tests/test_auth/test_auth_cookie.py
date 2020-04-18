# (c) 2005 Clark C. Evans
# This module is part of the Python Paste Project and is released under
# the MIT License: http://www.opensource.org/licenses/mit-license.php

from six.moves import xrange
import six

from paste.auth import cookie
from paste.wsgilib import raw_interactive, dump_environ
from paste.response import header_value
from paste.httpexceptions import *

def build(application,setenv, *args, **kwargs):
    def setter(environ, start_response):
        save = environ['paste.auth.cookie'].append
        for (k,v) in setenv.items():
            save(k)
            environ[k] = v
        return application(environ, start_response)
    return cookie.middleware(setter,*args,**kwargs)

def test_noop():
    app = build(dump_environ,{})
    (status,headers,content,errors) = \
        raw_interactive(app)
    assert not header_value(headers,'Set-Cookie')

def test_basic(key='key', val='bingles'):
    app = build(dump_environ,{key:val})
    (status,headers,content,errors) = \
        raw_interactive(app)
    value = header_value(headers,'Set-Cookie')
    assert "Path=/;" in value
    assert "expires=" not in value
    cookie = value.split(";")[0]
    (status,headers,content,errors) = \
            raw_interactive(app,{'HTTP_COOKIE': cookie})
    expected = ("%s: %s" % (key,val.replace("\n","\n    ")))
    if six.PY3:
        expected = expected.encode('utf8')
    assert expected in content

def test_roundtrip():
    roundtrip = str('').join(map(chr, xrange(256)))
    test_basic(roundtrip,roundtrip)

