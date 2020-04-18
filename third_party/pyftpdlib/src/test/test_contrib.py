#!/usr/bin/env python
# $Id$

#  pyftpdlib is released under the MIT license, reproduced below:
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

"""Tests for pyftpdlib.contrib namespace: handlers.py, authorizers.py
and filesystems.py modules.
"""

import ftplib
import unittest
import os
import random
import string
import warnings

try:
    import pwd
except ImportError:
    pwd = None

try:
    import ssl
except ImportError:
    ssl = None

try:
    from pywintypes import error as Win32ExtError
except ImportError:
    pass

from pyftpdlib import ftpserver
from pyftpdlib.contrib import authorizers
from pyftpdlib.contrib import handlers
from pyftpdlib.contrib import filesystems
from test_ftpd import *


FTPS_SUPPORT = hasattr(ftplib, 'FTP_TLS') and hasattr(handlers, 'TLS_FTPHandler')
CERTFILE = os.path.abspath(os.path.join(os.path.dirname(__file__), 'keycert.pem'))

if FTPS_SUPPORT:
    class FTPSClient(ftplib.FTP_TLS):
        """A modified version of ftplib.FTP_TLS class which implicitly
        secure the data connection after login().
        """
        def login(self, *args, **kwargs):
            ftplib.FTP_TLS.login(self, *args, **kwargs)
            self.prot_p()

    class FTPSServer(FTPd):
        """A threaded FTPS server used for functional testing."""
        handler = handlers.TLS_FTPHandler
        handler.certfile = CERTFILE

    class TLSTestMixin:
        server_class = FTPSServer
        client_class = FTPSClient
else:
    class TLSTestMixin:
        pass

# --- FTPS mixin tests
#
# What we're going to do here is repeating the original tests
# defined in test_ftpd.py but securing both control and data
# connections first.
# The tests are exactly the same, the only difference is that
# different classes are used (TLS_FPTHandler for the server
# and ftplib.FTP_TLS for the client) and everything goes through
# FTPS instead of clear-text FTP.
# This is very useful as we entirely reuse the existent test code
# base which is very large (more than 100 tests) and covers a lot
# of cases which are supposed to work no matter if the protocol
# is FTP or FTPS.

class TestFtpAuthenticationTLSMixin(TLSTestMixin, TestFtpAuthentication): pass
class TestTFtpDummyCmdsTLSMixin(TLSTestMixin, TestFtpDummyCmds): pass
class TestFtpCmdsSemanticTLSMixin(TLSTestMixin, TestFtpCmdsSemantic): pass
class TestFtpFsOperationsTLSMixin(TLSTestMixin, TestFtpFsOperations): pass
class TestFtpStoreDataTLSMixin(TLSTestMixin, TestFtpStoreData): pass
class TestFtpRetrieveDataTLSMixin(TLSTestMixin, TestFtpRetrieveData): pass
class TestFtpListingCmdsTLSMixin(TLSTestMixin, TestFtpListingCmds): pass
class TestFtpAbortTLSMixin(TLSTestMixin, TestFtpAbort):
    def test_oob_abor(self): pass

class TestTimeoutsTLSMixin(TLSTestMixin, TestTimeouts):
    def test_data_timeout_not_reached(self): pass

class TestConfigurableOptionsTLSMixin(TLSTestMixin, TestConfigurableOptions): pass
class TestCallbacksTLSMixin(TLSTestMixin, TestCallbacks):
    def test_on_file_received(self): pass
    def test_on_file_sent(self): pass
    def test_on_incomplete_file_received(self): pass
    def test_on_incomplete_file_sent(self): pass
    def test_on_login(self): pass
    def test_on_login_failed(self): pass
    def test_on_logout_quit(self): pass
    def test_on_logout_rein(self): pass
    def test_on_logout_user_issued_twice(self): pass


class TestIPv4EnvironmentTLSMixin(TLSTestMixin, TestIPv4Environment): pass
class TestIPv6EnvironmentTLSMixin(TLSTestMixin, TestIPv6Environment): pass
class TestCornerCasesTLSMixin(TLSTestMixin, TestCornerCases): pass


