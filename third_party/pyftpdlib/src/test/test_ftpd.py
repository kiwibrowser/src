#!/usr/bin/env python
# -*- coding: utf-8 -*-

# $Id$

#  ======================================================================
#  Copyright (C) 2007-2012 Giampaolo Rodola' <g.rodola@gmail.com>
#
#                         All Rights Reserved
#
# Permission is hereby granted, free of charge, to any person
# obtaining a copy of this software and associated documentation
# files (the "Software"), to deal in the Software without
# restriction, including without limitation the rights to use,
# copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the
# Software is furnished to do so, subject to the following
# conditions:
#
# The above copyright notice and this permission notice shall be
# included in all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
# EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
# OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
# NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
# HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
# WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
# FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
# OTHER DEALINGS IN THE SOFTWARE.
#
#  ======================================================================


# This test suite has been run successfully on the following systems:
#
# -----------------------------------------------------------
#  System                          | Python version
# -----------------------------------------------------------
#  Linux Ubuntu 2.6.20-15          | 2.4, 2.5, 2.6, 2.7
#  Linux Kubuntu 8.04 32 & 64 bits | 2.5.2
#  Windows XP prof SP3             | 2.4, 2.5, 2.6.1
#  Windows Vista Ultimate 64 bit   | 2.6, 2.7
#  Windows Server 2008 64bit       | 2.5.1
#  Windows Mobile 6.1              | PythonCE 2.5
#  OS X 10.4.10                    | 2.4, 2.5, 2.7
#  FreeBSD 7.0                     | 2.4, 2.5, 2.7
# -----------------------------------------------------------


import threading
import unittest
import socket
import os
import shutil
import time
import re
import tempfile
import ftplib
import random
import warnings
import sys
import errno
import asyncore
import atexit
import stat
try:
    import cStringIO as StringIO
except ImportError:
    import StringIO
try:
    import ssl
except ImportError:
    ssl = None
try:
    import sendfile
except ImportError:
    sendfile = None

from pyftpdlib import ftpserver


# Attempt to use IP rather than hostname (test suite will run a lot faster)
try:
    HOST = socket.gethostbyname('localhost')
except socket.error:
    HOST = 'localhost'
USER = 'user'
PASSWD = '12345'
HOME = os.getcwd()
TESTFN = 'tmp-pyftpdlib'

def try_address(host, port=0, family=socket.AF_INET):
    """Try to bind a socket on the given host:port and return True
    if that has been possible."""
    try:
        sock = socket.socket(family)
        sock.bind((host, port))
    except (socket.error, socket.gaierror):
        return False
    else:
        sock.close()
        return True

def support_hybrid_ipv6():
    """Return True if it is possible to use hybrid IPv6/IPv4 sockets
    on this platform.
    """
    # IPPROTO_IPV6 constant is broken, see: http://bugs.python.org/issue6926
    IPPROTO_IPV6 = getattr(socket, "IPV6_V6ONLY", 41)
    IPV6_V6ONLY = getattr(socket, "IPV6_V6ONLY", 26)
    sock = socket.socket(socket.AF_INET6)
    try:
        try:
            return not sock.getsockopt(IPPROTO_IPV6, IPV6_V6ONLY)
        except socket.error:
            return False
    finally:
        sock.close()


SUPPORTS_IPV4 = try_address('127.0.0.1')
SUPPORTS_IPV6 = socket.has_ipv6 and try_address('::1', family=socket.AF_INET6)
SUPPORTS_HYBRID_IPV6 = SUPPORTS_IPV6 and support_hybrid_ipv6()
SUPPORTS_SENDFILE = sendfile is not None

def safe_remove(*files):
    "Convenience function for removing temporary test files"
    for file in files:
        try:
            os.remove(file)
        except OSError, err:
            if err.errno != errno.ENOENT:
                raise

def safe_rmdir(dir):
    "Convenience function for removing temporary test directories"
    try:
        os.rmdir(dir)
    except OSError, err:
        if err.errno != errno.ENOENT:
            raise

def touch(name):
    """Create a file and return its name."""
    assert not os.path.isfile(name), name
    f = open(name, 'w')
    try:
        return f.name
    finally:
        f.close()

def onexit():
    """Convenience function for removing temporary files and
    directories on interpreter exit.
    Also closes all sockets/instances left behind in asyncore
    socket map (if any).
    """
    for name in os.listdir('.'):
        if name.startswith(tempfile.template):
            if os.path.isdir(name):
                shutil.rmtree(name)
            else:
                os.remove(name)
    map = asyncore.socket_map
    for x in map.values():
        try:
            sys.stderr.write("garbage: %s\n" % repr(x))
            x.close()
        except:
            pass
    map.clear()


# commented out as per bug http://bugs.python.org/issue10354
#tempfile.template = 'tmp-pyftpdlib'
atexit.register(onexit)

# lower this threshold so that the scheduler internal queue
# gets re-heapified more often
ftpserver._scheduler.cancellations_threshold = 5


class FTPd(threading.Thread):
    """A threaded FTP server used for running tests.

    This is basically a modified version of the FTPServer class which
    wraps the polling loop into a thread.

    The instance returned can be used to start(), stop() and
    eventually re-start() the server.
    """
    handler = ftpserver.FTPHandler

    def __init__(self, host=HOST, port=0, verbose=False):
        threading.Thread.__init__(self)
        self.__serving = False
        self.__stopped = False
        self.__lock = threading.Lock()
        self.__flag = threading.Event()

        if not verbose:
            ftpserver.log = ftpserver.logline = lambda x: x

        # this makes the threaded server raise an actual exception
        # instead of just logging its traceback
        def logerror(msg):
            raise
        ftpserver.logerror = logerror
        authorizer = ftpserver.DummyAuthorizer()
        authorizer.add_user(USER, PASSWD, HOME, perm='elradfmwM')  # full perms
        authorizer.add_anonymous(HOME)
        self.handler.authorizer = authorizer
        self.server = ftpserver.FTPServer((host, port), self.handler)
        self.host, self.port = self.server.socket.getsockname()[:2]

    def __repr__(self):
        status = [self.__class__.__module__ + "." + self.__class__.__name__]
        if self.__serving:
            status.append('active')
        else:
            status.append('inactive')
        status.append('%s:%s' % self.server.socket.getsockname()[:2])
        return '<%s at %#x>' % (' '.join(status), id(self))

    @property
    def running(self):
        return self.__serving

    def start(self, timeout=0.001, use_poll=False):
        """Start serving until an explicit stop() request.
        Polls for shutdown every 'timeout' seconds.
        """
        if self.__serving:
            raise RuntimeError("Server already started")
        if self.__stopped:
            # ensure the server can be started again
            FTPd.__init__(self, self.server.socket.getsockname(), self.handler)
        self.__timeout = timeout
        self.__use_poll = use_poll
        threading.Thread.start(self)
        self.__flag.wait()

    def run(self):
        self.__serving = True
        self.__flag.set()
        while self.__serving and asyncore.socket_map:
            self.__lock.acquire()
            self.server.serve_forever(timeout=self.__timeout, count=1,
                                      use_poll=self.__use_poll)
            self.__lock.release()
        self.server.close_all()

    def stop(self):
        """Stop serving (also disconnecting all currently connected
        clients) by telling the serve_forever() loop to stop and
        waits until it does.
        """
        if not self.__serving:
            raise RuntimeError("Server not started yet")
        self.__serving = False
        self.__stopped = True
        self.join()


class TestAbstractedFS(unittest.TestCase):
    """Test for conversion utility methods of AbstractedFS class."""

    def setUp(self):
        safe_remove(TESTFN)

    tearDown = setUp

    def test_ftpnorm(self):
        # Tests for ftpnorm method.
        ae = self.assertEquals
        fs = ftpserver.AbstractedFS('/', None)

        fs._cwd = '/'
        ae(fs.ftpnorm(''), '/')
        ae(fs.ftpnorm('/'), '/')
        ae(fs.ftpnorm('.'), '/')
        ae(fs.ftpnorm('..'), '/')
        ae(fs.ftpnorm('a'), '/a')
        ae(fs.ftpnorm('/a'), '/a')
        ae(fs.ftpnorm('/a/'), '/a')
        ae(fs.ftpnorm('a/..'), '/')
        ae(fs.ftpnorm('a/b'), '/a/b')
        ae(fs.ftpnorm('a/b/..'), '/a')
        ae(fs.ftpnorm('a/b/../..'), '/')
        fs._cwd = '/sub'
        ae(fs.ftpnorm(''), '/sub')
        ae(fs.ftpnorm('/'), '/')
        ae(fs.ftpnorm('.'), '/sub')
        ae(fs.ftpnorm('..'), '/')
        ae(fs.ftpnorm('a'), '/sub/a')
        ae(fs.ftpnorm('a/'), '/sub/a')
        ae(fs.ftpnorm('a/..'), '/sub')
        ae(fs.ftpnorm('a/b'), '/sub/a/b')
        ae(fs.ftpnorm('a/b/'), '/sub/a/b')
        ae(fs.ftpnorm('a/b/..'), '/sub/a')
        ae(fs.ftpnorm('a/b/../..'), '/sub')
        ae(fs.ftpnorm('a/b/../../..'), '/')
        ae(fs.ftpnorm('//'), '/')  # UNC paths must be collapsed

    def test_ftp2fs(self):
        # Tests for ftp2fs method.
        ae = self.assertEquals
        fs = ftpserver.AbstractedFS('/', None)
        join = lambda x, y: os.path.join(x, y.replace('/', os.sep))

        def goforit(root):
            fs._root = root
            fs._cwd = '/'
            ae(fs.ftp2fs(''), root)
            ae(fs.ftp2fs('/'), root)
            ae(fs.ftp2fs('.'), root)
            ae(fs.ftp2fs('..'), root)
            ae(fs.ftp2fs('a'), join(root, 'a'))
            ae(fs.ftp2fs('/a'), join(root, 'a'))
            ae(fs.ftp2fs('/a/'), join(root, 'a'))
            ae(fs.ftp2fs('a/..'), root)
            ae(fs.ftp2fs('a/b'), join(root, r'a/b'))
            ae(fs.ftp2fs('/a/b'), join(root, r'a/b'))
            ae(fs.ftp2fs('/a/b/..'), join(root, 'a'))
            ae(fs.ftp2fs('/a/b/../..'), root)
            fs._cwd = '/sub'
            ae(fs.ftp2fs(''), join(root, 'sub'))
            ae(fs.ftp2fs('/'), root)
            ae(fs.ftp2fs('.'), join(root, 'sub'))
            ae(fs.ftp2fs('..'), root)
            ae(fs.ftp2fs('a'), join(root, 'sub/a'))
            ae(fs.ftp2fs('a/'), join(root, 'sub/a'))
            ae(fs.ftp2fs('a/..'), join(root, 'sub'))
            ae(fs.ftp2fs('a/b'), join(root, 'sub/a/b'))
            ae(fs.ftp2fs('a/b/..'), join(root, 'sub/a'))
            ae(fs.ftp2fs('a/b/../..'), join(root, 'sub'))
            ae(fs.ftp2fs('a/b/../../..'), root)
            ae(fs.ftp2fs('//a'), join(root, 'a'))  # UNC paths must be collapsed

        if os.sep == '\\':
            goforit(r'C:\dir')
            goforit('C:\\')
            # on DOS-derived filesystems (e.g. Windows) this is the same
            # as specifying the current drive directory (e.g. 'C:\\')
            goforit('\\')
        elif os.sep == '/':
            goforit('/home/user')
            goforit('/')
        else:
            # os.sep == ':'? Don't know... let's try it anyway
            goforit(os.getcwd())

    def test_fs2ftp(self):
        # Tests for fs2ftp method.
        ae = self.assertEquals
        fs = ftpserver.AbstractedFS('/', None)
        join = lambda x, y: os.path.join(x, y.replace('/', os.sep))

        def goforit(root):
            fs._root = root
            ae(fs.fs2ftp(root), '/')
            ae(fs.fs2ftp(join(root, '/')), '/')
            ae(fs.fs2ftp(join(root, '.')), '/')
            ae(fs.fs2ftp(join(root, '..')), '/')  # can't escape from root
            ae(fs.fs2ftp(join(root, 'a')), '/a')
            ae(fs.fs2ftp(join(root, 'a/')), '/a')
            ae(fs.fs2ftp(join(root, 'a/..')), '/')
            ae(fs.fs2ftp(join(root, 'a/b')), '/a/b')
            ae(fs.fs2ftp(join(root, 'a/b')), '/a/b')
            ae(fs.fs2ftp(join(root, 'a/b/..')), '/a')
            ae(fs.fs2ftp(join(root, '/a/b/../..')), '/')
            fs._cwd = '/sub'
            ae(fs.fs2ftp(join(root, 'a/')), '/a')

        if os.sep == '\\':
            goforit(r'C:\dir')
            goforit('C:\\')
            # on DOS-derived filesystems (e.g. Windows) this is the same
            # as specifying the current drive directory (e.g. 'C:\\')
            goforit('\\')
            fs._root = r'C:\dir'
            ae(fs.fs2ftp('C:\\'), '/')
            ae(fs.fs2ftp('D:\\'), '/')
            ae(fs.fs2ftp('D:\\dir'), '/')
        elif os.sep == '/':
            goforit('/')
            if os.path.realpath('/__home/user') != '/__home/user':
                self.fail('Test skipped (symlinks not allowed).')
            goforit('/__home/user')
            fs._root = '/__home/user'
            ae(fs.fs2ftp('/__home'), '/')
            ae(fs.fs2ftp('/'), '/')
            ae(fs.fs2ftp('/__home/userx'), '/')
        else:
            # os.sep == ':'? Don't know... let's try it anyway
            goforit(os.getcwd())

    def test_validpath(self):
        # Tests for validpath method.
        fs = ftpserver.AbstractedFS('/', None)
        fs._root = HOME
        self.assertTrue(fs.validpath(HOME))
        self.assertTrue(fs.validpath(HOME + '/'))
        self.assertFalse(fs.validpath(HOME + 'bar'))

    if hasattr(os, 'symlink'):

        def test_validpath_validlink(self):
            # Test validpath by issuing a symlink pointing to a path
            # inside the root directory.
            fs = ftpserver.AbstractedFS('/', None)
            fs._root = HOME
            TESTFN2 = TESTFN + '1'
            try:
                touch(TESTFN)
                os.symlink(TESTFN, TESTFN2)
                self.assertTrue(fs.validpath(TESTFN))
            finally:
                safe_remove(TESTFN, TESTFN2)

        def test_validpath_external_symlink(self):
            # Test validpath by issuing a symlink pointing to a path
            # outside the root directory.
            fs = ftpserver.AbstractedFS('/', None)
            fs._root = HOME
            # tempfile should create our file in /tmp directory
            # which should be outside the user root.  If it is
            # not we just skip the test.
            file = tempfile.NamedTemporaryFile()
            try:
                if HOME == os.path.dirname(file.name):
                    return
                os.symlink(file.name, TESTFN)
                self.assertFalse(fs.validpath(TESTFN))
            finally:
                safe_remove(TESTFN)
                file.close()


