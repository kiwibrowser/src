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

"""An "authorizer" is a class handling authentications and permissions
of the FTP server. It is used by pyftpdlib.ftpserver.FTPHandler
class for:

- verifying user password
- getting user home directory
- checking user permissions when a filesystem read/write event occurs
- changing user when accessing the filesystem

This module contains two classes which implements such functionalities
in a system-specific way for both Unix and Windows.
"""

__all__ = []


import os
import errno

from pyftpdlib.ftpserver import DummyAuthorizer, AuthorizerError


def replace_anonymous(callable):
    """A decorator to replace anonymous user string passed to authorizer
    methods as first arugument with the actual user used to handle
    anonymous sessions.
    """
    def wrapper(self, username, *args, **kwargs):
        if username == 'anonymous':
            username = self.anonymous_user or username
        return callable(self, username, *args, **kwargs)
    return wrapper


class _Base(object):
    """Methods common to both Unix and Windows authorizers.
    Not supposed to be used directly.
    """

    def __init__(self):
        """Check for errors in the constructor."""
        if self.rejected_users and self.allowed_users:
            raise ValueError("rejected_users and allowed_users options are "
                             "mutually exclusive")

        users = self._get_system_users()
        for user in (self.allowed_users or self.rejected_users):
            if user == 'anonymous':
                raise ValueError('invalid username "anonymous"')
            if user not in users:
                raise ValueError('unknown user %s' % user)

        if self.anonymous_user is not None:
            if not self.has_user(self.anonymous_user):
                raise ValueError('no such user %s' % self.anonymous_user)
            home = self.get_home_dir(self.anonymous_user)
            if not os.path.isdir(home):
                raise ValueError('no valid home set for user %s'
                                 % self.anonymous_user)

    def override_user(self, username, password=None, homedir=None, perm=None,
                      msg_login=None, msg_quit=None):
        """Overrides the options specified in the class constructor
        for a specific user.
        """
        if not password and not homedir and not perm and not msg_login \
        and not msg_quit:
            raise ValueError("at least one keyword argument must be specified")
        if self.allowed_users and username not in self.allowed_users:
            raise ValueError('%s is not an allowed user' % username)
        if self.rejected_users and username in self.rejected_users:
            raise ValueError('%s is not an allowed user' % username)
        if username == "anonymous" and password:
            raise ValueError("can't assign password to anonymous user")
        if not self.has_user(username):
            raise ValueError('no such user %s' % username)

        if username in self._dummy_authorizer.user_table:
            # re-set parameters
            del self._dummy_authorizer.user_table[username]
        self._dummy_authorizer.add_user(username, password or "",
                                                  homedir or os.getcwd(),
                                                  perm or "",
                                                  msg_login or "",
                                                  msg_quit or "")
        if homedir is None:
            self._dummy_authorizer.user_table[username]['home'] = ""

    def get_msg_login(self, username):
        return self._get_key(username, 'msg_login') or self.msg_login

    def get_msg_quit(self, username):
        return self._get_key(username, 'msg_quit') or self.msg_quit

    def get_perms(self, username):
        overridden_perms = self._get_key(username, 'perm')
        if overridden_perms:
            return overridden_perms
        if username == 'anonymous':
            return 'elr'
        return self.global_perm

    def has_perm(self, username, perm, path=None):
        return perm in self.get_perms(username)

    def _get_key(self, username, key):
        if self._dummy_authorizer.has_user(username):
            return self._dummy_authorizer.user_table[username][key]

    def _is_rejected_user(self, username):
        """Return True if the user has been black listed via
        allowed_users or rejected_users options.
        """
        if self.allowed_users and username not in self.allowed_users:
            return True
        if self.rejected_users and username in self.rejected_users:
            return True
        return False


# Note: requires python >= 2.5
try:
    import pwd, spwd, crypt
except ImportError:
    pass