class TestFTPS(unittest.TestCase):
    """Specific tests fot TSL_FTPHandler class."""

    def setUp(self):
        self.server = FTPSServer()
        self.server.start()
        self.client = ftplib.FTP_TLS()
        self.client.connect(self.server.host, self.server.port)
        self.client.sock.settimeout(2)

    def tearDown(self):
        self.client.ssl_version = ssl.PROTOCOL_SSLv23
        self.server.handler.ssl_version = ssl.PROTOCOL_SSLv23
        self.server.handler.tls_control_required = False
        self.server.handler.tls_data_required = False
        self.client.close()
        self.server.stop()

    def assertRaisesWithMsg(self, excClass, msg, callableObj, *args, **kwargs):
        try:
            callableObj(*args, **kwargs)
        except excClass, why:
            if str(why) == msg:
                return
            raise self.failureException("%s != %s" % (str(why), msg))
        else:
            if hasattr(excClass,'__name__'):
                excName = excClass.__name__
            else:
                excName = str(excClass)
            raise self.failureException, "%s not raised" % excName

    def test_auth(self):
        # unsecured
        self.client.login(secure=False)
        self.assertFalse(isinstance(self.client.sock, ssl.SSLSocket))
        # secured
        self.client.login()
        self.assertTrue(isinstance(self.client.sock, ssl.SSLSocket))
        # AUTH issued twice
        msg = '503 Already using TLS.'
        self.assertRaisesWithMsg(ftplib.error_perm, msg,
                                 self.client.sendcmd, 'auth tls')

    def test_pbsz(self):
        # unsecured
        self.client.login(secure=False)
        msg = "503 PBSZ not allowed on insecure control connection."
        self.assertRaisesWithMsg(ftplib.error_perm, msg,
                                 self.client.sendcmd, 'pbsz 0')
        # secured
        self.client.login(secure=True)
        resp = self.client.sendcmd('pbsz 0')
        self.assertEqual(resp, "200 PBSZ=0 successful.")

    def test_prot(self):
        self.client.login(secure=False)
        msg = "503 PROT not allowed on insecure control connection."
        self.assertRaisesWithMsg(ftplib.error_perm, msg,
                                 self.client.sendcmd, 'prot p')
        self.client.login(secure=True)
        # secured
        self.client.prot_p()
        sock = self.client.transfercmd('list')
        while 1:
            if not sock.recv(1024):
                self.client.voidresp()
                break
        self.assertTrue(isinstance(sock, ssl.SSLSocket))
        # unsecured
        self.client.prot_c()
        sock = self.client.transfercmd('list')
        while 1:
            if not sock.recv(1024):
                self.client.voidresp()
                break
        self.assertFalse(isinstance(sock, ssl.SSLSocket))

    def test_feat(self):
        feat = self.client.sendcmd('feat')
        cmds = ['AUTH TLS', 'AUTH SSL', 'PBSZ', 'PROT']
        for cmd in cmds:
            self.assertTrue(cmd in feat)

    def test_unforseen_ssl_shutdown(self):
        self.client.login()
        try:
            sock = self.client.sock.unwrap()
        except socket.error, err:
            if err.errno == 0:
                return
            raise
        sock.sendall('noop')
        try:
            chunk = sock.recv(1024)
        except socket.error:
            pass
        else:
            self.assertEqual(chunk, "")

    def test_tls_control_required(self):
        self.server.handler.tls_control_required = True
        msg = "550 SSL/TLS required on the control channel."
        self.assertRaisesWithMsg(ftplib.error_perm, msg,
                                 self.client.sendcmd, "user " + USER)
        self.assertRaisesWithMsg(ftplib.error_perm, msg,
                                 self.client.sendcmd, "pass " + PASSWD)
        self.client.login(secure=True)

    def test_tls_data_required(self):
        self.server.handler.tls_data_required = True
        self.client.login(secure=True)
        msg = "550 SSL/TLS required on the data channel."
        self.assertRaisesWithMsg(ftplib.error_perm, msg,
                                 self.client.retrlines, 'list', lambda x: x)
        self.client.prot_p()
        self.client.retrlines('list', lambda x: x)

    def try_protocol_combo(self, server_protocol, client_protocol):
        self.server.handler.ssl_version = server_protocol
        self.client.ssl_version = client_protocol
        self.client.close()
        self.client.connect(self.server.host, self.server.port)
        try:
            self.client.login()
        except (ssl.SSLError, socket.error):
            self.client.close()
        else:
            self.client.quit()

    def test_ssl_version(self):
        protos = [ssl.PROTOCOL_SSLv3, ssl.PROTOCOL_SSLv23, ssl.PROTOCOL_TLSv1]
        if hasattr(ssl, "PROTOCOL_SSLv2"):
            protos.append(ssl.PROTOCOL_SSLv2)
            for proto in protos:
                self.try_protocol_combo(ssl.PROTOCOL_SSLv2, proto)
        for proto in protos:
            self.try_protocol_combo(ssl.PROTOCOL_SSLv3, proto)
        for proto in protos:
            self.try_protocol_combo(ssl.PROTOCOL_SSLv23, proto)
        for proto in protos:
            self.try_protocol_combo(ssl.PROTOCOL_TLSv1, proto)

    if hasattr(ssl, "PROTOCOL_SSLv2"):
        def test_sslv2(self):
            self.client.ssl_version = ssl.PROTOCOL_SSLv2
            self.client.close()
            self.client.connect(self.server.host, self.server.port)
            self.assertRaises(socket.error, self.client.login)
            self.client.ssl_version = ssl.PROTOCOL_SSLv2