class TestDummyAuthorizer(unittest.TestCase):
    """Tests for DummyAuthorizer class."""

    # temporarily change warnings to exceptions for the purposes of testing
    def setUp(self):
        self.tempdir = tempfile.mkdtemp(dir=HOME)
        self.subtempdir = tempfile.mkdtemp(dir=os.path.join(HOME, self.tempdir))
        self.tempfile = touch(os.path.join(self.tempdir, TESTFN))
        self.subtempfile = touch(os.path.join(self.subtempdir, TESTFN))
        warnings.filterwarnings("error")

    def tearDown(self):
        os.remove(self.tempfile)
        os.remove(self.subtempfile)
        os.rmdir(self.subtempdir)
        os.rmdir(self.tempdir)
        warnings.resetwarnings()

    def assertRaisesWithMsg(self, excClass, msg, callableObj, *args, **kwargs):
        try:
            callableObj(*args, **kwargs)
        except excClass, why:
            if str(why) == msg:
                return
            raise self.failureException("%s != %s" % (str(why), msg))
        else:
            if hasattr(excClass,'__name__'): excName = excClass.__name__
            else: excName = str(excClass)
            raise self.failureException, "%s not raised" % excName

    def test_common_methods(self):
        auth = ftpserver.DummyAuthorizer()
        # create user
        auth.add_user(USER, PASSWD, HOME)
        auth.add_anonymous(HOME)
        # check credentials
        self.assertTrue(auth.validate_authentication(USER, PASSWD))
        self.assertFalse(auth.validate_authentication(USER, 'wrongpwd'))
        # remove them
        auth.remove_user(USER)
        auth.remove_user('anonymous')
        # raise exc if user does not exists
        self.assertRaises(KeyError, auth.remove_user, USER)
        # raise exc if path does not exist
        self.assertRaisesWithMsg(ValueError,
                                'no such directory: "%s"' % '?:\\',
                                 auth.add_user, USER, PASSWD, '?:\\')
        self.assertRaisesWithMsg(ValueError,
                                'no such directory: "%s"' % '?:\\',
                                 auth.add_anonymous, '?:\\')
        # raise exc if user already exists
        auth.add_user(USER, PASSWD, HOME)
        auth.add_anonymous(HOME)
        self.assertRaisesWithMsg(ValueError,
                                'user "%s" already exists' % USER,
                                 auth.add_user, USER, PASSWD, HOME)
        self.assertRaisesWithMsg(ValueError,
                                'user "anonymous" already exists',
                                 auth.add_anonymous, HOME)
        auth.remove_user(USER)
        auth.remove_user('anonymous')
        # raise on wrong permission
        self.assertRaisesWithMsg(ValueError,
                                 'no such permission "?"',
                                 auth.add_user, USER, PASSWD, HOME, perm='?')
        self.assertRaisesWithMsg(ValueError,
                                 'no such permission "?"',
                                 auth.add_anonymous, HOME, perm='?')
        # expect warning on write permissions assigned to anonymous user
        for x in "adfmw":
            self.assertRaisesWithMsg(RuntimeWarning,
                                "write permissions assigned to anonymous user.",
                                auth.add_anonymous, HOME, perm=x)

    def test_override_perm_interface(self):
        auth = ftpserver.DummyAuthorizer()
        auth.add_user(USER, PASSWD, HOME, perm='elr')
        # raise exc if user does not exists
        self.assertRaises(KeyError, auth.override_perm, USER+'w', HOME, 'elr')
        # raise exc if path does not exist or it's not a directory
        self.assertRaisesWithMsg(ValueError,
                                'no such directory: "%s"' % '?:\\',
                                auth.override_perm, USER, '?:\\', 'elr')
        self.assertRaisesWithMsg(ValueError,
                                'no such directory: "%s"' % self.tempfile,
                                auth.override_perm, USER, self.tempfile, 'elr')
        # raise on wrong permission
        self.assertRaisesWithMsg(ValueError,
                                 'no such permission "?"', auth.override_perm,
                                 USER, HOME, perm='?')
        # expect warning on write permissions assigned to anonymous user
        auth.add_anonymous(HOME)
        for p in "adfmw":
            self.assertRaisesWithMsg(RuntimeWarning,
                                "write permissions assigned to anonymous user.",
                                auth.override_perm, 'anonymous', HOME, p)
        # raise on attempt to override home directory permissions
        self.assertRaisesWithMsg(ValueError,
                                 "can't override home directory permissions",
                                 auth.override_perm, USER, HOME, perm='w')
        # raise on attempt to override a path escaping home directory
        if os.path.dirname(HOME) != HOME:
            self.assertRaisesWithMsg(ValueError,
                                     "path escapes user home directory",
                                     auth.override_perm, USER,
                                     os.path.dirname(HOME), perm='w')
        # try to re-set an overridden permission
        auth.override_perm(USER, self.tempdir, perm='w')
        auth.override_perm(USER, self.tempdir, perm='wr')

    def test_override_perm_recursive_paths(self):
        auth = ftpserver.DummyAuthorizer()
        auth.add_user(USER, PASSWD, HOME, perm='elr')
        self.assertEqual(auth.has_perm(USER, 'w', self.tempdir), False)
        auth.override_perm(USER, self.tempdir, perm='w', recursive=True)
        self.assertEqual(auth.has_perm(USER, 'w', HOME), False)
        self.assertEqual(auth.has_perm(USER, 'w', self.tempdir), True)
        self.assertEqual(auth.has_perm(USER, 'w', self.tempfile), True)
        self.assertEqual(auth.has_perm(USER, 'w', self.subtempdir), True)
        self.assertEqual(auth.has_perm(USER, 'w', self.subtempfile), True)

        self.assertEqual(auth.has_perm(USER, 'w', HOME + '@'), False)
        self.assertEqual(auth.has_perm(USER, 'w', self.tempdir + '@'), False)
        path = os.path.join(self.tempdir + '@', os.path.basename(self.tempfile))
        self.assertEqual(auth.has_perm(USER, 'w', path), False)
        # test case-sensitiveness
        if (os.name in ('nt', 'ce')) or (sys.platform == 'cygwin'):
            self.assertEqual(auth.has_perm(USER, 'w', self.tempdir.upper()), True)

    def test_override_perm_not_recursive_paths(self):
        auth = ftpserver.DummyAuthorizer()
        auth.add_user(USER, PASSWD, HOME, perm='elr')
        self.assertEqual(auth.has_perm(USER, 'w', self.tempdir), False)
        auth.override_perm(USER, self.tempdir, perm='w')
        self.assertEqual(auth.has_perm(USER, 'w', HOME), False)
        self.assertEqual(auth.has_perm(USER, 'w', self.tempdir), True)
        self.assertEqual(auth.has_perm(USER, 'w', self.tempfile), True)
        self.assertEqual(auth.has_perm(USER, 'w', self.subtempdir), False)
        self.assertEqual(auth.has_perm(USER, 'w', self.subtempfile), False)

        self.assertEqual(auth.has_perm(USER, 'w', HOME + '@'), False)
        self.assertEqual(auth.has_perm(USER, 'w', self.tempdir + '@'), False)
        path = os.path.join(self.tempdir + '@', os.path.basename(self.tempfile))
        self.assertEqual(auth.has_perm(USER, 'w', path), False)
        # test case-sensitiveness
        if (os.name in ('nt', 'ce')) or (sys.platform == 'cygwin'):
            self.assertEqual(auth.has_perm(USER, 'w', self.tempdir.upper()), True)