else:
    __all__.extend(['BaseUnixAuthorizer', 'UnixAuthorizer'])

    # the uid/gid the server runs under
    PROCESS_UID = os.getuid()
    PROCESS_GID = os.getgid()

    class BaseUnixAuthorizer(object):
        """An authorizer compatible with Unix user account and password
        database.
        This class should not be used directly unless for subclassing.
        Use higher-level UnixAuthorizer class instead.
        """

        def __init__(self, anonymous_user=None):
            if os.geteuid() != 0 or not spwd.getspall():
                raise AuthorizerError("super user privileges are required")
            self.anonymous_user = anonymous_user

            if self.anonymous_user is not None:
                if not self.anonymous_user in self._get_system_users():
                    raise ValueError('no such user %s' % self.anonymous_user)
                try:
                    pwd.getpwnam(self.anonymous_user).pw_dir
                except KeyError:
                    raise ValueError('no such user %s' % anonymous_user)

        # --- overridden / private API

        def validate_authentication(self, username, password):
            """Authenticates against shadow password db; return
            True on success.
            """
            if username == "anonymous":
                return self.anonymous_user is not None
            try:
                pw1 = spwd.getspnam(username).sp_pwd
                pw2 = crypt.crypt(password, pw1)
            except KeyError:  # no such username
                return False
            else:
                return pw1 == pw2

        @replace_anonymous
        def impersonate_user(self, username, password):
            """Change process effective user/group ids to reflect
            logged in user.
            """
            try:
                pwdstruct = pwd.getpwnam(username)
            except KeyError:
                raise AuthorizerError('no such user %s' % username)
            else:
                os.setegid(pwdstruct.pw_gid)
                os.seteuid(pwdstruct.pw_uid)

        def terminate_impersonation(self, username):
            """Revert process effective user/group IDs."""
            os.setegid(PROCESS_GID)
            os.seteuid(PROCESS_UID)

        @replace_anonymous
        def has_user(self, username):
            """Return True if user exists on the Unix system.
            If the user has been black listed via allowed_users or
            rejected_users options always return False.
            """
            return username in self._get_system_users()

        @replace_anonymous
        def get_home_dir(self, username):
            """Return user home directory."""
            try:
                return pwd.getpwnam(username).pw_dir
            except KeyError:
                raise AuthorizerError('no such user %s' % username)

        @staticmethod
        def _get_system_users():
            """Return all users defined on the UNIX system."""
            return [entry.pw_name for entry in pwd.getpwall()]

        def get_msg_login(self, username):
            return "Login successful."

        def get_msg_quit(self, username):
            return "Goodbye."

        def get_perms(self, username):
            return "elradfmw"

        def has_perm(self, username, perm, path=None):
            return perm in self.get_perms(username)


    class UnixAuthorizer(_Base, BaseUnixAuthorizer):
        """A wrapper on top of BaseUnixAuthorizer providing options
        to specify what users should be allowed to login, per-user
        options, etc.

        Example usages:

         >>> from pyftpdlib.contrib.authorizers import UnixAuthorizer
         >>> # accept all except root
         >>> auth = UnixAuthorizer(rejected_users=["root"])
         >>>
         >>> # accept some users only
         >>> auth = UnixAuthorizer(allowed_users=["matt", "jay"])
         >>>
         >>> # accept everybody and don't care if they have not a valid shell
         >>> auth = UnixAuthorizer(require_valid_shell=False)
         >>>
         >>> # set specific options for a user
         >>> auth.override_user("matt", password="foo", perm="elr")
        """

        # --- public API

        def __init__(self, global_perm="elradfmw",
                           allowed_users=[],
                           rejected_users=[],
                           require_valid_shell=True,
                           anonymous_user=None,
                           msg_login="Login successful.",
                           msg_quit="Goodbye."):
            """Parameters:

             - (string) global_perm:
                a series of letters referencing the users permissions;
                defaults to "elradfmw" which means full read and write
                access for everybody (except anonymous).

             - (list) allowed_users:
                a list of users which are accepted for authenticating
                against the FTP server; defaults to [] (no restrictions).

             - (list) rejected_users:
                a list of users which are not accepted for authenticating
                against the FTP server; defaults to [] (no restrictions).

             - (bool) require_valid_shell:
                Deny access for those users which do not have a valid shell
                binary listed in /etc/shells.
                If /etc/shells cannot be found this is a no-op.
                Anonymous user is not subject to this option, and is free
                to not have a valid shell defined.
                Defaults to True (a valid shell is required for login).

             - (string) anonymous_user:
                specify it if you intend to provide anonymous access.
                The value expected is a string representing the system user
                to use for managing anonymous sessions;  defaults to None
                (anonymous access disabled).

             - (string) msg_login:
                the string sent when client logs in.

             - (string) msg_quit:
                the string sent when client quits.
            """
            BaseUnixAuthorizer.__init__(self, anonymous_user)
            self.global_perm = global_perm
            self.allowed_users = allowed_users
            self.rejected_users = rejected_users
            self.anonymous_user = anonymous_user
            self.require_valid_shell = require_valid_shell
            self.msg_login = msg_login
            self.msg_quit = msg_quit

            self._dummy_authorizer = DummyAuthorizer()
            self._dummy_authorizer._check_permissions('', global_perm)
            _Base.__init__(self)
            if require_valid_shell:
                for username in self.allowed_users:
                    if not self._has_valid_shell(username):
                        raise ValueError("user %s has not a valid shell"
                                         % username)

        def override_user(self, username, password=None, homedir=None, perm=None,
                          msg_login=None, msg_quit=None):
            """Overrides the options specified in the class constructor
            for a specific user.
            """
            if self.require_valid_shell and username != 'anonymous':
                if not self._has_valid_shell(username):
                    raise ValueError("user %s has not a valid shell"
                                     % username)
            _Base.override_user(self, username, password, homedir, perm,
                                msg_login, msg_quit)

        # --- overridden / private API

        def validate_authentication(self, username, password):
            if username == "anonymous":
                return self.anonymous_user is not None
            if self._is_rejected_user(username):
                return False
            if self.require_valid_shell and username != 'anonymous':
                if not self._has_valid_shell(username):
                    return False
            overridden_password = self._get_key(username, 'pwd')
            if overridden_password:
                return overridden_password == password

            return BaseUnixAuthorizer.validate_authentication(self, username, password)

        @replace_anonymous
        def has_user(self, username):
            if self._is_rejected_user(username):
                return False
            return username in self._get_system_users()

        @replace_anonymous
        def get_home_dir(self, username):
            overridden_home = self._get_key(username, 'home')
            if overridden_home:
                return overridden_home
            return BaseUnixAuthorizer.get_home_dir(self, username)

        @staticmethod
        def _has_valid_shell(username):
            """Return True if the user has a valid shell binary listed
            in /etc/shells. If /etc/shells can't be found return True.
            """
            try:
                file = open('/etc/shells', 'r')
            except IOError, err:
                if err.errno == errno.ENOENT:
                    return True
                raise
            else:
                try:
                    try:
                        shell = pwd.getpwnam(username).pw_shell
                    except KeyError:  # invalid user
                        return False
                    for line in file:
                        if line.startswith('#'):
                            continue
                        line = line.strip()
                        if line == shell:
                            return True
                    return False
                finally:
                    file.close()