# --- System dependant authorizers tests

class SharedAuthorizerTests(unittest.TestCase):
    """Tests valid for both UnixAuthorizer and WindowsAuthorizer for
    those parts which share the same API.
    """
    authorizer_class = None

    # --- utils

    def get_users(self):
        return self.authorizer_class._get_system_users()

    def get_current_user(self):
        if os.name == 'posix':
            return pwd.getpwuid(os.getuid()).pw_name
        else:
            return os.environ['USERNAME']

    def get_current_user_homedir(self):
        if os.name == 'posix':
            return pwd.getpwuid(os.getuid()).pw_dir
        else:
            return os.environ['USERPROFILE']

    def get_nonexistent_user(self):
        # return a user which does not exist on the system
        users = self.get_users()
        letters = string.ascii_lowercase
        while 1:
            user = ''.join([random.choice(letters) for i in range(10)])
            if user not in users:
                return user

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

    # --- /utils

    def test_get_home_dir(self):
        auth = self.authorizer_class()
        home = auth.get_home_dir(self.get_current_user())
        nonexistent_user = self.get_nonexistent_user()
        self.assertTrue(os.path.isdir(home))
        if auth.has_user('nobody'):
            home = auth.get_home_dir('nobody')
        self.assertRaises(ftpserver.AuthorizerError,
                          auth.get_home_dir, nonexistent_user)

    def test_has_user(self):
        auth = self.authorizer_class()
        current_user = self.get_current_user()
        nonexistent_user = self.get_nonexistent_user()
        self.assertTrue(auth.has_user(current_user))
        self.assertFalse(auth.has_user(nonexistent_user))
        auth = self.authorizer_class(rejected_users=[current_user])
        self.assertFalse(auth.has_user(current_user))

    def test_validate_authentication(self):
        # can't test for actual success in case of valid authentication
        # here as we don't have the user password
        if self.authorizer_class.__name__ == 'UnixAuthorizer':
            auth = self.authorizer_class(require_valid_shell=False)
        else:
            auth = self.authorizer_class()
        current_user = self.get_current_user()
        nonexistent_user = self.get_nonexistent_user()
        self.assertFalse(auth.validate_authentication(current_user, 'wrongpasswd'))
        self.assertFalse(auth.validate_authentication(nonexistent_user, 'bar'))

    def test_impersonate_user(self):
        auth = self.authorizer_class()
        nonexistent_user = self.get_nonexistent_user()
        try:
            if self.authorizer_class.__name__ == 'UnixAuthorizer':
                auth.impersonate_user(self.get_current_user(), '')
                self.assertRaises(ftpserver.AuthorizerError,
                                  auth.impersonate_user, nonexistent_user, 'pwd')
            else:
                self.assertRaises(Win32ExtError,
                            auth.impersonate_user, nonexistent_user, 'pwd')
                self.assertRaises(Win32ExtError,
                            auth.impersonate_user, self.get_current_user(), '')
        finally:
            auth.terminate_impersonation('')

    def test_terminate_impersonation(self):
        auth = self.authorizer_class()
        auth.terminate_impersonation('')
        auth.terminate_impersonation('')

    def test_get_perms(self):
        auth = self.authorizer_class(global_perm='elr')
        self.assertTrue('r' in auth.get_perms(self.get_current_user()))
        self.assertFalse('w' in auth.get_perms(self.get_current_user()))

    def test_has_perm(self):
        auth = self.authorizer_class(global_perm='elr')
        self.assertTrue(auth.has_perm(self.get_current_user(), 'r'))
        self.assertFalse(auth.has_perm(self.get_current_user(), 'w'))

    def test_messages(self):
        auth = self.authorizer_class(msg_login="login", msg_quit="quit")
        self.assertTrue(auth.get_msg_login, "login")
        self.assertTrue(auth.get_msg_quit, "quit")

    def test_error_options(self):
        wrong_user = self.get_nonexistent_user()
        self.assertRaisesWithMsg(ValueError,
           "rejected_users and allowed_users options are mutually exclusive",
           self.authorizer_class, allowed_users=['foo'], rejected_users=['bar'])
        self.assertRaisesWithMsg(ValueError,
                             'invalid username "anonymous"',
                             self.authorizer_class, allowed_users=['anonymous'])
        self.assertRaisesWithMsg(ValueError,
                            'invalid username "anonymous"',
                            self.authorizer_class, rejected_users=['anonymous'])
        self.assertRaisesWithMsg(ValueError,
                            'unknown user %s' % wrong_user,
                            self.authorizer_class, allowed_users=[wrong_user])
        self.assertRaisesWithMsg(ValueError, 'unknown user %s' % wrong_user,
                            self.authorizer_class, rejected_users=[wrong_user])

    def test_override_user_password(self):
        auth = self.authorizer_class()
        user = self.get_current_user()
        auth.override_user(user, password='foo')
        self.assertTrue(auth.validate_authentication(user, 'foo'))
        self.assertFalse(auth.validate_authentication(user, 'bar'))
        # make sure other settings keep using default values
        self.assertEqual(auth.get_home_dir(user), self.get_current_user_homedir())
        self.assertEqual(auth.get_perms(user), "elradfmw")
        self.assertEqual(auth.get_msg_login(user), "Login successful.")
        self.assertEqual(auth.get_msg_quit(user), "Goodbye.")

    def test_override_user_homedir(self):
        auth = self.authorizer_class()
        user = self.get_current_user()
        dir = os.path.dirname(os.getcwd())
        auth.override_user(user, homedir=dir)
        self.assertEqual(auth.get_home_dir(user), dir)
        # make sure other settings keep using default values
        #self.assertEqual(auth.get_home_dir(user), self.get_current_user_homedir())
        self.assertEqual(auth.get_perms(user), "elradfmw")
        self.assertEqual(auth.get_msg_login(user), "Login successful.")
        self.assertEqual(auth.get_msg_quit(user), "Goodbye.")

    def test_override_user_perm(self):
        auth = self.authorizer_class()
        user = self.get_current_user()
        auth.override_user(user, perm="elr")
        self.assertEqual(auth.get_perms(user), "elr")
        # make sure other settings keep using default values
        self.assertEqual(auth.get_home_dir(user), self.get_current_user_homedir())
        #self.assertEqual(auth.get_perms(user), "elradfmw")
        self.assertEqual(auth.get_msg_login(user), "Login successful.")
        self.assertEqual(auth.get_msg_quit(user), "Goodbye.")

    def test_override_user_msg_login_quit(self):
        auth = self.authorizer_class()
        user = self.get_current_user()
        auth.override_user(user, msg_login="foo", msg_quit="bar")
        self.assertEqual(auth.get_msg_login(user), "foo")
        self.assertEqual(auth.get_msg_quit(user), "bar")
        # make sure other settings keep using default values
        self.assertEqual(auth.get_home_dir(user), self.get_current_user_homedir())
        self.assertEqual(auth.get_perms(user), "elradfmw")
        #self.assertEqual(auth.get_msg_login(user), "Login successful.")
        #self.assertEqual(auth.get_msg_quit(user), "Goodbye.")

    def test_override_user_errors(self):
        if self.authorizer_class.__name__ == 'UnixAuthorizer':
            auth = self.authorizer_class(require_valid_shell=False)
        else:
            auth = self.authorizer_class()
        this_user = self.get_current_user()
        for x in self.get_users():
            if x != this_user:
                another_user = x
                break
        nonexistent_user = self.get_nonexistent_user()
        self.assertRaisesWithMsg(ValueError,
                                "at least one keyword argument must be specified",
                                auth.override_user, this_user)
        self.assertRaisesWithMsg(ValueError,
                                 'no such user %s' % nonexistent_user,
                                 auth.override_user, nonexistent_user, perm='r')
        if self.authorizer_class.__name__ == 'UnixAuthorizer':
            auth = self.authorizer_class(allowed_users=[this_user],
                                         require_valid_shell=False)
        else:
            auth = self.authorizer_class(allowed_users=[this_user])
        auth.override_user(this_user, perm='r')
        self.assertRaisesWithMsg(ValueError,
                                 '%s is not an allowed user' % another_user,
                                 auth.override_user, another_user, perm='r')
        if self.authorizer_class.__name__ == 'UnixAuthorizer':
            auth = self.authorizer_class(rejected_users=[this_user],
                                         require_valid_shell=False)
        else:
            auth = self.authorizer_class(rejected_users=[this_user])
        auth.override_user(another_user, perm='r')
        self.assertRaisesWithMsg(ValueError,
                                 '%s is not an allowed user' % this_user,
                                 auth.override_user, this_user, perm='r')
        self.assertRaisesWithMsg(ValueError,
                                 "can't assign password to anonymous user",
                                 auth.override_user, "anonymous", password='foo')


