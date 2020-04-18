# copyright 2003-2014 LOGILAB S.A. (Paris, FRANCE), all rights reserved.
# contact http://www.logilab.fr/ -- mailto:contact@logilab.fr
#
# This file is part of astroid.
#
# astroid is free software: you can redistribute it and/or modify it under
# the terms of the GNU Lesser General Public License as published by the Free
# Software Foundation, either version 2.1 of the License, or (at your option) any
# later version.
#
# astroid is distributed in the hope that it will be useful, but WITHOUT
# ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
# FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for more
# details.
#
# You should have received a copy of the GNU Lesser General Public License along
# with astroid.  If not, see <http://www.gnu.org/licenses/>.

"""Astroid hooks for six.moves."""

import sys
from textwrap import dedent

from astroid import MANAGER, register_module_extender
from astroid.builder import AstroidBuilder


def six_moves_transform_py2():
    return AstroidBuilder(MANAGER).string_build(dedent('''
    import urllib as _urllib
    import urllib2 as _urllib2
    import urlparse as _urlparse

    class Moves(object):
        import BaseHTTPServer
        import CGIHTTPServer
        import SimpleHTTPServer

        from StringIO import StringIO
        from cStringIO import StringIO as cStringIO
        from UserDict import UserDict
        from UserList import UserList
        from UserString import UserString

        import __builtin__ as builtins
        import thread as _thread
        import dummy_thread as _dummy_thread
        import ConfigParser as configparser
        import copy_reg as copyreg
        from itertools import (imap as map,
                               ifilter as filter,
                               ifilterfalse as filterfalse,
                               izip_longest as zip_longest,
                               izip as zip)
        import htmlentitydefs as html_entities
        import HTMLParser as html_parser
        import httplib as http_client
        import cookielib as http_cookiejar
        import Cookie as http_cookies
        import Queue as queue
        import repr as reprlib
        from pipes import quote as shlex_quote
        import SocketServer as socketserver
        import SimpleXMLRPCServer as xmlrpc_server
        import xmlrpclib as xmlrpc_client
        import _winreg as winreg
        import robotparser as urllib_robotparser

        input = raw_input
        intern = intern
        range = xrange
        xrange = xrange
        reduce = reduce
        reload_module = reload

        class UrllibParse(object):
            ParseResult = _urlparse.ParseResult
            SplitResult = _urlparse.SplitResult
            parse_qs = _urlparse.parse_qs
            parse_qsl = _urlparse.parse_qsl
            urldefrag = _urlparse.urldefrag
            urljoin = _urlparse.urljoin
            urlparse = _urlparse.urlparse
            urlsplit = _urlparse.urlsplit
            urlunparse = _urlparse.urlunparse
            urlunsplit = _urlparse.urlunsplit
            quote = _urllib.quote
            quote_plus = _urllib.quote_plus
            unquote = _urllib.unquote
            unquote_plus = _urllib.unquote_plus
            urlencode = _urllib.urlencode
            splitquery = _urllib.splitquery
            splittag = _urllib.splittag
            splituser = _urllib.splituser
            uses_fragment = _urlparse.uses_fragment       
            uses_netloc = _urlparse.uses_netloc
            uses_params = _urlparse.uses_params
            uses_query = _urlparse.uses_query
            uses_relative = _urlparse.uses_relative

        class UrllibError(object):
            URLError = _urllib2.URLError
            HTTPError = _urllib2.HTTPError
            ContentTooShortError = _urllib.ContentTooShortError

        class DummyModule(object):
            pass

        class UrllibRequest(object):
            urlopen = _urllib2.urlopen
            install_opener = _urllib2.install_opener
            build_opener = _urllib2.build_opener
            pathname2url = _urllib.pathname2url
            url2pathname = _urllib.url2pathname
            getproxies = _urllib.getproxies
            Request = _urllib2.Request
            OpenerDirector = _urllib2.OpenerDirector
            HTTPDefaultErrorHandler = _urllib2.HTTPDefaultErrorHandler
            HTTPRedirectHandler = _urllib2.HTTPRedirectHandler
            HTTPCookieProcessor = _urllib2.HTTPCookieProcessor
            ProxyHandler = _urllib2.ProxyHandler
            BaseHandler = _urllib2.BaseHandler
            HTTPPasswordMgr = _urllib2.HTTPPasswordMgr
            HTTPPasswordMgrWithDefaultRealm = _urllib2.HTTPPasswordMgrWithDefaultRealm
            AbstractBasicAuthHandler = _urllib2.AbstractBasicAuthHandler
            HTTPBasicAuthHandler = _urllib2.HTTPBasicAuthHandler
            ProxyBasicAuthHandler = _urllib2.ProxyBasicAuthHandler
            AbstractDigestAuthHandler = _urllib2.AbstractDigestAuthHandler
            HTTPDigestAuthHandler = _urllib2.HTTPDigestAuthHandler
            ProxyDigestAuthHandler = _urllib2.ProxyDigestAuthHandler
            HTTPHandler = _urllib2.HTTPHandler
            HTTPSHandler = _urllib2.HTTPSHandler
            FileHandler = _urllib2.FileHandler
            FTPHandler = _urllib2.FTPHandler
            CacheFTPHandler = _urllib2.CacheFTPHandler
            UnknownHandler = _urllib2.UnknownHandler
            HTTPErrorProcessor = _urllib2.HTTPErrorProcessor
            urlretrieve = _urllib.urlretrieve
            urlcleanup = _urllib.urlcleanup
            proxy_bypass = _urllib.proxy_bypass

        urllib_parse = UrllibParse()
        urllib_error = UrllibError()
        urllib = DummyModule()
        urllib.request = UrllibRequest()
        urllib.parse = UrllibParse()
        urllib.error = UrllibError()

    moves = Moves()

    '''))