# Note: requires pywin32 extension
try:
    import _winreg
    import win32security, win32net, pywintypes, win32con, win32api
except ImportError:
    pass
else:
    __all__.extend(['BaseWindowsAuthorizer', 'WindowsAuthorizer'])

    class BaseWindowsAuthorizer(object):
        """An authorizer compatible with Windows user account and
        password database.
        This class should not be used directly unless for subclassing.
        Use higher-level WinowsAuthorizer class instead.
        """

        def __init__(self, anonymous_user=None, anonymous_password=None):
            # actually try to impersonate the user
            self.anonymous_user = anonymous_user
            self.anonymous_password = anonymous_password
            if self.anonymous_user is not None:
                self.impersonate_user(self.anonymous_user,
                                      self.anonymous_password)
                self.terminate_impersonation()

        def validate_authentication(self, username, password):
            if username == "anonymous":
                return self.anonymous_user is not None
            try:
                win32security.LogonUser(username, None, password,
                                        win32con.LOGON32_LOGON_INTERACTIVE,
                                        win32con.LOGON32_PROVIDER_DEFAULT)
            except pywintypes.error:
                return False
            else:
                return True

        @replace_anonymous
        def impersonate_user(self, username, password):
            """Impersonate the security context of another user."""
            handler = win32security.LogonUser(username, None, password,
                                              win32con.LOGON32_LOGON_INTERACTIVE,
                                              win32con.LOGON32_PROVIDER_DEFAULT)
            win32security.ImpersonateLoggedOnUser(handler)
            handler.Close()

        def terminate_impersonation(self, username):
            """Terminate the impersonation of another user."""
            win32security.RevertToSelf()

        @replace_anonymous
        def has_user(self, username):
            return username in self._get_system_users()

        @replace_anonymous
        def get_home_dir(self, username):
            """Return the user's profile directory, the closest thing
            to a user home directory we have on Windows.
            """
            try:
                sid = win32security.ConvertSidToStringSid(
                        win32security.LookupAccountName(None, username)[0])
            except pywintypes.error, err:
                raise AuthorizerError(err)
            path = r"SOFTWARE\Microsoft\Windows NT\CurrentVersion\ProfileList" + \
                   "\\" + sid
            try:
                key = _winreg.OpenKey(_winreg.HKEY_LOCAL_MACHINE, path)
            except WindowsError:
                raise AuthorizerError("No profile directory defined for user %s"
                                      % username)
            value = _winreg.QueryValueEx(key, "ProfileImagePath")[0]
            return win32api.ExpandEnvironmentStrings(value)

        @classmethod
        def _get_system_users(cls):
            """Return all users defined on the Windows system."""
            return [entry['name'] for entry in win32net.NetUserEnum(None, 0)[0]]

        def get_msg_login(self, username):
            return "Login successful."

        def get_msg_quit(self, username):
            return "Goodbye."

        def get_perms(self, username):
            return "elradfmw"

        def has_perm(self, username, perm, path=None):
            return perm in self.get_perms(username)


    class WindowsAuthorizer(_Base, BaseWindowsAuthorizer):
        """A wrapper on top of BaseWindowsAuthorizer providing options
        to specify what users should be allowed to login, per-user
        options, etc.

        Example usages:

         >>> from pyftpdlib.contrib.authorizers import WindowsAuthorizer
         >>> # accept all except Administrator
         >>> auth = UnixAuthorizer(rejected_users=["Administrator"])
         >>>
         >>> # accept some users only
         >>> auth = UnixAuthorizer(allowed_users=["matt", "jay"])
         >>>
         >>> # set specific options for a user
         >>> auth.override_user("matt", password="foo", perm="elr")
        """

        # --- public API

        def __init__(self, global_perm="elradfmw",
                           allowed_users=[],
                           rejected_users=[],
                           anonymous_user=None,
                           anonymous_password=None,
                           msg_login="Login successful.",
                           msg_quit="Goodbye."):
            """Parameters:

             - (string) global_perm:
                a series of letters referencing the users permissions;
                defaults to "elradfmw" which means full read and write
                access for everybody (except anonymous).

             - (list) allowed_users:
                a list of users which are accepted for authenticating
                against the FTP server; defaults to [] (no restrictions).

             - (list) rejected_users:
                a list of users which are not accepted for authenticating
                against the FTP server; defaults to [] (no restrictions).

             - (string) anonymous_user:
                specify it if you intend to provide anonymous access.
                The value expected is a string representing the system user
                to use for managing anonymous sessions.
                As for IIS, it is recommended to use Guest account.
                The common practice is to first enable the Guest user, which
                is disabled by default and then assign an empty password.
                Defaults to None (anonymous access disabled).

             - (string) anonymous_password:
                the password of the user who has been chosen to manage the
                anonymous sessions.  Defaults to None (empty password).

             - (string) msg_login:
                the string sent when client logs in.

             - (string) msg_quit:
                the string sent when client quits.
            """
            self.global_perm = global_perm
            self.allowed_users = allowed_users
            self.rejected_users = rejected_users
            self.anonymous_user = anonymous_user
            self.anonymous_password = anonymous_password
            self.msg_login = msg_login
            self.msg_quit = msg_quit
            self._dummy_authorizer = DummyAuthorizer()
            self._dummy_authorizer._check_permissions('', global_perm)
            _Base.__init__(self)
            # actually try to impersonate the user
            if self.anonymous_user is not None:
                self.impersonate_user(self.anonymous_user,
                                      self.anonymous_password)
                self.terminate_impersonation()

        def override_user(self, username, password=None, homedir=None, perm=None,
                          msg_login=None, msg_quit=None):
            """Overrides the options specified in the class constructor
            for a specific user.
            """
            _Base.override_user(self, username, password, homedir, perm,
                                msg_login, msg_quit)

        # --- overridden / private API

        def validate_authentication(self, username, password):
            """Authenticates against Windows user database; return
            True on success.
            """
            if username == "anonymous":
                return self.anonymous_user is not None
            if self.allowed_users and username not in self.allowed_users:
                return False
            if self.rejected_users and username in self.rejected_users:
                return False

            overridden_password = self._get_key(username, 'pwd')
            if overridden_password:
                return overridden_password == password
            else:
                return BaseWindowsAuthorizer.validate_authentication(self,
                                                            username, password)

        def impersonate_user(self, username, password):
            """Impersonate the security context of another user."""
            if username == "anonymous":
                username = self.anonymous_user or ""
                password = self.anonymous_password or ""
            return BaseWindowsAuthorizer.impersonate_user(self, username, password)

        @replace_anonymous
        def has_user(self, username):
            if self._is_rejected_user(username):
                return False
            return username in self._get_system_users()

        @replace_anonymous
        def get_home_dir(self, username):
            overridden_home = self._get_key(username, 'home')
            if overridden_home:
                return overridden_home
            return BaseWindowsAuthorizer.get_home_dir(self, username)