class TestUnixAuthorizer(SharedAuthorizerTests):
    """Unix authorizer specific tests."""

    authorizer_class = getattr(authorizers, "UnixAuthorizer", None)

    def test_get_perms_anonymous(self):
        auth = authorizers.UnixAuthorizer(global_perm='elr',
                                          anonymous_user=self.get_current_user())
        self.assertTrue('e' in auth.get_perms('anonymous'))
        self.assertFalse('w' in auth.get_perms('anonymous'))
        warnings.filterwarnings("ignore")
        auth.override_user('anonymous', perm='w')
        warnings.resetwarnings()
        self.assertTrue('w' in auth.get_perms('anonymous'))

    def test_has_perm_anonymous(self):
        auth = authorizers.UnixAuthorizer(global_perm='elr',
                                          anonymous_user=self.get_current_user())
        self.assertTrue(auth.has_perm(self.get_current_user(), 'r'))
        self.assertFalse(auth.has_perm(self.get_current_user(), 'w'))
        self.assertTrue(auth.has_perm('anonymous', 'e'))
        self.assertFalse(auth.has_perm('anonymous', 'w'))
        warnings.filterwarnings("ignore")
        auth.override_user('anonymous', perm='w')
        warnings.resetwarnings()
        self.assertTrue(auth.has_perm('anonymous', 'w'))

    def test_validate_authentication(self):
        # we can only test for invalid credentials
        auth = authorizers.UnixAuthorizer(require_valid_shell=False)
        ret = auth.validate_authentication('?!foo', '?!foo')
        self.assertFalse(ret)
        auth = authorizers.UnixAuthorizer(require_valid_shell=True)
        ret = auth.validate_authentication('?!foo', '?!foo')
        self.assertFalse(ret)

    def test_validate_authentication_anonymous(self):
        current_user = self.get_current_user()
        auth = authorizers.UnixAuthorizer(anonymous_user=current_user,
                                          require_valid_shell=False)
        self.assertFalse(auth.validate_authentication('foo', 'passwd'))
        self.assertFalse(auth.validate_authentication(current_user, 'passwd'))
        self.assertTrue(auth.validate_authentication('anonymous', 'passwd'))

    def test_require_valid_shell(self):

        def get_fake_shell_user():
            for user in self.get_users():
                shell = pwd.getpwnam(user).pw_shell
                # On linux fake shell is usually /bin/false, on
                # freebsd /usr/sbin/nologin;  in case of other
                # UNIX variants test needs to be adjusted.
                if '/false' in shell or '/nologin' in shell:
                    return user
            self.fail("no user found")

        user = get_fake_shell_user()
        self.assertRaisesWithMsg(ValueError,
                             "user %s has not a valid shell" % user,
                             authorizers.UnixAuthorizer, allowed_users=[user])
        # commented as it first fails for invalid home
        #self.assertRaisesWithMsg(ValueError,
        #                     "user %s has not a valid shell" % user,
        #                     authorizers.UnixAuthorizer, anonymous_user=user)
        auth = authorizers.UnixAuthorizer()
        self.assertTrue(auth._has_valid_shell(self.get_current_user()))
        self.assertFalse(auth._has_valid_shell(user))
        self.assertRaisesWithMsg(ValueError,
                                 "user %s has not a valid shell" % user,
                                 auth.override_user, user, perm='r')

    def test_not_root(self):
        # UnixAuthorizer is supposed to work only as super user
        auth = self.authorizer_class()
        try:
            auth.impersonate_user('nobody', '')
            self.assertRaisesWithMsg(ftpserver.AuthorizerError,
                                     "super user privileges are required",
                                     authorizers.UnixAuthorizer)
        finally:
            auth.terminate_impersonation('nobody')


