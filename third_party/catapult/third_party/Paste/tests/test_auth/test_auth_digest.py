# (c) 2005 Clark C. Evans
# This module is part of the Python Paste Project and is released under
# the MIT License: http://www.opensource.org/licenses/mit-license.php

from paste.auth.digest import *
from paste.wsgilib import raw_interactive
from paste.httpexceptions import *
from paste.httpheaders import AUTHORIZATION, WWW_AUTHENTICATE, REMOTE_USER
import os
import six

def application(environ, start_response):
    content = REMOTE_USER(environ)
    start_response("200 OK",(('Content-Type', 'text/plain'),
                             ('Content-Length', len(content))))

    if six.PY3:
        content = content.encode('utf8')
    return [content]

realm = "tag:clarkevans.com,2005:testing"

def backwords(environ, realm, username):
    """ dummy password hash, where user password is just reverse """
    password = list(username)
    password.reverse()
    password = "".join(password)
    return digest_password(realm, username, password)

application = AuthDigestHandler(application,realm,backwords)
application = HTTPExceptionHandler(application)

def check(username, password, path="/"):
    """ perform two-stage authentication to verify login """
    (status,headers,content,errors) = \
        raw_interactive(application,path, accept='text/html')
    assert status.startswith("401")
    challenge = WWW_AUTHENTICATE(headers)
    response = AUTHORIZATION(username=username, password=password,
                             challenge=challenge, path=path)
    assert "Digest" in response and username in response
    (status,headers,content,errors) = \
        raw_interactive(application,path,
                        HTTP_AUTHORIZATION=response)
    if status.startswith("200"):
        return content
    if status.startswith("401"):
        return None
    assert False, "Unexpected Status: %s" % status

def test_digest():
    assert b'bing' == check("bing","gnib")
    assert check("bing","bad") is None

#
# The following code uses sockets to test the functionality,
# to enable use:
#
# $ TEST_SOCKET py.test
#

if os.environ.get("TEST_SOCKET",""):
    from six.moves.urllib.error import HTTPError
    from six.moves.urllib.request import build_opener, HTTPDigestAuthHandler
    from paste.debug.testserver import serve
    server = serve(application)

    def authfetch(username,password,path="/",realm=realm):
        server.accept(2)
        import socket
        socket.setdefaulttimeout(5)
        uri = ("http://%s:%s" % server.server_address) + path
        auth = HTTPDigestAuthHandler()
        auth.add_password(realm,uri,username,password)
        opener = build_opener(auth)
        result = opener.open(uri)
        return result.read()

    def test_success():
        assert "bing" == authfetch('bing','gnib')

    def test_failure():
        # urllib tries 5 more times before it gives up
        server.accept(5)
        try:
            authfetch('bing','wrong')
            assert False, "this should raise an exception"
        except HTTPError as e:
            assert e.code == 401

    def test_shutdown():
        server.stop()