class TestCallLater(unittest.TestCase):
    """Tests for CallLater class."""

    def setUp(self):
        for task in ftpserver._scheduler._tasks:
            if not task.cancelled:
                task.cancel()
        del ftpserver._scheduler._tasks[:]

    def scheduler(self, timeout=0.01, count=100):
        while ftpserver._scheduler._tasks and count > 0:
            ftpserver._scheduler()
            count -= 1
            time.sleep(timeout)

    def test_interface(self):
        fun = lambda: 0
        self.assertRaises(AssertionError, ftpserver.CallLater, -1, fun)
        x = ftpserver.CallLater(3, fun)
        self.assertRaises(AssertionError, x.delay, -1)
        self.assertEqual(x.cancelled, False)
        x.cancel()
        self.assertEqual(x.cancelled, True)
        self.assertRaises(AssertionError, x.call)
        self.assertRaises(AssertionError, x.reset)
        self.assertRaises(AssertionError, x.delay, 2)
        self.assertRaises(AssertionError, x.cancel)

    def test_order(self):
        l = []
        fun = lambda x: l.append(x)
        for x in [0.05, 0.04, 0.03, 0.02, 0.01]:
            ftpserver.CallLater(x, fun, x)
        self.scheduler()
        self.assertEqual(l, [0.01, 0.02, 0.03, 0.04, 0.05])

    def test_delay(self):
        l = []
        fun = lambda x: l.append(x)
        ftpserver.CallLater(0.01, fun, 0.01).delay(0.07)
        ftpserver.CallLater(0.02, fun, 0.02).delay(0.08)
        ftpserver.CallLater(0.03, fun, 0.03)
        ftpserver.CallLater(0.04, fun, 0.04)
        ftpserver.CallLater(0.05, fun, 0.05)
        ftpserver.CallLater(0.06, fun, 0.06).delay(0.001)
        self.scheduler()
        self.assertEqual(l, [0.06, 0.03, 0.04, 0.05, 0.01, 0.02])

    # The test is reliable only on those systems where time.time()
    # provides time with a better precision than 1 second.
    if not str(time.time()).endswith('.0'):
        def test_reset(self):
            l = []
            fun = lambda x: l.append(x)
            ftpserver.CallLater(0.01, fun, 0.01)
            ftpserver.CallLater(0.02, fun, 0.02)
            ftpserver.CallLater(0.03, fun, 0.03)
            x = ftpserver.CallLater(0.04, fun, 0.04)
            ftpserver.CallLater(0.05, fun, 0.05)
            time.sleep(0.1)
            x.reset()
            self.scheduler()
            self.assertEqual(l, [0.01, 0.02, 0.03, 0.05, 0.04])

    def test_cancel(self):
        l = []
        fun = lambda x: l.append(x)
        ftpserver.CallLater(0.01, fun, 0.01).cancel()
        ftpserver.CallLater(0.02, fun, 0.02)
        ftpserver.CallLater(0.03, fun, 0.03)
        ftpserver.CallLater(0.04, fun, 0.04)
        ftpserver.CallLater(0.05, fun, 0.05).cancel()
        self.scheduler()
        self.assertEqual(l, [0.02, 0.03, 0.04])

    def test_errback(self):
        l = []
        ftpserver.CallLater(0.0, lambda: 1//0, _errback=lambda: l.append(True))
        self.scheduler()
        self.assertEqual(l, [True])


class TestCallEvery(unittest.TestCase):
    """Tests for CallEvery class."""

    def setUp(self):
        for task in ftpserver._scheduler._tasks:
            if not task.cancelled:
                task.cancel()
        del ftpserver._scheduler._tasks[:]

    def scheduler(self, timeout=0.0001):
        for x in range(100):
            ftpserver._scheduler()
            time.sleep(timeout)

    def test_interface(self):
        fun = lambda: 0
        self.assertRaises(AssertionError, ftpserver.CallEvery, -1, fun)
        x = ftpserver.CallEvery(3, fun)
        self.assertRaises(AssertionError, x.delay, -1)
        self.assertEqual(x.cancelled, False)
        x.cancel()
        self.assertEqual(x.cancelled, True)
        self.assertRaises(AssertionError, x.call)
        self.assertRaises(AssertionError, x.reset)
        self.assertRaises(AssertionError, x.delay, 2)
        self.assertRaises(AssertionError, x.cancel)

    def test_only_once(self):
        # make sure that callback is called only once per-loop
        l1 = []
        fun = lambda: l1.append(None)
        ftpserver.CallEvery(0, fun)
        ftpserver._scheduler()
        self.assertEqual(l1, [None])

    def test_multi_0_timeout(self):
        # make sure a 0 timeout callback is called as many times
        # as the number of loops
        l = []
        fun = lambda: l.append(None)
        ftpserver.CallEvery(0, fun)
        self.scheduler()
        self.assertEqual(len(l), 100)

    # run it on systems where time.time() has a higher precision
    if os.name == 'posix':
        def test_low_and_high_timeouts(self):
            # make sure a callback with a lower timeout is called more
            # frequently than another with a greater timeout
            l1 = []
            fun = lambda: l1.append(None)
            ftpserver.CallEvery(0.001, fun)
            self.scheduler()

            l2 = []
            fun = lambda: l2.append(None)
            ftpserver.CallEvery(0.01, fun)
            self.scheduler()

            self.assertTrue(len(l1) > len(l2))

    def test_cancel(self):
        # make sure a cancelled callback doesn't get called anymore
        l = []
        fun = lambda: l.append(None)
        call = ftpserver.CallEvery(0.001, fun)
        self.scheduler()
        len_l = len(l)
        call.cancel()
        self.scheduler()
        self.assertEqual(len_l, len(l))

    def test_errback(self):
        l = []
        ftpserver.CallEvery(0.0, lambda: 1//0, _errback=lambda: l.append(True))
        self.scheduler()
        self.assertTrue(l)


class TestFtpAuthentication(unittest.TestCase):
    "test: USER, PASS, REIN."
    server_class = FTPd
    client_class = ftplib.FTP

    def setUp(self):
        self.server = self.server_class()
        self.server.handler._auth_failed_timeout = 0
        self.server.start()
        self.client = self.client_class()
        self.client.connect(self.server.host, self.server.port)
        self.client.sock.settimeout(2)
        self.file = open(TESTFN, 'w+b')
        self.dummyfile = StringIO.StringIO()

    def tearDown(self):
        self.server.handler._auth_failed_timeout = 5
        self.client.close()
        self.server.stop()
        if not self.file.closed:
            self.file.close()
        if not self.dummyfile.closed:
            self.dummyfile.close()
        os.remove(TESTFN)

    def test_auth_ok(self):
        self.client.login(user=USER, passwd=PASSWD)

    def test_anon_auth(self):
        self.client.login(user='anonymous', passwd='anon@')
        self.client.login(user='anonymous', passwd='')
        self.assertRaises(ftplib.error_perm, self.client.login, 'AnoNymouS')

    def test_auth_failed(self):
        self.assertRaises(ftplib.error_perm, self.client.login, USER, 'wrong')
        self.assertRaises(ftplib.error_perm, self.client.login, 'wrong', PASSWD)
        self.assertRaises(ftplib.error_perm, self.client.login, 'wrong', 'wrong')

    def test_wrong_cmds_order(self):
        self.assertRaises(ftplib.error_perm, self.client.sendcmd, 'pass ' + PASSWD)
        self.client.login(user=USER, passwd=PASSWD)
        self.assertRaises(ftplib.error_perm, self.client.sendcmd, 'pass ' + PASSWD)

    def test_max_auth(self):
        self.assertRaises(ftplib.error_perm, self.client.login, USER, 'wrong')
        self.assertRaises(ftplib.error_perm, self.client.login, USER, 'wrong')
        self.assertRaises(ftplib.error_perm, self.client.login, USER, 'wrong')
        # If authentication fails for 3 times ftpd disconnects the
        # client.  We can check if that happens by using self.client.sendcmd()
        # on the 'dead' socket object.  If socket object is really
        # closed it should be raised a socket.error exception (Windows)
        # or a EOFError exception (Linux).
        self.assertRaises((socket.error, EOFError), self.client.sendcmd, '')

    def test_rein(self):
        self.client.login(user=USER, passwd=PASSWD)
        self.client.sendcmd('rein')
        # user not authenticated, error response expected
        self.assertRaises(ftplib.error_perm, self.client.sendcmd, 'pwd')
        # by logging-in again we should be able to execute a
        # file-system command
        self.client.login(user=USER, passwd=PASSWD)
        self.client.sendcmd('pwd')

    def test_rein_during_transfer(self):
        # Test REIN while already authenticated and a transfer is
        # in progress.
        self.client.login(user=USER, passwd=PASSWD)
        data = 'abcde12345' * 1000000
        self.file.write(data)
        self.file.close()

        conn = self.client.transfercmd('retr ' + TESTFN)
        rein_sent = False
        bytes_recv = 0
        while 1:
            chunk = conn.recv(8192)
            if not chunk:
                break
            bytes_recv += len(chunk)
            self.dummyfile.write(chunk)
            if bytes_recv > 65536 and not rein_sent:
                rein_sent = True
                # flush account, error response expected
                self.client.sendcmd('rein')
                self.assertRaises(ftplib.error_perm, self.client.dir)

        # a 226 response is expected once tranfer finishes
        self.assertEqual(self.client.voidresp()[:3], '226')
        # account is still flushed, error response is still expected
        self.assertRaises(ftplib.error_perm, self.client.sendcmd,
                          'size ' + TESTFN)
        # by logging-in again we should be able to execute a
        # filesystem command
        self.client.login(user=USER, passwd=PASSWD)
        self.client.sendcmd('pwd')
        self.dummyfile.seek(0)
        self.assertEqual(hash(data), hash (self.dummyfile.read()))

    def test_user(self):
        # Test USER while already authenticated and no transfer
        # is in progress.
        self.client.login(user=USER, passwd=PASSWD)
        self.client.sendcmd('user ' + USER)  # authentication flushed
        self.assertRaises(ftplib.error_perm, self.client.sendcmd, 'pwd')
        self.client.sendcmd('pass ' + PASSWD)
        self.client.sendcmd('pwd')

    def test_user_during_transfer(self):
        # Test USER while already authenticated and a transfer is
        # in progress.
        self.client.login(user=USER, passwd=PASSWD)
        data = 'abcde12345' * 1000000
        self.file.write(data)
        self.file.close()

        conn = self.client.transfercmd('retr ' + TESTFN)
        rein_sent = 0
        bytes_recv = 0
        while 1:
            chunk = conn.recv(8192)
            if not chunk:
                break
            bytes_recv += len(chunk)
            self.dummyfile.write(chunk)
            # stop transfer while it isn't finished yet
            if bytes_recv > 65536 and not rein_sent:
                rein_sent = True
                # flush account, expect an error response
                self.client.sendcmd('user ' + USER)
                self.assertRaises(ftplib.error_perm, self.client.dir)

        # a 226 response is expected once transfer finishes
        self.assertEqual(self.client.voidresp()[:3], '226')
        # account is still flushed, error response is still expected
        self.assertRaises(ftplib.error_perm, self.client.sendcmd, 'pwd')
        # by logging-in again we should be able to execute a
        # filesystem command
        self.client.sendcmd('pass ' + PASSWD)
        self.client.sendcmd('pwd')
        self.dummyfile.seek(0)
        self.assertEqual(hash(data), hash (self.dummyfile.read()))


class TestFtpDummyCmds(unittest.TestCase):
    "test: TYPE, STRU, MODE, NOOP, SYST, ALLO, HELP, SITE HELP"
    server_class = FTPd
    client_class = ftplib.FTP

    def setUp(self):
        self.server = self.server_class()
        self.server.start()
        self.client = self.client_class()
        self.client.connect(self.server.host, self.server.port)
        self.client.sock.settimeout(2)
        self.client.login(USER, PASSWD)

    def tearDown(self):
        self.client.close()
        self.server.stop()

    def test_type(self):
        self.client.sendcmd('type a')
        self.client.sendcmd('type i')
        self.client.sendcmd('type l7')
        self.client.sendcmd('type l8')
        self.assertRaises(ftplib.error_perm, self.client.sendcmd, 'type ?!?')

    def test_stru(self):
        self.client.sendcmd('stru f')
        self.client.sendcmd('stru F')
        self.assertRaises(ftplib.error_perm, self.client.sendcmd, 'stru p')
        self.assertRaises(ftplib.error_perm, self.client.sendcmd, 'stru r')
        self.assertRaises(ftplib.error_perm, self.client.sendcmd, 'stru ?!?')

    def test_mode(self):
        self.client.sendcmd('mode s')
        self.client.sendcmd('mode S')
        self.assertRaises(ftplib.error_perm, self.client.sendcmd, 'mode b')
        self.assertRaises(ftplib.error_perm, self.client.sendcmd, 'mode c')
        self.assertRaises(ftplib.error_perm, self.client.sendcmd, 'mode ?!?')

    def test_noop(self):
        self.client.sendcmd('noop')

    def test_syst(self):
        self.client.sendcmd('syst')

    def test_allo(self):
        self.client.sendcmd('allo x')

    def test_quit(self):
        self.client.sendcmd('quit')

    def test_help(self):
        self.client.sendcmd('help')
        cmd = random.choice(ftpserver.proto_cmds.keys())
        self.client.sendcmd('help %s' % cmd)
        self.assertRaises(ftplib.error_perm, self.client.sendcmd, 'help ?!?')

    def test_site(self):
        self.assertRaises(ftplib.error_perm, self.client.sendcmd, 'site')
        self.assertRaises(ftplib.error_perm, self.client.sendcmd, 'site ?!?')
        self.assertRaises(ftplib.error_perm, self.client.sendcmd, 'site foo bar')
        self.assertRaises(ftplib.error_perm, self.client.sendcmd, 'sitefoo bar')

    def test_site_help(self):
        self.client.sendcmd('site help')
        self.client.sendcmd('site help help')
        self.assertRaises(ftplib.error_perm, self.client.sendcmd, 'site help ?!?')

    def test_rest(self):
        # Test error conditions only; resumed data transfers are
        # tested later.
        self.client.sendcmd('type i')
        self.assertRaises(ftplib.error_perm, self.client.sendcmd, 'rest')
        self.assertRaises(ftplib.error_perm, self.client.sendcmd, 'rest str')
        self.assertRaises(ftplib.error_perm, self.client.sendcmd, 'rest -1')
        self.assertRaises(ftplib.error_perm, self.client.sendcmd, 'rest 10.1')
        # REST is not supposed to be allowed in ASCII mode
        self.client.sendcmd('type a')
        self.assertRaises(ftplib.error_perm, self.client.sendcmd, 'rest 10')

    def test_opts_feat(self):
        self.assertRaises(ftplib.error_perm, self.client.sendcmd, 'opts mlst bad_fact')
        self.assertRaises(ftplib.error_perm, self.client.sendcmd, 'opts mlst type ;')
        self.assertRaises(ftplib.error_perm, self.client.sendcmd, 'opts not_mlst')
        # utility function which used for extracting the MLST "facts"
        # string from the FEAT response
        def mlst():
            resp = self.client.sendcmd('feat')
            return re.search(r'^\s*MLST\s+(\S+)$', resp, re.MULTILINE).group(1)
        # we rely on "type", "perm", "size", and "modify" facts which
        # are those available on all platforms
        self.assertTrue('type*;perm*;size*;modify*;' in mlst())
        self.assertEqual(self.client.sendcmd('opts mlst type;'), '200 MLST OPTS type;')
        self.assertEqual(self.client.sendcmd('opts mLSt TypE;'), '200 MLST OPTS type;')
        self.assertTrue('type*;perm;size;modify;' in mlst())

        self.assertEqual(self.client.sendcmd('opts mlst'), '200 MLST OPTS ')
        self.assertTrue(not '*' in mlst())

        self.assertEqual(self.client.sendcmd('opts mlst fish;cakes;'), '200 MLST OPTS ')
        self.assertTrue(not '*' in mlst())
        self.assertEqual(self.client.sendcmd('opts mlst fish;cakes;type;'),
                         '200 MLST OPTS type;')
        self.assertTrue('type*;perm;size;modify;' in mlst())


class TestFtpCmdsSemantic(unittest.TestCase):
    server_class = FTPd
    client_class = ftplib.FTP
    arg_cmds = ['allo','appe','dele','eprt','mdtm','mode','mkd','opts','port',
                'rest','retr','rmd','rnfr','rnto','site','size','stor','stru',
                'type','user','xmkd','xrmd','site chmod']

    def setUp(self):
        self.server = self.server_class()
        self.server.start()
        self.client = self.client_class()
        self.client.connect(self.server.host, self.server.port)
        self.client.sock.settimeout(2)
        self.client.login(USER, PASSWD)

    def tearDown(self):
        self.client.close()
        self.server.stop()

    def test_arg_cmds(self):
        # Test commands requiring an argument.
        expected = "501 Syntax error: command needs an argument."
        for cmd in self.arg_cmds:
            self.client.putcmd(cmd)
            resp = self.client.getmultiline()
            self.assertEqual(resp, expected)

    def test_no_arg_cmds(self):
        # Test commands accepting no arguments.
        expected = "501 Syntax error: command does not accept arguments."
        for cmd in ('abor','cdup','feat','noop','pasv','pwd','quit','rein',
                    'syst','xcup','xpwd'):
            self.client.putcmd(cmd + ' arg')
            resp = self.client.getmultiline()
            self.assertEqual(resp, expected)

    def test_auth_cmds(self):
        # Test those commands requiring client to be authenticated.
        expected = "530 Log in with USER and PASS first."
        self.client.sendcmd('rein')
        for cmd in self.server.handler.proto_cmds:
            cmd = cmd.lower()
            if cmd in ('feat','help','noop','user','pass','stat','syst','quit',
                       'site', 'site help', 'pbsz', 'auth', 'prot', 'ccc'):
                continue
            if cmd in self.arg_cmds:
                cmd = cmd + ' arg'
            self.client.putcmd(cmd)
            resp = self.client.getmultiline()
            self.assertEqual(resp, expected)

    def test_no_auth_cmds(self):
        # Test those commands that do not require client to be authenticated.
        self.client.sendcmd('rein')
        for cmd in ('feat','help','noop','stat','syst','site help'):
            self.client.sendcmd(cmd)
        # STAT provided with an argument is equal to LIST hence not allowed
        # if not authenticated
        self.assertRaises(ftplib.error_perm, self.client.sendcmd, 'stat /')
        self.client.sendcmd('quit')


class TestFtpFsOperations(unittest.TestCase):
    "test: PWD, CWD, CDUP, SIZE, RNFR, RNTO, DELE, MKD, RMD, MDTM, STAT"
    server_class = FTPd
    client_class = ftplib.FTP

    def setUp(self):
        self.server = self.server_class()
        self.server.start()
        self.client = self.client_class()
        self.client.connect(self.server.host, self.server.port)
        self.client.sock.settimeout(2)
        self.client.login(USER, PASSWD)
        self.tempfile = os.path.basename(touch(TESTFN))
        self.tempdir = os.path.basename(tempfile.mkdtemp(dir=HOME))

    def tearDown(self):
        self.client.close()
        self.server.stop()
        safe_remove(self.tempfile)
        if os.path.exists(self.tempdir):
            shutil.rmtree(self.tempdir)

    def test_cwd(self):
        self.client.cwd(self.tempdir)
        self.assertEqual(self.client.pwd(), '/' + self.tempdir)
        self.assertRaises(ftplib.error_perm, self.client.cwd, 'subtempdir')
        # cwd provided with no arguments is supposed to move us to the
        # root directory
        self.client.sendcmd('cwd')
        self.assertEqual(self.client.pwd(), '/')

    def test_pwd(self):
        self.assertEqual(self.client.pwd(), '/')
        self.client.cwd(self.tempdir)
        self.assertEqual(self.client.pwd(), '/' + self.tempdir)

    def test_cdup(self):
        subfolder = os.path.basename(tempfile.mkdtemp(dir=self.tempdir))
        self.assertEqual(self.client.pwd(), '/')
        self.client.cwd(self.tempdir)
        self.assertEqual(self.client.pwd(), '/%s' % self.tempdir)
        self.client.cwd(subfolder)
        self.assertEqual(self.client.pwd(), '/%s/%s' % (self.tempdir, subfolder))
        self.client.sendcmd('cdup')
        self.assertEqual(self.client.pwd(), '/%s' % self.tempdir)
        self.client.sendcmd('cdup')
        self.assertEqual(self.client.pwd(), '/')

        # make sure we can't escape from root directory
        self.client.sendcmd('cdup')
        self.assertEqual(self.client.pwd(), '/')

    def test_mkd(self):
        tempdir = os.path.basename(tempfile.mktemp(dir=HOME))
        dirname = self.client.mkd(tempdir)
        # the 257 response is supposed to include the absolute dirname
        self.assertEqual(dirname, '/' + tempdir)
        # make sure we can't create directories which already exist
        # (probably not really necessary);
        # let's use a try/except statement to avoid leaving behind
        # orphaned temporary directory in the event of a test failure.
        try:
            self.client.mkd(tempdir)
        except ftplib.error_perm:
            os.rmdir(tempdir)  # ok
        else:
            self.fail('ftplib.error_perm not raised.')

    def test_rmd(self):
        self.client.rmd(self.tempdir)
        self.assertRaises(ftplib.error_perm, self.client.rmd, self.tempfile)
        # make sure we can't remove the root directory
        self.assertRaises(ftplib.error_perm, self.client.rmd, '/')

    def test_dele(self):
        self.client.delete(self.tempfile)
        self.assertRaises(ftplib.error_perm, self.client.delete, self.tempdir)

    def test_rnfr_rnto(self):
        # rename file
        tempname = os.path.basename(tempfile.mktemp(dir=HOME))
        self.client.rename(self.tempfile, tempname)
        self.client.rename(tempname, self.tempfile)
        # rename dir
        tempname = os.path.basename(tempfile.mktemp(dir=HOME))
        self.client.rename(self.tempdir, tempname)
        self.client.rename(tempname, self.tempdir)
        # rnfr/rnto over non-existing paths
        bogus = os.path.basename(tempfile.mktemp(dir=HOME))
        self.assertRaises(ftplib.error_perm, self.client.rename, bogus, '/x')
        self.assertRaises(ftplib.error_perm, self.client.rename, self.tempfile, '/')
        # rnto sent without first specifying the source
        self.assertRaises(ftplib.error_perm, self.client.sendcmd, 'rnto ' + self.tempfile)

        # make sure we can't rename root directory
        self.assertRaises(ftplib.error_perm, self.client.rename, '/', '/x')

    def test_mdtm(self):
        self.client.sendcmd('mdtm ' + self.tempfile)
        bogus = os.path.basename(tempfile.mktemp(dir=HOME))
        self.assertRaises(ftplib.error_perm, self.client.sendcmd, 'mdtm ' + bogus)
        # make sure we can't use mdtm against directories
        try:
            self.client.sendcmd('mdtm ' + self.tempdir)
        except ftplib.error_perm, err:
            self.assertTrue("not retrievable" in str(err))
        else:
            self.fail('Exception not raised')

    def test_unforeseen_mdtm_event(self):
        # Emulate a case where the file last modification time is prior
        # to year 1900.  This most likely will never happen unless
        # someone specifically force the last modification time of a
        # file in some way.
        # To do so we temporarily override os.path.getmtime so that it
        # returns a negative value referring to a year prior to 1900.
        # It causes time.localtime/gmtime to raise a ValueError exception
        # which is supposed to be handled by server.
        _getmtime = ftpserver.AbstractedFS.getmtime
        try:
            ftpserver.AbstractedFS.getmtime = lambda x, y: -9000000000
            self.assertRaises(ftplib.error_perm, self.client.sendcmd,
                              'mdtm ' + self.tempfile)
            # make sure client hasn't been disconnected
            self.client.sendcmd('noop')
        finally:
            ftpserver.AbstractedFS.getmtime = _getmtime

    def test_size(self):
        self.client.sendcmd('type a')
        self.assertRaises(ftplib.error_perm, self.client.size, self.tempfile)
        self.client.sendcmd('type i')
        self.client.size(self.tempfile)
        # make sure we can't use size against directories
        try:
            self.client.sendcmd('size ' + self.tempdir)
        except ftplib.error_perm, err:
            self.assertTrue("not retrievable" in str(err))
        else:
            self.fail('Exception not raised')

    if not hasattr(os, 'chmod'):
        def test_site_chmod(self):
            self.assertRaises(ftplib.error_perm, self.client.sendcmd,
                              'site chmod 777 ' + self.tempfile)
    else:
        def test_site_chmod(self):
            # not enough args
            self.assertRaises(ftplib.error_perm,
                              self.client.sendcmd, 'site chmod 777')
            # bad args
            self.assertRaises(ftplib.error_perm, self.client.sendcmd,
                              'site chmod -177 ' + self.tempfile)
            self.assertRaises(ftplib.error_perm, self.client.sendcmd,
                              'site chmod 778 ' + self.tempfile)
            self.assertRaises(ftplib.error_perm, self.client.sendcmd,
                              'site chmod foo ' + self.tempfile)

            # on Windows it is possible to set read-only flag only
            if os.name == 'nt':
                self.client.sendcmd('site chmod 777 ' + self.tempfile)
                mode = oct(stat.S_IMODE(os.stat(self.tempfile).st_mode))
                self.assertEqual(mode, '0666')
                self.client.sendcmd('site chmod 444 ' + self.tempfile)
                mode = oct(stat.S_IMODE(os.stat(self.tempfile).st_mode))
                self.assertEqual(mode, '0444')
                self.client.sendcmd('site chmod 666 ' + self.tempfile)
                mode = oct(stat.S_IMODE(os.stat(self.tempfile).st_mode))
                self.assertEqual(mode, '0666')
            else:
                self.client.sendcmd('site chmod 777 ' + self.tempfile)
                mode = oct(stat.S_IMODE(os.stat(self.tempfile).st_mode))
                self.assertEqual(mode, '0777')
                self.client.sendcmd('site chmod 755 ' + self.tempfile)
                mode = oct(stat.S_IMODE(os.stat(self.tempfile).st_mode))
                self.assertEqual(mode, '0755')
                self.client.sendcmd('site chmod 555 ' + self.tempfile)
                mode = oct(stat.S_IMODE(os.stat(self.tempfile).st_mode))
                self.assertEqual(mode, '0555')


class TestFtpStoreData(unittest.TestCase):
    """Test STOR, STOU, APPE, REST, TYPE."""
    server_class = FTPd
    client_class = ftplib.FTP

    def setUp(self):
        self.server = self.server_class()
        self.server.start()
        self.client = self.client_class()
        self.client.connect(self.server.host, self.server.port)
        self.client.sock.settimeout(2)
        self.client.login(USER, PASSWD)
        self.dummy_recvfile = StringIO.StringIO()
        self.dummy_sendfile = StringIO.StringIO()

    def tearDown(self):
        self.client.close()
        self.server.stop()
        self.dummy_recvfile.close()
        self.dummy_sendfile.close()
        safe_remove(TESTFN)

    def test_stor(self):
        try:
            data = 'abcde12345' * 100000
            self.dummy_sendfile.write(data)
            self.dummy_sendfile.seek(0)
            self.client.storbinary('stor ' + TESTFN, self.dummy_sendfile)
            self.client.retrbinary('retr ' + TESTFN, self.dummy_recvfile.write)
            self.dummy_recvfile.seek(0)
            self.assertEqual(hash(data), hash (self.dummy_recvfile.read()))
        finally:
            # We do not use os.remove() because file could still be
            # locked by ftpd thread.  If DELE through FTP fails try
            # os.remove() as last resort.
            if os.path.exists(TESTFN):
                try:
                    self.client.delete(TESTFN)
                except (ftplib.Error, EOFError, socket.error):
                    safe_remove(TESTFN)

    def test_stor_active(self):
        # Like test_stor but using PORT
        self.client.set_pasv(False)
        self.test_stor()

    def test_stor_ascii(self):
        # Test STOR in ASCII mode

        def store(cmd, fp, blocksize=8192):
            # like storbinary() except it sends "type a" instead of
            # "type i" before starting the transfer
            self.client.voidcmd('type a')
            conn = self.client.transfercmd(cmd)
            while 1:
                buf = fp.read(blocksize)
                if not buf:
                    break
                conn.sendall(buf)
            conn.close()
            return self.client.voidresp()

        try:
            data = 'abcde12345\r\n' * 100000
            self.dummy_sendfile.write(data)
            self.dummy_sendfile.seek(0)
            store('stor ' + TESTFN, self.dummy_sendfile)
            self.client.retrbinary('retr ' + TESTFN, self.dummy_recvfile.write)
            expected = data.replace('\r\n', os.linesep)
            self.dummy_recvfile.seek(0)
            self.assertEqual(hash(expected), hash(self.dummy_recvfile.read()))
        finally:
            # We do not use os.remove() because file could still be
            # locked by ftpd thread.  If DELE through FTP fails try
            # os.remove() as last resort.
            if os.path.exists(TESTFN):
                try:
                    self.client.delete(TESTFN)
                except (ftplib.Error, EOFError, socket.error):
                    safe_remove(TESTFN)

    def test_stor_ascii_2(self):
        # Test that no extra extra carriage returns are added to the
        # file in ASCII mode in case CRLF gets truncated in two chunks
        # (issue 116)

        def store(cmd, fp, blocksize=8192):
            # like storbinary() except it sends "type a" instead of
            # "type i" before starting the transfer
            self.client.voidcmd('type a')
            conn = self.client.transfercmd(cmd)
            while 1:
                buf = fp.read(blocksize)
                if not buf:
                    break
                conn.sendall(buf)
            conn.close()
            return self.client.voidresp()

        old_buffer = ftpserver.DTPHandler.ac_in_buffer_size
        try:
            # set a small buffer so that CRLF gets delivered in two
            # separate chunks: "CRLF", " f", "oo", " CR", "LF", " b", "ar"
            ftpserver.DTPHandler.ac_in_buffer_size = 2
            data = '\r\n foo \r\n bar'
            self.dummy_sendfile.write(data)
            self.dummy_sendfile.seek(0)
            store('stor ' + TESTFN, self.dummy_sendfile)

            expected = data.replace('\r\n', os.linesep)
            self.client.retrbinary('retr ' + TESTFN, self.dummy_recvfile.write)
            self.dummy_recvfile.seek(0)
            self.assertEqual(expected, self.dummy_recvfile.read())
        finally:
            ftpserver.DTPHandler.ac_in_buffer_size = old_buffer
            # We do not use os.remove() because file could still be
            # locked by ftpd thread.  If DELE through FTP fails try
            # os.remove() as last resort.
            if os.path.exists(TESTFN):
                try:
                    self.client.delete(TESTFN)
                except (ftplib.Error, EOFError, socket.error):
                    safe_remove(TESTFN)

    def test_stou(self):
        data = 'abcde12345' * 100000
        self.dummy_sendfile.write(data)
        self.dummy_sendfile.seek(0)

        self.client.voidcmd('TYPE I')
        # filename comes in as "1xx FILE: <filename>"
        filename = self.client.sendcmd('stou').split('FILE: ')[1]
        try:
            sock = self.client.makeport()
            conn, sockaddr = sock.accept()
            if hasattr(self.client_class, 'ssl_version'):
                conn = ssl.wrap_socket(conn)
            while 1:
                buf = self.dummy_sendfile.read(8192)
                if not buf:
                    break
                conn.sendall(buf)
            sock.close()
            conn.close()
            # transfer finished, a 226 response is expected
            self.assertEqual('226', self.client.voidresp()[:3])
            self.client.retrbinary('retr ' + filename, self.dummy_recvfile.write)
            self.dummy_recvfile.seek(0)
            self.assertEqual(hash(data), hash (self.dummy_recvfile.read()))
        finally:
            # We do not use os.remove() because file could still be
            # locked by ftpd thread.  If DELE through FTP fails try
            # os.remove() as last resort.
            if os.path.exists(filename):
                try:
                    self.client.delete(filename)
                except (ftplib.Error, EOFError, socket.error):
                    safe_remove(filename)

    def test_stou_rest(self):
        # Watch for STOU preceded by REST, which makes no sense.
        self.client.sendcmd('type i')
        self.client.sendcmd('rest 10')
        self.assertRaises(ftplib.error_temp, self.client.sendcmd, 'stou')

    def test_stou_orphaned_file(self):
        # Check that no orphaned file gets left behind when STOU fails.
        # Even if STOU fails the file is first created and then erased.
        # Since we can't know the name of the file the best way that
        # we have to test this case is comparing the content of the
        # directory before and after STOU has been issued.
        # Assuming that TESTFN is supposed to be a "reserved" file
        # name we shouldn't get false positives.
        safe_remove(TESTFN)
        # login as a limited user to let STOU fail
        self.client.login('anonymous', '@nopasswd')
        before = os.listdir(HOME)
        self.assertRaises(ftplib.error_perm, self.client.sendcmd, 'stou ' + TESTFN)
        after = os.listdir(HOME)
        if before != after:
            for file in after:
                self.assert_(not file.startswith(TESTFN))

    def test_appe(self):
        try:
            data1 = 'abcde12345' * 100000
            self.dummy_sendfile.write(data1)
            self.dummy_sendfile.seek(0)
            self.client.storbinary('stor ' + TESTFN, self.dummy_sendfile)

            data2 = 'fghil67890' * 100000
            self.dummy_sendfile.write(data2)
            self.dummy_sendfile.seek(len(data1))
            self.client.storbinary('appe ' + TESTFN, self.dummy_sendfile)

            self.client.retrbinary("retr " + TESTFN, self.dummy_recvfile.write)
            self.dummy_recvfile.seek(0)
            self.assertEqual(hash(data1 + data2), hash (self.dummy_recvfile.read()))
        finally:
            # We do not use os.remove() because file could still be
            # locked by ftpd thread.  If DELE through FTP fails try
            # os.remove() as last resort.
            if os.path.exists(TESTFN):
                try:
                    self.client.delete(TESTFN)
                except (ftplib.Error, EOFError, socket.error):
                    safe_remove(TESTFN)

    def test_appe_rest(self):
        # Watch for APPE preceded by REST, which makes no sense.
        self.client.sendcmd('type i')
        self.client.sendcmd('rest 10')
        self.assertRaises(ftplib.error_temp, self.client.sendcmd, 'appe x')

    def test_rest_on_stor(self):
        # Test STOR preceded by REST.
        data = 'abcde12345' * 100000
        self.dummy_sendfile.write(data)
        self.dummy_sendfile.seek(0)

        self.client.voidcmd('TYPE I')
        conn = self.client.transfercmd('stor ' + TESTFN)
        bytes_sent = 0
        while 1:
            chunk = self.dummy_sendfile.read(8192)
            conn.sendall(chunk)
            bytes_sent += len(chunk)
            # stop transfer while it isn't finished yet
            if bytes_sent >= 524288 or not chunk:
                break

        conn.close()
        # transfer wasn't finished yet but server can't know this,
        # hence expect a 226 response
        self.assertEqual('226', self.client.voidresp()[:3])

        # resuming transfer by using a marker value greater than the
        # file size stored on the server should result in an error
        # on stor
        file_size = self.client.size(TESTFN)
        self.assertEqual(file_size, bytes_sent)
        self.client.sendcmd('rest %s' % ((file_size + 1)))
        self.assertRaises(ftplib.error_perm, self.client.sendcmd, 'stor ' + TESTFN)

        self.client.sendcmd('rest %s' % bytes_sent)
        self.client.storbinary('stor ' + TESTFN, self.dummy_sendfile)

        self.client.retrbinary('retr ' + TESTFN, self.dummy_recvfile.write)
        self.dummy_sendfile.seek(0)
        self.dummy_recvfile.seek(0)
        self.assertEqual(hash(self.dummy_sendfile.read()),
                         hash(self.dummy_recvfile.read())
                         )
        self.client.delete(TESTFN)

    def test_failing_rest_on_stor(self):
        # Test REST -> STOR against a non existing file.
        if os.path.exists(TESTFN):
            self.client.delete(TESTFN)
        self.client.sendcmd('type i')
        self.client.sendcmd('rest 10')
        self.assertRaises(ftplib.error_perm, self.client.storbinary,
                          'stor ' + TESTFN, lambda x: x)
        # if the first STOR failed because of REST, the REST marker
        # is supposed to be resetted to 0
        self.dummy_sendfile.write('x' * 4096)
        self.dummy_sendfile.seek(0)
        self.client.storbinary('stor ' + TESTFN, self.dummy_sendfile)

    def test_quit_during_transfer(self):
        # RFC-959 states that if QUIT is sent while a transfer is in
        # progress, the connection must remain open for result response
        # and the server will then close it.
        conn = self.client.transfercmd('stor ' + TESTFN)
        conn.sendall('abcde12345' * 50000)
        self.client.sendcmd('quit')
        conn.sendall('abcde12345' * 50000)
        conn.close()
        # expect the response (transfer ok)
        self.assertEqual('226', self.client.voidresp()[:3])
        # Make sure client has been disconnected.
        # socket.error (Windows) or EOFError (Linux) exception is supposed
        # to be raised in such a case.
        self.assertRaises((socket.error, EOFError), self.client.sendcmd, 'noop')

    def test_stor_empty_file(self):
        self.client.storbinary('stor ' + TESTFN, self.dummy_sendfile)
        self.client.quit()
        f = open(TESTFN)
        self.assertEqual(f.read(), "")
        f.close()


if SUPPORTS_SENDFILE:
    class TestFtpStoreDataNoSendfile(TestFtpStoreData):
        """Test STOR, STOU, APPE, REST, TYPE not using sendfile()."""

        def setUp(self):
            TestFtpStoreData.setUp(self)
            self.server.handler.use_sendfile = False

        def tearDown(self):
            TestFtpStoreData.tearDown(self)
            self.server.handler.use_sendfile = True


class TestFtpRetrieveData(unittest.TestCase):
    "Test RETR, REST, TYPE"
    server_class = FTPd
    client_class = ftplib.FTP

    def setUp(self):
        self.server = self.server_class()
        self.server.start()
        self.client = self.client_class()
        self.client.connect(self.server.host, self.server.port)
        self.client.sock.settimeout(2)
        self.client.login(USER, PASSWD)
        self.file = open(TESTFN, 'w+b')
        self.dummyfile = StringIO.StringIO()

    def tearDown(self):
        self.client.close()
        self.server.stop()
        if not self.file.closed:
            self.file.close()
        if not self.dummyfile.closed:
            self.dummyfile.close()
        safe_remove(TESTFN)

    def test_retr(self):
        data = 'abcde12345' * 100000
        self.file.write(data)
        self.file.close()
        self.client.retrbinary("retr " + TESTFN, self.dummyfile.write)
        self.dummyfile.seek(0)
        self.assertEqual(hash(data), hash(self.dummyfile.read()))

        # attempt to retrieve a file which doesn't exist
        bogus = os.path.basename(tempfile.mktemp(dir=HOME))
        self.assertRaises(ftplib.error_perm, self.client.retrbinary,
                                             "retr " + bogus, lambda x: x)

    def test_retr_ascii(self):
        # Test RETR in ASCII mode.

        def retrieve(cmd, callback, blocksize=8192, rest=None):
            # like retrbinary but uses TYPE A instead
            self.client.voidcmd('type a')
            conn = self.client.transfercmd(cmd, rest)
            while 1:
                data = conn.recv(blocksize)
                if not data:
                    break
                callback(data)
            conn.close()
            return self.client.voidresp()

        data = ('abcde12345' + os.linesep) * 100000
        self.file.write(data)
        self.file.close()
        retrieve("retr " + TESTFN, self.dummyfile.write)
        expected = data.replace(os.linesep, '\r\n')
        self.dummyfile.seek(0)
        self.assertEqual(hash(expected), hash(self.dummyfile.read()))

    def test_restore_on_retr(self):
        data = 'abcde12345' * 1000000
        self.file.write(data)
        self.file.close()

        received_bytes = 0
        self.client.voidcmd('TYPE I')
        conn = self.client.transfercmd('retr ' + TESTFN)
        while 1:
            chunk = conn.recv(8192)
            if not chunk:
                break
            self.dummyfile.write(chunk)
            received_bytes += len(chunk)
            if received_bytes >= len(data) // 2:
                break
        conn.close()

        # transfer wasn't finished yet so we expect a 426 response
        self.assertEqual(self.client.getline()[:3], "426")

        # resuming transfer by using a marker value greater than the
        # file size stored on the server should result in an error
        # on retr (RFC-1123)
        file_size = self.client.size(TESTFN)
        self.client.sendcmd('rest %s' % ((file_size + 1)))
        self.assertRaises(ftplib.error_perm, self.client.sendcmd, 'retr ' + TESTFN)

        # test resume
        self.client.sendcmd('rest %s' % received_bytes)
        self.client.retrbinary("retr " + TESTFN, self.dummyfile.write)
        self.dummyfile.seek(0)
        self.assertEqual(hash(data), hash (self.dummyfile.read()))

    def test_retr_empty_file(self):
        self.client.retrbinary("retr " + TESTFN, self.dummyfile.write)
        self.dummyfile.seek(0)
        self.assertEqual(self.dummyfile.read(), "")


if SUPPORTS_SENDFILE:
    class TestFtpRetrieveDataNoSendfile(TestFtpRetrieveData):
        """Test RETR, REST, TYPE by not using sendfile()."""

        def setUp(self):
            TestFtpRetrieveData.setUp(self)
            self.server.handler.use_sendfile = False

        def tearDown(self):
            TestFtpRetrieveData.tearDown(self)
            self.server.handler.use_sendfile = True


class TestFtpListingCmds(unittest.TestCase):
    """Test LIST, NLST, argumented STAT."""
    server_class = FTPd
    client_class = ftplib.FTP

    def setUp(self):
        self.server = self.server_class()
        self.server.start()
        self.client = self.client_class()
        self.client.connect(self.server.host, self.server.port)
        self.client.sock.settimeout(2)
        self.client.login(USER, PASSWD)
        touch(TESTFN)

    def tearDown(self):
        self.client.close()
        self.server.stop()
        os.remove(TESTFN)

    def _test_listing_cmds(self, cmd):
        """Tests common to LIST NLST and MLSD commands."""
        # assume that no argument has the same meaning of "/"
        l1 = l2 = []
        self.client.retrlines(cmd, l1.append)
        self.client.retrlines(cmd + ' /', l2.append)
        self.assertEqual(l1, l2)
        if cmd.lower() != 'mlsd':
            # if pathname is a file one line is expected
            x = []
            self.client.retrlines('%s ' % cmd + TESTFN, x.append)
            self.assertEqual(len(x), 1)
            self.assertTrue(''.join(x).endswith(TESTFN))
        # non-existent path, 550 response is expected
        bogus = os.path.basename(tempfile.mktemp(dir=HOME))
        self.assertRaises(ftplib.error_perm, self.client.retrlines,
                          '%s ' %cmd + bogus, lambda x: x)
        # for an empty directory we excpect that the data channel is
        # opened anyway and that no data is received
        x = []
        tempdir = os.path.basename(tempfile.mkdtemp(dir=HOME))
        try:
            self.client.retrlines('%s %s' % (cmd, tempdir), x.append)
            self.assertEqual(x, [])
        finally:
            try:
                os.rmdir(tempdir)
            except OSError:
                pass

    def test_nlst(self):
        # common tests
        self._test_listing_cmds('nlst')

    def test_list(self):
        # common tests
        self._test_listing_cmds('list')
        # known incorrect pathname arguments (e.g. old clients) are
        # expected to be treated as if pathname would be == '/'
        l1 = l2 = l3 = l4 = l5 = []
        self.client.retrlines('list /', l1.append)
        self.client.retrlines('list -a', l2.append)
        self.client.retrlines('list -l', l3.append)
        self.client.retrlines('list -al', l4.append)
        self.client.retrlines('list -la', l5.append)
        tot = (l1, l2, l3, l4, l5)
        for x in range(len(tot) - 1):
            self.assertEqual(tot[x], tot[x+1])

    def test_mlst(self):
        # utility function for extracting the line of interest
        mlstline = lambda cmd: self.client.voidcmd(cmd).split('\n')[1]

        # the fact set must be preceded by a space
        self.assertTrue(mlstline('mlst').startswith(' '))
        # where TVFS is supported, a fully qualified pathname is expected
        self.assertTrue(mlstline('mlst ' + TESTFN).endswith('/' + TESTFN))
        self.assertTrue(mlstline('mlst').endswith('/'))
        # assume that no argument has the same meaning of "/"
        self.assertEqual(mlstline('mlst'), mlstline('mlst /'))
        # non-existent path
        bogus = os.path.basename(tempfile.mktemp(dir=HOME))
        self.assertRaises(ftplib.error_perm, self.client.sendcmd, 'mlst '+bogus)
        # test file/dir notations
        self.assertTrue('type=dir' in mlstline('mlst'))
        self.assertTrue('type=file' in mlstline('mlst ' + TESTFN))
        # let's add some tests for OPTS command
        self.client.sendcmd('opts mlst type;')
        self.assertEqual(mlstline('mlst'), ' type=dir; /')
        # where no facts are present, two leading spaces before the
        # pathname are required (RFC-3659)
        self.client.sendcmd('opts mlst')
        self.assertEqual(mlstline('mlst'), '  /')

    def test_mlsd(self):
        # common tests
        self._test_listing_cmds('mlsd')
        dir = os.path.basename(tempfile.mkdtemp(dir=HOME))
        try:
            try:
                self.client.retrlines('mlsd ' + TESTFN, lambda x: x)
            except ftplib.error_perm, resp:
                # if path is a file a 501 response code is expected
                self.assertEqual(str(resp)[0:3], "501")
            else:
                self.fail("Exception not raised")
        finally:
            safe_rmdir(dir)

    def test_mlsd_all_facts(self):
        feat = self.client.sendcmd('feat')
        # all the facts
        facts = re.search(r'^\s*MLST\s+(\S+)$', feat, re.MULTILINE).group(1)
        facts = facts.replace("*;", ";")
        self.client.sendcmd('opts mlst ' + facts)
        resp = self.client.sendcmd('mlst')

        local = facts[:-1].split(";")
        returned = resp.split("\n")[1].strip()[:-3]
        returned = [x.split("=")[0] for x in returned.split(";")]
        self.assertEqual(sorted(local), sorted(returned))

        self.assertTrue("type" in resp)
        self.assertTrue("size" in resp)
        self.assertTrue("perm" in resp)
        self.assertTrue("modify" in resp)
        if os.name == 'posix':
            self.assertTrue("unique" in resp)
            self.assertTrue("unix.mode" in resp)
            self.assertTrue("unix.uid" in resp)
            self.assertTrue("unix.gid" in resp)
        elif os.name == 'nt':
            self.assertTrue("create" in resp)

    def test_stat(self):
        # Test STAT provided with argument which is equal to LIST
        self.client.sendcmd('stat /')
        self.client.sendcmd('stat ' + TESTFN)
        self.client.putcmd('stat *')
        resp = self.client.getmultiline()
        self.assertEqual(resp, '550 Globbing not supported.')
        bogus = os.path.basename(tempfile.mktemp(dir=HOME))
        self.assertRaises(ftplib.error_perm, self.client.sendcmd, 'stat ' + bogus)

    def test_unforeseen_time_event(self):
        # Emulate a case where the file last modification time is prior
        # to year 1900.  This most likely will never happen unless
        # someone specifically force the last modification time of a
        # file in some way.
        # To do so we temporarily override os.path.getmtime so that it
        # returns a negative value referring to a year prior to 1900.
        # It causes time.localtime/gmtime to raise a ValueError exception
        # which is supposed to be handled by server.
        _getmtime = ftpserver.AbstractedFS.getmtime
        try:
            ftpserver.AbstractedFS.getmtime = lambda x, y: -9000000000
            self.client.sendcmd('stat /')  # test AbstractedFS.format_list()
            self.client.sendcmd('mlst /')  # test AbstractedFS.format_mlsx()
            # make sure client hasn't been disconnected
            self.client.sendcmd('noop')
        finally:
            ftpserver.AbstractedFS.getmtime = _getmtime


class TestFtpAbort(unittest.TestCase):
    "test: ABOR"
    server_class = FTPd
    client_class = ftplib.FTP

    def setUp(self):
        self.server = self.server_class()
        self.server.start()
        self.client = self.client_class()
        self.client.connect(self.server.host, self.server.port)
        self.client.sock.settimeout(2)
        self.client.login(USER, PASSWD)

    def tearDown(self):
        self.client.close()
        self.server.stop()

    def test_abor_no_data(self):
        # Case 1: ABOR while no data channel is opened: respond with 225.
        resp = self.client.sendcmd('ABOR')
        self.assertEqual('225 No transfer to abort.', resp)
        self.client.retrlines('list', [].append)

    def test_abor_pasv(self):
        # Case 2: user sends a PASV, a data-channel socket is listening
        # but not connected, and ABOR is sent: close listening data
        # socket, respond with 225.
        self.client.makepasv()
        respcode = self.client.sendcmd('ABOR')[:3]
        self.assertEqual('225', respcode)
        self.client.retrlines('list', [].append)

    def test_abor_port(self):
        # Case 3: data channel opened with PASV or PORT, but ABOR sent
        # before a data transfer has been started: close data channel,
        # respond with 225
        self.client.set_pasv(0)
        sock = self.client.makeport()
        respcode = self.client.sendcmd('ABOR')[:3]
        sock.close()
        self.assertEqual('225', respcode)
        self.client.retrlines('list', [].append)

    def test_abor_during_transfer(self):
        # Case 4: ABOR while a data transfer on DTP channel is in
        # progress: close data channel, respond with 426, respond
        # with 226.
        data = 'abcde12345' * 1000000
        f = open(TESTFN, 'w+b')
        f.write(data)
        f.close()
        try:
            self.client.voidcmd('TYPE I')
            conn = self.client.transfercmd('retr ' + TESTFN)
            bytes_recv = 0
            while bytes_recv < 65536:
                chunk = conn.recv(8192)
                bytes_recv += len(chunk)

            # stop transfer while it isn't finished yet
            self.client.putcmd('ABOR')

            # transfer isn't finished yet so ftpd should respond with 426
            self.assertEqual(self.client.getline()[:3], "426")

            # transfer successfully aborted, so should now respond with a 226
            self.assertEqual('226', self.client.voidresp()[:3])
        finally:
            # We do not use os.remove() because file could still be
            # locked by ftpd thread.  If DELE through FTP fails try
            # os.remove() as last resort.
            try:
                self.client.delete(TESTFN)
            except (ftplib.Error, EOFError, socket.error):
                safe_remove(TESTFN)

    if hasattr(socket, 'MSG_OOB'):
        def test_oob_abor(self):
            # Send ABOR by following the RFC-959 directives of sending
            # Telnet IP/Synch sequence as OOB data.
            # On some systems like FreeBSD this happened to be a problem
            # due to a different SO_OOBINLINE behavior.
            # On some platforms (e.g. Python CE) the test may fail
            # although the MSG_OOB constant is defined.
            self.client.sock.sendall(chr(244), socket.MSG_OOB)
            self.client.sock.sendall(chr(255), socket.MSG_OOB)
            self.client.sock.sendall('abor\r\n')
            self.client.sock.settimeout(1)
            self.assertEqual(self.client.getresp()[:3], '225')


class TestTimeouts(unittest.TestCase):
    """Test idle-timeout capabilities of control and data channels.
    Some tests may fail on slow machines.
    """
    server_class = FTPd
    client_class = ftplib.FTP

    def setUp(self):
        self.server = None
        self.client = None

    def _setUp(self, idle_timeout=300, data_timeout=300, pasv_timeout=30,
               port_timeout=30):
        self.server = self.server_class()
        self.server.handler.timeout = idle_timeout
        self.server.handler.dtp_handler.timeout = data_timeout
        self.server.handler.passive_dtp.timeout = pasv_timeout
        self.server.handler.active_dtp.timeout = port_timeout
        self.server.start()
        self.client = self.client_class()
        self.client.connect(self.server.host, self.server.port)
        self.client.sock.settimeout(2)
        self.client.login(USER, PASSWD)

    def tearDown(self):
        if self.client is not None and self.server is not None:
            self.client.close()
            self.server.handler.timeout = 300
            self.server.handler.dtp_handler.timeout = 300
            self.server.handler.passive_dtp.timeout = 30
            self.server.handler.active_dtp.timeout = 30
            self.server.stop()

    def test_idle_timeout(self):
        # Test control channel timeout.  The client which does not send
        # any command within the time specified in FTPHandler.timeout is
        # supposed to be kicked off.
        self._setUp(idle_timeout=0.1)
        # fail if no msg is received within 1 second
        self.client.sock.settimeout(1)
        data = self.client.sock.recv(1024)
        self.assertEqual(data, "421 Control connection timed out.\r\n")
        # ensure client has been kicked off
        self.assertRaises((socket.error, EOFError), self.client.sendcmd, 'noop')

    def test_data_timeout(self):
        # Test data channel timeout.  The client which does not send
        # or receive any data within the time specified in
        # DTPHandler.timeout is supposed to be kicked off.
        self._setUp(data_timeout=0.1)
        addr = self.client.makepasv()
        s = socket.socket()
        s.connect(addr)
        # fail if no msg is received within 1 second
        self.client.sock.settimeout(1)
        data = self.client.sock.recv(1024)
        self.assertEqual(data, "421 Data connection timed out.\r\n")
        # ensure client has been kicked off
        self.assertRaises((socket.error, EOFError), self.client.sendcmd, 'noop')

    def test_data_timeout_not_reached(self):
        # Impose a timeout for the data channel, then keep sending data for a
        # time which is longer than that to make sure that the code checking
        # whether the transfer stalled for with no progress is executed.
        self._setUp(data_timeout=0.1)
        sock = self.client.transfercmd('stor ' + TESTFN)
        if hasattr(self.client_class, 'ssl_version'):
            sock = ssl.wrap_socket(sock)
        try:
            stop_at = time.time() + 0.2
            while time.time() < stop_at:
                sock.send('x' * 1024)
            sock.close()
            self.client.voidresp()
        finally:
            if os.path.exists(TESTFN):
                self.client.delete(TESTFN)

    def test_idle_data_timeout1(self):
        # Tests that the control connection timeout is suspended while
        # the data channel is opened
        self._setUp(idle_timeout=0.1, data_timeout=0.2)
        addr = self.client.makepasv()
        s = socket.socket()
        s.connect(addr)
        # fail if no msg is received within 1 second
        self.client.sock.settimeout(1)
        data = self.client.sock.recv(1024)
        self.assertEqual(data, "421 Data connection timed out.\r\n")
        # ensure client has been kicked off
        self.assertRaises((socket.error, EOFError), self.client.sendcmd, 'noop')

    def test_idle_data_timeout2(self):
        # Tests that the control connection timeout is restarted after
        # data channel has been closed
        self._setUp(idle_timeout=0.1, data_timeout=0.2)
        addr = self.client.makepasv()
        s = socket.socket()
        s.connect(addr)
        # close data channel
        self.client.sendcmd('abor')
        self.client.sock.settimeout(1)
        data = self.client.sock.recv(1024)
        self.assertEqual(data, "421 Control connection timed out.\r\n")
        # ensure client has been kicked off
        self.assertRaises((socket.error, EOFError), self.client.sendcmd, 'noop')

    def test_pasv_timeout(self):
        # Test pasv data channel timeout.  The client which does not
        # connect to the listening data socket within the time specified
        # in PassiveDTP.timeout is supposed to receive a 421 response.
        self._setUp(pasv_timeout=0.1)
        self.client.makepasv()
        # fail if no msg is received within 1 second
        self.client.sock.settimeout(1)
        data = self.client.sock.recv(1024)
        self.assertEqual(data, "421 Passive data channel timed out.\r\n")
        # client is not expected to be kicked off
        self.client.sendcmd('noop')

    def test_disabled_idle_timeout(self):
        self._setUp(idle_timeout=0)
        self.client.sendcmd('noop')

    def test_disabled_data_timeout(self):
        self._setUp(data_timeout=0)
        addr = self.client.makepasv()
        s = socket.socket()
        s.connect(addr)
        s.close()

    def test_disabled_pasv_timeout(self):
        self._setUp(pasv_timeout=0)
        self.client.makepasv()
        # reset passive socket
        addr = self.client.makepasv()
        s = socket.socket()
        s.connect(addr)
        s.close()

    def test_disabled_port_timeout(self):
        self._setUp(port_timeout=0)
        s1 =self.client.makeport()
        s2 = self.client.makeport()
        s1.close()
        s2.close()


class TestConfigurableOptions(unittest.TestCase):
    """Test those daemon options which are commonly modified by user."""
    server_class = FTPd
    client_class = ftplib.FTP

    def setUp(self):
        touch(TESTFN)
        self.server = self.server_class()
        self.server.start()
        self.client = self.client_class()
        self.client.connect(self.server.host, self.server.port)
        self.client.sock.settimeout(2)
        self.client.login(USER, PASSWD)

    def tearDown(self):
        os.remove(TESTFN)
        # set back options to their original value
        self.server.server.max_cons = 0
        self.server.server.max_cons_per_ip = 0
        self.server.handler.banner = "pyftpdlib %s ready." % ftpserver.__ver__
        self.server.handler.max_login_attempts = 3
        self.server.handler._auth_failed_timeout = 5
        self.server.handler.masquerade_address = None
        self.server.handler.masquerade_address_map = {}
        self.server.handler.permit_privileged_ports = False
        self.server.handler.passive_ports = None
        self.server.handler.use_gmt_times = True
        self.server.handler.tcp_no_delay = hasattr(socket, 'TCP_NODELAY')
        self.server.stop()

    def test_max_connections(self):
        # Test FTPServer.max_cons attribute
        self.server.server.max_cons = 3
        self.client.quit()
        c1 = self.client_class()
        c2 = self.client_class()
        c3 = self.client_class()
        try:
            c1.connect(self.server.host, self.server.port)
            c2.connect(self.server.host, self.server.port)
            self.assertRaises(ftplib.error_temp, c3.connect, self.server.host,
                              self.server.port)
            # with passive data channel established
            c2.quit()
            c1.login(USER, PASSWD)
            c1.makepasv()
            self.assertRaises(ftplib.error_temp, c2.connect, self.server.host,
                              self.server.port)
            # with passive data socket waiting for connection
            c1.login(USER, PASSWD)
            c1.sendcmd('pasv')
            self.assertRaises(ftplib.error_temp, c2.connect, self.server.host,
                              self.server.port)
            # with active data channel established
            c1.login(USER, PASSWD)
            sock = c1.makeport()
            self.assertRaises(ftplib.error_temp, c2.connect, self.server.host,
                              self.server.port)
            sock.close()
        finally:
            c1.close()
            c2.close()
            c3.close()

    def test_max_connections_per_ip(self):
        # Test FTPServer.max_cons_per_ip attribute
        self.server.server.max_cons_per_ip = 3
        self.client.quit()
        c1 = self.client_class()
        c2 = self.client_class()
        c3 = self.client_class()
        c4 = self.client_class()
        try:
            c1.connect(self.server.host, self.server.port)
            c2.connect(self.server.host, self.server.port)
            c3.connect(self.server.host, self.server.port)
            self.assertRaises(ftplib.error_temp, c4.connect, self.server.host,
                              self.server.port)
            # Make sure client has been disconnected.
            # socket.error (Windows) or EOFError (Linux) exception is
            # supposed to be raised in such a case.
            self.assertRaises((socket.error, EOFError), c4.sendcmd, 'noop')
        finally:
            c1.close()
            c2.close()
            c3.close()
            c4.close()

    def test_banner(self):
        # Test FTPHandler.banner attribute
        self.server.handler.banner = 'hello there'
        self.client.close()
        self.client = self.client_class()
        self.client.connect(self.server.host, self.server.port)
        self.client.sock.settimeout(2)
        self.assertEqual(self.client.getwelcome()[4:], 'hello there')

    def test_max_login_attempts(self):
        # Test FTPHandler.max_login_attempts attribute.
        self.server.handler.max_login_attempts = 1
        self.server.handler._auth_failed_timeout = 0
        self.assertRaises(ftplib.error_perm, self.client.login, 'wrong', 'wrong')
        # socket.error (Windows) or EOFError (Linux) exceptions are
        # supposed to be raised when attempting to send/recv some data
        # using a disconnected socket
        self.assertRaises((socket.error, EOFError), self.client.sendcmd, 'noop')

    def test_masquerade_address(self):
        # Test FTPHandler.masquerade_address attribute
        host, port = self.client.makepasv()
        self.assertEqual(host, self.server.host)
        self.server.handler.masquerade_address = "256.256.256.256"
        host, port = self.client.makepasv()
        self.assertEqual(host, "256.256.256.256")

    def test_masquerade_address_map(self):
        # Test FTPHandler.masquerade_address_map attribute
        host, port = self.client.makepasv()
        self.assertEqual(host, self.server.host)
        self.server.handler.masquerade_address_map = {self.server.host :
                                                      "128.128.128.128"}
        host, port = self.client.makepasv()
        self.assertEqual(host, "128.128.128.128")

    def test_passive_ports(self):
        # Test FTPHandler.passive_ports attribute
        _range = range(40000, 60000, 200)
        self.server.handler.passive_ports = _range
        self.assert_(self.client.makepasv()[1] in _range)
        self.assert_(self.client.makepasv()[1] in _range)
        self.assert_(self.client.makepasv()[1] in _range)
        self.assert_(self.client.makepasv()[1] in _range)

    def test_passive_ports_busy(self):
        # If the ports in the configured range are busy it is expected
        # that a kernel-assigned port gets chosen
        s = socket.socket()
        s.bind((HOST, 0))
        port = s.getsockname()[1]
        self.server.handler.passive_ports = [port]
        resulting_port = self.client.makepasv()[1]
        self.assert_(port != resulting_port)

    def test_permit_privileged_ports(self):
        # Test FTPHandler.permit_privileged_ports_active attribute

        # try to bind a socket on a privileged port
        sock = None
        for port in reversed(range(1, 1024)):
            try:
                socket.getservbyport(port)
            except socket.error, err:
                # not registered port; go on
                try:
                    sock = socket.socket(self.client.af, socket.SOCK_STREAM)
                    sock.bind((HOST, port))
                    break
                except socket.error, err:
                    if err.args[0] == errno.EACCES:
                        # root privileges needed
                        sock = None
                        break
                    sock.close()
                    continue
            else:
                # registered port found; skip to the next one
                continue
        else:
            # no usable privileged port was found
            sock = None

        try:
            self.server.handler.permit_privileged_ports = False
            self.assertRaises(ftplib.error_perm, self.client.sendport, HOST,
                              port)
            if sock:
                port = sock.getsockname()[1]
                self.server.handler.permit_privileged_ports = True
                sock.listen(5)
                sock.settimeout(2)
                self.client.sendport(HOST, port)
                sock.accept()
        finally:
            if sock is not None:
                sock.close()

    def test_use_gmt_times(self):
        # use GMT time
        self.server.handler.use_gmt_times = True
        gmt1 = self.client.sendcmd('mdtm ' + TESTFN)
        gmt2 = self.client.sendcmd('mlst ' + TESTFN)
        gmt3 = self.client.sendcmd('stat ' + TESTFN)

        # use local time
        self.server.handler.use_gmt_times = False

        self.client.quit()
        self.client.connect(self.server.host, self.server.port)
        self.client.sock.settimeout(2)
        self.client.login(USER, PASSWD)

        loc1 = self.client.sendcmd('mdtm ' + TESTFN)
        loc2 = self.client.sendcmd('mlst ' + TESTFN)
        loc3 = self.client.sendcmd('stat ' + TESTFN)

        # if we're not in a GMT time zone times are supposed to be
        # different
        if time.timezone != 0:
            self.assertNotEqual(gmt1, loc1)
            self.assertNotEqual(gmt2, loc2)
            self.assertNotEqual(gmt3, loc3)
        # ...otherwise they should be the same
        else:
            self.assertEqual(gmt1, loc1)
            self.assertEqual(gmt2, loc2)
            self.assertEqual(gmt3, loc3)

    if hasattr(socket, 'TCP_NODELAY'):
        def test_tcp_no_delay(self):
            def get_handler_socket():
                # return the server's handler socket object
                for fd in asyncore.socket_map:
                    instance = asyncore.socket_map[fd]
                    if isinstance(instance, ftpserver.FTPHandler):
                        break
                return instance.socket

            s = get_handler_socket()
            self.assertTrue(s.getsockopt(socket.SOL_TCP, socket.TCP_NODELAY))
            self.client.quit()
            self.server.handler.tcp_no_delay = False
            self.client.connect(self.server.host, self.server.port)
            self.client.sendcmd('noop')
            s = get_handler_socket()
            self.assertFalse(s.getsockopt(socket.SOL_TCP, socket.TCP_NODELAY))


class TestCallbacks(unittest.TestCase):
    """Test FTPHandler class callback methods."""
    server_class = FTPd
    client_class = ftplib.FTP

    def setUp(self):
        self.client = None
        self.server = None
        self._tearDown = True

    def _setUp(self, handler, login=True):
        FTPd.handler = handler
        self.server = self.server_class()
        self.server.start()
        self.client = self.client_class()
        self.client.connect(self.server.host, self.server.port)
        self.client.sock.settimeout(2)
        if login:
            self.client.login(USER, PASSWD)
        self.file = open(TESTFN, 'w+b')
        self.dummyfile = StringIO.StringIO()
        self._tearDown = False

    def tearDown(self):
        if not self._tearDown:
            FTPd.handler = ftpserver.FTPHandler
            self._tearDown = True
            if self.client is not None:
                self.client.close()
            if self.server is not None:
                self.server.stop()
            if not self.file.closed:
                self.file.close()
            if not self.dummyfile.closed:
                self.dummyfile.close()
            os.remove(TESTFN)

    def test_on_file_sent(self):
        _file = []

        class TestHandler(ftpserver.FTPHandler):

            def on_file_sent(self, file):
                _file.append(file)

            def on_file_received(self, file):
                raise Exception

            def on_incomplete_file_sent(self, file):
                raise Exception

            def on_incomplete_file_received(self, file):
                raise Exception

        self._setUp(TestHandler)
        data = 'abcde12345' * 100000
        self.file.write(data)
        self.file.close()
        self.client.retrbinary("retr " + TESTFN, lambda x: x)
        # shut down the server to avoid race conditions
        self.tearDown()
        self.assertEqual(_file, [os.path.abspath(TESTFN)])

    def test_on_file_received(self):
        _file = []

        class TestHandler(ftpserver.FTPHandler):

            def on_file_sent(self, file):
                raise Exception

            def on_file_received(self, file):
                _file.append(file)

            def on_incomplete_file_sent(self, file):
                raise Exception

            def on_incomplete_file_received(self, file):
                raise Exception

        self._setUp(TestHandler)
        data = 'abcde12345' * 100000
        self.dummyfile.write(data)
        self.dummyfile.seek(0)
        self.client.storbinary('stor ' + TESTFN, self.dummyfile)
        # shut down the server to avoid race conditions
        self.tearDown()
        self.assertEqual(_file, [os.path.abspath(TESTFN)])

    def test_on_incomplete_file_sent(self):
        _file = []

        class TestHandler(ftpserver.FTPHandler):

            def on_file_sent(self, file):
                raise Exception

            def on_file_received(self, file):
                raise Exception

            def on_incomplete_file_sent(self, file):
                _file.append(file)

            def on_incomplete_file_received(self, file):
                raise Exception

        self._setUp(TestHandler)
        data = 'abcde12345' * 100000
        self.file.write(data)
        self.file.close()

        bytes_recv = 0
        conn = self.client.transfercmd("retr " + TESTFN, None)
        while 1:
            chunk = conn.recv(8192)
            bytes_recv += len(chunk)
            if bytes_recv >= 524288 or not chunk:
                break
        conn.close()
        self.assertEqual(self.client.getline()[:3], "426")

        # shut down the server to avoid race conditions
        self.tearDown()
        self.assertEqual(_file, [os.path.abspath(TESTFN)])

    def test_on_incomplete_file_received(self):
        _file = []

        class TestHandler(ftpserver.FTPHandler):

            def on_file_sent(self, file):
                raise Exception

            def on_file_received(self, file):
                raise Exception

            def on_incomplete_file_sent(self, file):
                raise Exception

            def on_incomplete_file_received(self, file):
                _file.append(file)

        self._setUp(TestHandler)
        data = 'abcde12345' * 100000
        self.dummyfile.write(data)
        self.dummyfile.seek(0)

        conn = self.client.transfercmd('stor ' + TESTFN)
        bytes_sent = 0
        while 1:
            chunk = self.dummyfile.read(8192)
            conn.sendall(chunk)
            bytes_sent += len(chunk)
            # stop transfer while it isn't finished yet
            if bytes_sent >= 524288 or not chunk:
                self.client.putcmd('abor')
                break
        conn.close()

        # shut down the server to avoid race conditions
        self.tearDown()
        self.assertEqual(_file, [os.path.abspath(TESTFN)])

    def test_on_login(self):
        user = []

        class TestHandler(ftpserver.FTPHandler):
            _auth_failed_timeout = 0

            def on_login(self, username):
                user.append(username)

            def on_login_failed(self, username, password):
                raise Exception


        self._setUp(TestHandler)
        # shut down the server to avoid race conditions
        self.tearDown()
        self.assertEqual(user, [USER])

    def test_on_login_failed(self):
        pair = []

        class TestHandler(ftpserver.FTPHandler):
            _auth_failed_timeout = 0

            def on_login(self, username):
                raise Exception

            def on_login_failed(self, username, password):
                pair.append((username, password))

        self._setUp(TestHandler, login=False)
        self.assertRaises(ftplib.error_perm, self.client.login, 'foo', 'bar')
        # shut down the server to avoid race conditions
        self.tearDown()
        self.assertEqual(pair, [('foo', 'bar')])

    def test_on_login_failed(self):
        pair = []

        class TestHandler(ftpserver.FTPHandler):
            _auth_failed_timeout = 0

            def on_login(self, username):
                raise Exception

            def on_login_failed(self, username, password):
                pair.append((username, password))

        self._setUp(TestHandler, login=False)
        self.assertRaises(ftplib.error_perm, self.client.login, 'foo', 'bar')
        # shut down the server to avoid race conditions
        self.tearDown()
        self.assertEqual(pair, [('foo', 'bar')])


    def test_on_logout_quit(self):
        user = []

        class TestHandler(ftpserver.FTPHandler):

            def on_logout(self, username):
                user.append(username)

        self._setUp(TestHandler)
        self.client.quit()
        # shut down the server to avoid race conditions
        self.tearDown()
        self.assertEqual(user, [USER])

    def test_on_logout_rein(self):
        user = []

        class TestHandler(ftpserver.FTPHandler):

            def on_logout(self, username):
                user.append(username)

        self._setUp(TestHandler)
        self.client.sendcmd('rein')
        # shut down the server to avoid race conditions
        self.tearDown()
        self.assertEqual(user, [USER])

    def test_on_logout_user_issued_twice(self):
        users = []

        class TestHandler(ftpserver.FTPHandler):

            def on_logout(self, username):
                users.append(username)

        self._setUp(TestHandler)
        # At this point user "user" is logged in. Re-login as anonymous,
        # then quit and expect queue == ["user", "anonymous"]
        self.client.login("anonymous")
        self.client.quit()
        # shut down the server to avoid race conditions
        self.tearDown()
        self.assertEqual(users, [USER, 'anonymous'])


class _TestNetworkProtocols(unittest.TestCase):
    """Test PASV, EPSV, PORT and EPRT commands.

    Do not use this class directly, let TestIPv4Environment and
    TestIPv6Environment classes use it instead.
    """
    server_class = FTPd
    client_class = ftplib.FTP
    HOST = HOST

    def setUp(self):
        self.server = self.server_class(self.HOST)
        self.server.start()
        self.client = self.client_class()
        self.client.connect(self.server.host, self.server.port)
        self.client.sock.settimeout(2)
        self.client.login(USER, PASSWD)
        if self.client.af == socket.AF_INET:
            self.proto = "1"
            self.other_proto = "2"
        else:
            self.proto = "2"
            self.other_proto = "1"

    def tearDown(self):
        self.client.close()
        self.server.stop()

    def cmdresp(self, cmd):
        """Send a command and return response, also if the command failed."""
        try:
            return self.client.sendcmd(cmd)
        except ftplib.Error, err:
            return str(err)

    def test_eprt(self):
        # test wrong proto
        try:
            self.client.sendcmd('eprt |%s|%s|%s|' % (self.other_proto,
                                self.server.host, self.server.port))
        except ftplib.error_perm, err:
            self.assertEqual(str(err)[0:3], "522")
        else:
            self.fail("Exception not raised")

        # test bad args
        msg = "501 Invalid EPRT format."
        # len('|') > 3
        self.assertEqual(self.cmdresp('eprt ||||'), msg)
        # len('|') < 3
        self.assertEqual(self.cmdresp('eprt ||'), msg)
        # port > 65535
        self.assertEqual(self.cmdresp('eprt |%s|%s|65536|' % (self.proto,
                                                             self.HOST)), msg)
        # port < 0
        self.assertEqual(self.cmdresp('eprt |%s|%s|-1|' % (self.proto,
                                                          self.HOST)), msg)
        # port < 1024
        self.assertEqual(self.cmdresp('eprt |%s|%s|222|' % (self.proto,
                       self.HOST)), "501 Can't connect over a privileged port.")
        # proto > 2
        _cmd = 'eprt |3|%s|%s|' % (self.server.host, self.server.port)
        self.assertRaises(ftplib.error_perm,  self.client.sendcmd, _cmd)


        if self.proto == '1':
            # len(ip.octs) > 4
            self.assertEqual(self.cmdresp('eprt |1|1.2.3.4.5|2048|'), msg)
            # ip.oct > 255
            self.assertEqual(self.cmdresp('eprt |1|1.2.3.256|2048|'), msg)
            # bad proto
            resp = self.cmdresp('eprt |2|1.2.3.256|2048|')
            self.assert_("Network protocol not supported" in resp)

        # test connection
        sock = socket.socket(self.client.af)
        sock.bind((self.client.sock.getsockname()[0], 0))
        sock.listen(5)
        sock.settimeout(2)
        ip, port =  sock.getsockname()[:2]
        self.client.sendcmd('eprt |%s|%s|%s|' % (self.proto, ip, port))
        try:
            try:
                sock.accept()
            except socket.timeout:
                self.fail("Server didn't connect to passive socket")
        finally:
            sock.close()

    def test_epsv(self):
        # test wrong proto
        try:
            self.client.sendcmd('epsv ' + self.other_proto)
        except ftplib.error_perm, err:
            self.assertEqual(str(err)[0:3], "522")
        else:
            self.fail("Exception not raised")

        # proto > 2
        self.assertRaises(ftplib.error_perm, self.client.sendcmd, 'epsv 3')

        # test connection
        for cmd in ('EPSV', 'EPSV ' + self.proto):
            host, port = ftplib.parse229(self.client.sendcmd(cmd),
                         self.client.sock.getpeername())
            s = socket.socket(self.client.af, socket.SOCK_STREAM)
            s.settimeout(2)
            try:
                s.connect((host, port))
                self.client.sendcmd('abor')
            finally:
                s.close()

    def test_epsv_all(self):
        self.client.sendcmd('epsv all')
        self.assertRaises(ftplib.error_perm, self.client.sendcmd, 'pasv')
        self.assertRaises(ftplib.error_perm, self.client.sendport, self.HOST, 2000)
        self.assertRaises(ftplib.error_perm, self.client.sendcmd,
                          'eprt |%s|%s|%s|' % (self.proto, self.HOST, 2000))


class TestIPv4Environment(_TestNetworkProtocols):
    """Test PASV, EPSV, PORT and EPRT commands.

    Runs tests contained in _TestNetworkProtocols class by using IPv4
    plus some additional specific tests.
    """
    server_class = FTPd
    client_class = ftplib.FTP
    HOST = '127.0.0.1'

    def test_port_v4(self):
        # test connection
        sock = self.client.makeport()
        self.client.sendcmd('abor')
        sock.close()
        # test bad arguments
        ae = self.assertEqual
        msg = "501 Invalid PORT format."
        ae(self.cmdresp('port 127,0,0,1,1.1'), msg)    # sep != ','
        ae(self.cmdresp('port X,0,0,1,1,1'), msg)      # value != int
        ae(self.cmdresp('port 127,0,0,1,1,1,1'), msg)  # len(args) > 6
        ae(self.cmdresp('port 127,0,0,1'), msg)        # len(args) < 6
        ae(self.cmdresp('port 256,0,0,1,1,1'), msg)    # oct > 255
        ae(self.cmdresp('port 127,0,0,1,256,1'), msg)  # port > 65535
        ae(self.cmdresp('port 127,0,0,1,-1,0'), msg)   # port < 0
        msg = "501 Can't connect over a privileged port."
        ae(self.cmdresp('port %s,1,1' % self.HOST.replace('.',',')),msg) # port < 1024
        if "1.2.3.4" != self.HOST:
            msg = "501 Can't connect to a foreign address."
            ae(self.cmdresp('port 1,2,3,4,4,4'), msg)

    def test_eprt_v4(self):
        self.assertEqual(self.cmdresp('eprt |1|0.10.10.10|2222|'),
                         "501 Can't connect to a foreign address.")

    def test_pasv_v4(self):
        host, port = ftplib.parse227(self.client.sendcmd('pasv'))
        s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        s.settimeout(2)
        try:
            s.connect((host, port))
        finally:
            s.close()


class TestIPv6Environment(_TestNetworkProtocols):
    """Test PASV, EPSV, PORT and EPRT commands.

    Runs tests contained in _TestNetworkProtocols class by using IPv6
    plus some additional specific tests.
    """
    server_class = FTPd
    client_class = ftplib.FTP
    HOST = '::1'

    def test_port_v6(self):
        # PORT is not supposed to work
        self.assertRaises(ftplib.error_perm, self.client.sendport,
                          self.server.host, self.server.port)

    def test_pasv_v6(self):
        # PASV is still supposed to work to support clients using
        # IPv4 connecting to a server supporting both IPv4 and IPv6
        self.client.makepasv()

    def test_eprt_v6(self):
        self.assertEqual(self.cmdresp('eprt |2|::foo|2222|'),
                         "501 Can't connect to a foreign address.")


class TestIPv6MixedEnvironment(unittest.TestCase):
    """By running the server by specifying "::" as IP address the
    server is supposed to listen on all interfaces, supporting both
    IPv4 and IPv6 by using a single socket.

    What we are going to do here is starting the server in this
    manner and try to connect by using an IPv4 client.
    """
    server_class = FTPd
    client_class = ftplib.FTP
    HOST = "::"

    def setUp(self):
        self.server = self.server_class(self.HOST)
        self.server.start()
        self.client = None

    def tearDown(self):
        if self.client is not None:
            self.client.close()
        self.server.stop()

    def test_port_v4(self):
        noop = lambda x: x
        self.client = self.client_class()
        self.client.connect('127.0.0.1', self.server.port)
        self.client.set_pasv(False)
        self.client.sock.settimeout(2)
        self.client.login(USER, PASSWD)
        self.client.retrlines('list', noop)

    def test_pasv_v4(self):
        noop = lambda x: x
        self.client = self.client_class()
        self.client.connect('127.0.0.1', self.server.port)
        self.client.set_pasv(True)
        self.client.sock.settimeout(2)
        self.client.login(USER, PASSWD)
        self.client.retrlines('list', noop)
        # make sure pasv response doesn't return an IPv4-mapped address
        ip = self.client.makepasv()[0]
        self.assertFalse(ip.startswith("::ffff:"))


class TestCornerCases(unittest.TestCase):
    """Tests for any kind of strange situation for the server to be in,
    mainly referring to bugs signaled on the bug tracker.
    """
    server_class = FTPd
    client_class = ftplib.FTP

    def setUp(self):
        self.server = self.server_class()
        self.server.start()
        self.client = self.client_class()
        self.client.connect(self.server.host, self.server.port)
        self.client.sock.settimeout(2)
        self.client.login(USER, PASSWD)

    def tearDown(self):
        self.client.close()
        if self.server.running:
            self.server.stop()

    def test_port_race_condition(self):
        # Refers to bug #120, first sends PORT, then disconnects the
        # control channel before accept()ing the incoming data connection.
        # The original server behavior was to reply with "200 Active
        # data connection established" *after* the client had already
        # disconnected the control connection.
        sock = socket.socket(self.client.af)
        sock.bind((self.client.sock.getsockname()[0], 0))
        sock.listen(5)
        sock.settimeout(2)
        host, port =  sock.getsockname()[:2]

        hbytes = host.split('.')
        pbytes = [repr(port // 256), repr(port % 256)]
        bytes = hbytes + pbytes
        cmd = 'PORT ' + ','.join(bytes)
        self.client.sock.sendall(cmd + '\r\n')
        self.client.quit()
        sock.accept()
        sock.close()

    def test_stou_max_tries(self):
        # Emulates case where the max number of tries to find out a
        # unique file name when processing STOU command gets hit.

        class TestFS(ftpserver.AbstractedFS):
            def mkstemp(self, *args, **kwargs):
                raise IOError(errno.EEXIST, "No usable temporary file name found")

        self.server.handler.abstracted_fs = TestFS
        try:
            self.client.quit()
            self.client.connect(self.server.host, self.server.port)
            self.client.login(USER, PASSWD)
            self.assertRaises(ftplib.error_temp, self.client.sendcmd, 'stou')
        finally:
            self.server.handler.abstracted_fs = ftpserver.AbstractedFS

    def test_quick_connect(self):
        # Clients that connected and disconnected quickly could cause
        # the server to crash, due to a failure to catch errors in the
        # initial part of the connection process.
        # Tracked in issues #91, #104 and #105.
        # See also https://bugs.launchpad.net/zodb/+bug/135108
        import struct

        def connect(addr):
            s = socket.socket()
            s.setsockopt(socket.SOL_SOCKET, socket.SO_LINGER,
                         struct.pack('ii', 1, 0))
            try:
                s.connect(addr)
            except socket.error:
                pass
            s.close()

        for x in xrange(10):
            connect((self.server.host, self.server.port))
        for x in xrange(10):
            addr = self.client.makepasv()
            connect(addr)

    def test_error_on_callback(self):
        # test that the server do not crash in case an error occurs
        # while firing a scheduled function
        self.tearDown()
        flag = []
        original_logerror = ftpserver.logerror
        ftpserver.logerror = lambda msg: flag.append(msg)
        server = ftpserver.FTPServer((HOST, 0), ftpserver.FTPHandler)
        try:
            len1 = len(asyncore.socket_map)
            ftpserver.CallLater(0, lambda: 1 // 0)
            server.serve_forever(timeout=0, count=1)
            len2 = len(asyncore.socket_map)
            self.assertEqual(len1, len2)
            self.assertTrue(flag)
        finally:
            ftpserver.logerror = original_logerror
            server.close()

    def test_active_conn_error(self):
        # we open a socket() but avoid to invoke accept() to
        # reproduce this error condition:
        # http://code.google.com/p/pyftpdlib/source/detail?r=905
        sock = socket.socket()
        sock.bind((HOST, 0))
        port = sock.getsockname()[1]
        self.client.sock.settimeout(.1)
        try:
            resp = self.client.sendport(HOST, port)
        except ftplib.error_temp, err:
            self.assertEqual(str(err)[:3], '425')
        except socket.timeout:
            pass
        else:
            self.assertNotEqual(str(resp)[:3], '200')


class TestUnicodePathNames(unittest.TestCase):
    """Test FTP commands and responses by using path names with non
    ASCII characters.
    """
    server_class = FTPd
    client_class = ftplib.FTP

    def setUp(self):
        self.server = self.server_class()
        self.server.start()
        self.client = self.client_class()
        self.client.connect(self.server.host, self.server.port)
        self.client.sock.settimeout(2)
        self.client.login(USER, PASSWD)
        self.tempfile = os.path.basename(touch(TESTFN + '☃'))
        self.tempdir = TESTFN + '☺'
        os.mkdir(self.tempdir)

    def tearDown(self):
        self.client.close()
        self.server.stop()
        safe_remove(self.tempfile)
        if os.path.exists(self.tempdir):
            shutil.rmtree(self.tempdir)

    # --- fs operations

    def test_cwd(self):
        resp = self.client.cwd(self.tempdir)
        self.assertTrue(self.tempdir in resp)

    def test_mkd(self):
        os.rmdir(self.tempdir)
        dirname = self.client.mkd(self.tempdir)
        self.assertEqual(dirname, '/' + self.tempdir)
        self.assertTrue(os.path.isdir(self.tempdir))

    def test_rmdir(self):
        self.client.rmd(self.tempdir)

    def test_dele(self):
        self.client.delete(self.tempfile)
        self.assertFalse(os.path.exists(self.tempfile))

    def test_rnfr_rnto(self):
        tempname = TESTFN + '♥'
        try:
            # rename file
            self.client.rename(self.tempfile, tempname)
            self.assertTrue(os.path.isfile(tempname))
            self.client.rename(tempname, self.tempfile)
            # rename dir
            self.client.rename(self.tempdir, tempname)
            self.assertTrue(os.path.isdir(tempname))
            self.client.rename(tempname, self.tempdir)
        finally:
            safe_remove(tempname)
            safe_rmdir(tempname)

    def test_size(self):
        self.client.sendcmd('type i')
        self.client.sendcmd('size ' + self.tempfile)

    def test_mdtm(self):
        self.client.sendcmd('mdtm ' + self.tempfile)

    def test_stou(self):
        resp = self.client.sendcmd('stou ' + self.tempfile)
        try:
            self.assertTrue(self.tempfile in resp)
            self.client.quit()
        finally:
            os.remove(resp.rsplit(' ', 1)[1])

    if hasattr(os, 'chmod'):
        def test_site_chmod(self):
            self.client.sendcmd('site chmod 777 ' + self.tempfile)

    # --- listing cmds

    def _test_listing_cmds(self, cmd):
        ls = []
        touch(os.path.join(self.tempdir, self.tempfile))
        self.client.retrlines("%s %s" % (cmd, self.tempdir), ls.append)
        self.assertTrue(self.tempfile in ls[0])

    def test_list(self):
        self._test_listing_cmds('list')

    def test_nlst(self):
        self._test_listing_cmds('nlst')

    def test_mlsd(self):
        self._test_listing_cmds('mlsd')

    def test_mlst(self):
        # utility function for extracting the line of interest
        mlstline = lambda cmd: self.client.voidcmd(cmd).split('\n')[1]
        self.assertTrue('type=dir' in mlstline('mlst ' + self.tempdir))
        self.assertTrue('/' + self.tempdir in mlstline('mlst ' + self.tempdir))
        self.assertTrue('type=file' in mlstline('mlst ' + self.tempfile))
        self.assertTrue('/' + self.tempfile in mlstline('mlst ' + self.tempfile))

    # --- file transfer

    def test_stor(self):
        data = 'abcde12345' * 500
        os.remove(self.tempfile)
        dummy = StringIO.StringIO()
        dummy.write(data)
        dummy.seek(0)
        self.client.storbinary('stor ' + self.tempfile, dummy)
        dummy_recv = StringIO.StringIO()
        self.client.retrbinary('retr ' + self.tempfile, dummy_recv.write)
        dummy_recv.seek(0)
        self.assertEqual(dummy_recv.read(), data)

    def test_retr(self):
        data = 'abcd1234' * 500
        f = open(self.tempfile, 'wb')
        f.write(data)
        f.close()
        dummy = StringIO.StringIO()
        self.client.retrbinary('retr ' + self.tempfile, dummy.write)
        dummy.seek(0)
        self.assertEqual(dummy.read(), data)

    # XXX - provisional
    def test_encode_decode(self):
        self.client.sendcmd('type i')
        self.client.sendcmd('size ' + self.tempfile.decode('utf8').encode('utf8'))


class TestCommandLineParser(unittest.TestCase):
    """Test command line parser."""
    SYSARGV = sys.argv
    STDERR = sys.stderr

    def setUp(self):
        class DummyFTPServer(ftpserver.FTPServer):
            """An overridden version of FTPServer class which forces
            serve_forever() to return immediately.
            """
            def serve_forever(self, *args, **kwargs):
                return

        self.devnull = StringIO.StringIO()
        sys.argv = self.SYSARGV[:]
        sys.stderr = self.STDERR
        self.original_ftpserver_class = ftpserver.FTPServer
        ftpserver.FTPServer = DummyFTPServer

    def tearDown(self):
        self.devnull.close()
        sys.argv = self.SYSARGV[:]
        sys.stderr = self.STDERR
        ftpserver.FTPServer = self.original_ftpserver_class
        safe_rmdir(TESTFN)

    def test_a_option(self):
        sys.argv += ["-i", "localhost", "-p", "0"]
        ftpserver.main()
        sys.argv = self.SYSARGV[:]

        # no argument
        sys.argv += ["-a"]
        sys.stderr = self.devnull
        self.assertRaises(SystemExit, ftpserver.main)

    def test_p_option(self):
        sys.argv += ["-p", "0"]
        ftpserver.main()

        # no argument
        sys.argv = self.SYSARGV[:]
        sys.argv += ["-p"]
        sys.stderr = self.devnull
        self.assertRaises(SystemExit, ftpserver.main)

        # invalid argument
        sys.argv += ["-p foo"]
        self.assertRaises(SystemExit, ftpserver.main)

    def test_w_option(self):
        sys.argv += ["-w", "-p", "0"]
        warnings.filterwarnings("error")
        try:
            self.assertRaises(RuntimeWarning, ftpserver.main)
        finally:
            warnings.resetwarnings()

        # unexpected argument
        sys.argv = self.SYSARGV[:]
        sys.argv += ["-w foo"]
        sys.stderr = self.devnull
        self.assertRaises(SystemExit, ftpserver.main)

    def test_d_option(self):
        sys.argv += ["-d", TESTFN, "-p", "0"]
        if not os.path.isdir(TESTFN):
            os.mkdir(TESTFN)
        ftpserver.main()

        # without argument
        sys.argv = self.SYSARGV[:]
        sys.argv += ["-d"]
        sys.stderr = self.devnull
        self.assertRaises(SystemExit, ftpserver.main)

        # no such directory
        sys.argv = self.SYSARGV[:]
        sys.argv += ["-d %s" % TESTFN]
        safe_rmdir(TESTFN)
        self.assertRaises(ValueError, ftpserver.main)

    def test_r_option(self):
        sys.argv += ["-r 60000-61000", "-p", "0"]
        ftpserver.main()

        # without arg
        sys.argv = self.SYSARGV[:]
        sys.argv += ["-r"]
        sys.stderr = self.devnull
        self.assertRaises(SystemExit, ftpserver.main)

        # wrong arg
        sys.argv = self.SYSARGV[:]
        sys.argv += ["-r yyy-zzz"]
        self.assertRaises(SystemExit, ftpserver.main)

    def test_v_option(self):
        sys.argv += ["-v"]
        self.assertRaises(SystemExit, ftpserver.main)

        # unexpected argument
        sys.argv = self.SYSARGV[:]
        sys.argv += ["-v foo"]
        sys.stderr = self.devnull
        self.assertRaises(SystemExit, ftpserver.main)


def test_main(tests=None):
    test_suite = unittest.TestSuite()
    if tests is None:
        tests = [
                 TestAbstractedFS,
                 TestDummyAuthorizer,
                 TestCallLater,
                 TestCallEvery,
                 TestFtpAuthentication,
                 TestFtpDummyCmds,
                 TestFtpCmdsSemantic,
                 TestFtpFsOperations,
                 TestFtpStoreData,
                 TestFtpRetrieveData,
                 TestFtpListingCmds,
                 TestFtpAbort,
                 TestTimeouts,
                 TestConfigurableOptions,
                 TestCallbacks,
                 TestCornerCases,
                 TestUnicodePathNames,
                 TestCommandLineParser,
                 ]
        if SUPPORTS_IPV4:
            tests.append(TestIPv4Environment)
        if SUPPORTS_IPV6:
            tests.append(TestIPv6Environment)
        if SUPPORTS_HYBRID_IPV6:
            tests.append(TestIPv6MixedEnvironment)
        if SUPPORTS_SENDFILE:
            tests.append(TestFtpRetrieveDataNoSendfile)
            tests.append(TestFtpStoreDataNoSendfile)
        else:
            if os.name == 'posix':
                atexit.register(warnings.warn, "couldn't run sendfile() tests",
                                RuntimeWarning)

    for test in tests:
        test_suite.addTest(unittest.makeSuite(test))
    try:
        unittest.TextTestRunner(verbosity=2).run(test_suite)
    except:
        # in case of KeyboardInterrupt grant that the threaded FTP
        # server running in background gets stopped
        asyncore.socket_map.clear()
        raise


if __name__ == '__main__':
    test_main()