class TestWindowsAuthorizer(SharedAuthorizerTests):
    """Windows authorizer specific tests."""

    authorizer_class = getattr(authorizers, "WindowsAuthorizer", None)

    def test_wrong_anonymous_credentials(self):
        user = self.get_current_user()
        self.assertRaises(Win32ExtError, self.authorizer_class,
                       anonymous_user=user, anonymous_password='$|1wrongpasswd')


if os.name == 'posix':
    class TestUnixFilesystem(unittest.TestCase):

        def test_case(self):
            root = os.getcwd()
            fs = filesystems.UnixFilesystem(root, None)
            self.assertEqual(fs.root, root)
            self.assertEqual(fs.cwd, root)
            cdup = os.path.dirname(root)
            self.assertEqual(fs.ftp2fs('..'), cdup)
            self.assertEqual(fs.fs2ftp(root), root)


def test_main():
    test_suite = unittest.TestSuite()
    tests = []
    warns = []

    # FTPS tests
    if FTPS_SUPPORT:
        ftps_tests = [TestFTPS,
                      TestFtpAuthenticationTLSMixin,
                      TestTFtpDummyCmdsTLSMixin,
                      TestFtpCmdsSemanticTLSMixin,
                      TestFtpFsOperationsTLSMixin,
                      TestFtpStoreDataTLSMixin,
                      TestFtpRetrieveDataTLSMixin,
                      TestFtpListingCmdsTLSMixin,
                      TestFtpAbortTLSMixin,
                      TestTimeoutsTLSMixin,
                      TestConfigurableOptionsTLSMixin,
                      TestCallbacksTLSMixin,
                      TestCornerCasesTLSMixin,
                     ]
        if SUPPORTS_IPV4:
            ftps_tests.append(TestIPv4EnvironmentTLSMixin)
        if SUPPORTS_IPV6:
            ftps_tests.append(TestIPv6EnvironmentTLSMixin)
        tests += ftps_tests
    else:
        if sys.version_info < (2, 7):
            warns.append("FTPS tests skipped (requires python 2.7)")
        elif ssl is None:
            warns.append("FTPS tests skipped (requires ssl module)")
        elif not hasattr(handlers, 'TLS_FTPHandler'):
            warns.append("FTPS tests skipped (requires PyOpenSSL module)")
        else:
            warns.append("FTPS tests skipped")

    # authorizers tests
    if os.name == 'posix':
        if hasattr(authorizers, "UnixAuthorizer"):
            try:
                authorizers.UnixAuthorizer()
            except ftpserver.AuthorizerError:  # not root
                warns.append("UnixAuthorizer tests skipped (root privileges are "
                             "required)")
            else:
                tests.append(TestUnixAuthorizer)
        else:
            try:
                import spwd
            except ImportError:
                warns.append("UnixAuthorizer tests skipped (spwd module is "
                             "missing")
            else:
                warns.append("UnixAuthorizer tests skipped")
    elif os.name in ('nt', 'ce'):
        if hasattr(authorizers, "WindowsAuthorizer"):
            tests.append(TestWindowsAuthorizer)
        else:
            try:
                import win32api
            except ImportError:
                warns.append("WindowsAuthorizer tests skipped (pywin32 extension "
                             "is required)")
            else:
                warns.append("WindowsAuthorizer tests skipped")

    if os.name == 'posix':
        tests.append(TestUnixFilesystem)

    for test in tests:
        test_suite.addTest(unittest.makeSuite(test))
    try:
        unittest.TextTestRunner(verbosity=2).run(test_suite)
    except:
        # in case of KeyboardInterrupt grant that the threaded FTP
        # server running in background gets stopped
        asyncore.socket_map.clear()
        raise
    for warn in warns:
        warnings.warn(warn, RuntimeWarning)

if __name__ == '__main__':
    test_main()