def six_moves_transform_py3():
    return AstroidBuilder(MANAGER).string_build(dedent('''
    class Moves(object):
        import _io
        cStringIO = _io.StringIO
        filter = filter
        from itertools import filterfalse
        input = input
        from sys import intern
        map = map
        range = range
        from imp import reload as reload_module
        from functools import reduce
        from shlex import quote as shlex_quote
        from io import StringIO
        from collections import UserDict, UserList, UserString
        xrange = range
        zip = zip
        from itertools import zip_longest
        import builtins
        import configparser
        import copyreg
        import _dummy_thread
        import http.cookiejar as http_cookiejar
        import http.cookies as http_cookies
        import html.entities as html_entities
        import html.parser as html_parser
        import http.client as http_client
        import http.server
        BaseHTTPServer = CGIHTTPServer = SimpleHTTPServer = http.server
        import pickle as cPickle
        import queue
        import reprlib
        import socketserver
        import _thread
        import winreg
        import xmlrpc.server as xmlrpc_server
        import xmlrpc.client as xmlrpc_client
        import urllib.robotparser as urllib_robotparser
        import email.mime.multipart as email_mime_multipart
        import email.mime.nonmultipart as email_mime_nonmultipart
        import email.mime.text as email_mime_text
        import email.mime.base as email_mime_base
        import urllib.parse as urllib_parse
        import urllib.error as urllib_error
        import tkinter
        import tkinter.dialog as tkinter_dialog
        import tkinter.filedialog as tkinter_filedialog
        import tkinter.scrolledtext as tkinter_scrolledtext
        import tkinter.simpledialog as tkinder_simpledialog
        import tkinter.tix as tkinter_tix
        import tkinter.ttk as tkinter_ttk
        import tkinter.constants as tkinter_constants
        import tkinter.dnd as tkinter_dnd
        import tkinter.colorchooser as tkinter_colorchooser
        import tkinter.commondialog as tkinter_commondialog
        import tkinter.filedialog as tkinter_tkfiledialog
        import tkinter.font as tkinter_font
        import tkinter.messagebox as tkinter_messagebox
        import urllib.request
        import urllib.robotparser as urllib_robotparser
        import urllib.parse as urllib_parse
        import urllib.error as urllib_error
    moves = Moves()
    '''))

if sys.version_info[0] == 2:
    TRANSFORM = six_moves_transform_py2
else:
    TRANSFORM = six_moves_transform_py3

register_module_extender(MANAGER, 'six', TRANSFORM)
