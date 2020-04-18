#
# Copyright 2015 Google Inc.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

"""Command-line utility for fetching/inspecting credentials.

oauth2l (pronounced "oauthtool") is a small utility for fetching
credentials, or inspecting existing credentials. Here we demonstrate
some sample use:

    $ oauth2l fetch userinfo.email bigquery compute
    Fetched credentials of type:
      oauth2client.client.OAuth2Credentials
    Access token:
      ya29.abcdefghijklmnopqrstuvwxyz123yessirree
    $ oauth2l header userinfo.email
    Authorization: Bearer ya29.zyxwvutsrqpnmolkjihgfedcba
    $ oauth2l validate thisisnotatoken
    <exit status: 1>
    $ oauth2l validate ya29.zyxwvutsrqpnmolkjihgfedcba
    $ oauth2l scopes ya29.abcdefghijklmnopqrstuvwxyz123yessirree
    https://www.googleapis.com/auth/bigquery
    https://www.googleapis.com/auth/compute
    https://www.googleapis.com/auth/userinfo.email

The `header` command is designed to be easy to use with `curl`:

    $ curl "$(oauth2l header bigquery)" \
           'https://www.googleapis.com/bigquery/v2/projects'

The token can also be printed in other formats, for easy chaining
into other programs:

    $ oauth2l fetch -f json_compact userinfo.email
    <one-line JSON object with credential information>
    $ oauth2l fetch -f bare drive
    ya29.suchT0kenManyCredentialsW0Wokyougetthepoint

"""

import httplib
import json
import logging
import os
import pkgutil
import sys
import textwrap

import gflags as flags
from google.apputils import appcommands
import oauth2client.client

import apitools.base.py as apitools_base
from apitools.base.py import cli as apitools_cli

FLAGS = flags.FLAGS
# We could use a generated client here, but it's used for precisely
# one URL, with one parameter and no worries about URL encoding. Let's
# go with simple.
_OAUTH2_TOKENINFO_TEMPLATE = (
    'https://www.googleapis.com/oauth2/v2/tokeninfo'
    '?access_token={access_token}'
)


flags.DEFINE_string(
    'client_secrets', '',
    'If specified, use the client ID/secret from the named '
    'file, which should be a client_secrets.json file as downloaded '
    'from the Developer Console.')
flags.DEFINE_string(
    'credentials_filename', '',
    '(optional) Filename for fetching/storing credentials.')
flags.DEFINE_string(
    'service_account_json_keyfile', '',
    'Filename for a JSON service account key downloaded from the Developer '
    'Console.')


def GetDefaultClientInfo():
    client_secrets = json.loads(pkgutil.get_data(
        'apitools.data', 'apitools_client_secrets.json'))['installed']
    return {
        'client_id': client_secrets['client_id'],
        'client_secret': client_secrets['client_secret'],
        'user_agent': 'apitools/0.2 oauth2l/0.1',
    }


def GetClientInfoFromFlags():
    """Fetch client info from FLAGS."""
    if FLAGS.client_secrets:
        client_secrets_path = os.path.expanduser(FLAGS.client_secrets)
        if not os.path.exists(client_secrets_path):
            raise ValueError('Cannot find file: %s' % FLAGS.client_secrets)
        with open(client_secrets_path) as client_secrets_file:
            client_secrets = json.load(client_secrets_file)
        if 'installed' not in client_secrets:
            raise ValueError('Provided client ID must be for an installed app')
        client_secrets = client_secrets['installed']
        return {
            'client_id': client_secrets['client_id'],
            'client_secret': client_secrets['client_secret'],
            'user_agent': 'apitools/0.2 oauth2l/0.1',
        }
    else:
        return GetDefaultClientInfo()


def _ExpandScopes(scopes):
    scope_prefix = 'https://www.googleapis.com/auth/'
    return [s if s.startswith('https://') else scope_prefix + s
            for s in scopes]


def _PrettyJson(data):
    return json.dumps(data, sort_keys=True, indent=4, separators=(',', ': '))


def _CompactJson(data):
    return json.dumps(data, sort_keys=True, separators=(',', ':'))


def _Format(fmt, credentials):
    """Format credentials according to fmt."""
    if fmt == 'bare':
        return credentials.access_token
    elif fmt == 'header':
        return 'Authorization: Bearer %s' % credentials.access_token
    elif fmt == 'json':
        return _PrettyJson(json.loads(credentials.to_json()))
    elif fmt == 'json_compact':
        return _CompactJson(json.loads(credentials.to_json()))
    elif fmt == 'pretty':
        format_str = textwrap.dedent('\n'.join([
            'Fetched credentials of type:',
            '  {credentials_type.__module__}.{credentials_type.__name__}',
            'Access token:',
            '  {credentials.access_token}',
        ]))
        return format_str.format(credentials=credentials,
                                 credentials_type=type(credentials))
    raise ValueError('Unknown format: {}'.format(fmt))

_FORMATS = set(('bare', 'header', 'json', 'json_compact', 'pretty'))


def _GetTokenScopes(access_token):
    """Return the list of valid scopes for the given token as a list."""
    url = _OAUTH2_TOKENINFO_TEMPLATE.format(access_token=access_token)
    response = apitools_base.MakeRequest(
        apitools_base.GetHttp(), apitools_base.Request(url))
    if response.status_code not in [httplib.OK, httplib.BAD_REQUEST]:
        raise apitools_base.HttpError.FromResponse(response)
    if response.status_code == httplib.BAD_REQUEST:
        return []
    return json.loads(response.content)['scope'].split(' ')


def _ValidateToken(access_token):
    """Return True iff the provided access token is valid."""
    return bool(_GetTokenScopes(access_token))


def FetchCredentials(scopes, client_info=None, credentials_filename=None):
    """Fetch a credential for the given client_info and scopes."""
    client_info = client_info or GetClientInfoFromFlags()
    scopes = _ExpandScopes(scopes)
    if not scopes:
        raise ValueError('No scopes provided')
    credentials_filename = credentials_filename or FLAGS.credentials_filename
    # TODO(craigcitro): Remove this logging nonsense once we quiet the
    # spurious logging in oauth2client.
    old_level = logging.getLogger().level
    logging.getLogger().setLevel(logging.ERROR)
    credentials = apitools_base.GetCredentials(
        'oauth2l', scopes, credentials_filename=credentials_filename,
        service_account_json_keyfile=FLAGS.service_account_json_keyfile,
        oauth2client_args='', **client_info)
    logging.getLogger().setLevel(old_level)
    if not _ValidateToken(credentials.access_token):
        credentials.refresh(apitools_base.GetHttp())
    return credentials


class _Email(apitools_cli.NewCmd):

    """Get user email."""

    usage = 'email <access_token>'

    def RunWithArgs(self, access_token):
        """Print the email address for this token, if possible."""
        userinfo = apitools_base.GetUserinfo(
            oauth2client.client.AccessTokenCredentials(access_token,
                                                       'oauth2l/1.0'))
        user_email = userinfo.get('email')
        if user_email:
            print user_email


class _Fetch(apitools_cli.NewCmd):

    """Fetch credentials."""

    usage = 'fetch <scope> [<scope> ...]'

    def __init__(self, name, flag_values):
        super(_Fetch, self).__init__(name, flag_values)
        flags.DEFINE_enum(
            'credentials_format', 'pretty', sorted(_FORMATS),
            'Output format for token.',
            short_name='f', flag_values=flag_values)

    def RunWithArgs(self, *scopes):
        """Fetch a valid access token and display it."""
        credentials = FetchCredentials(scopes)
        print _Format(FLAGS.credentials_format.lower(), credentials)


class _Header(apitools_cli.NewCmd):

    """Print credentials for a header."""

    usage = 'header <scope> [<scope> ...]'

    def RunWithArgs(self, *scopes):
        """Fetch a valid access token and display it formatted for a header."""
        print _Format('header', FetchCredentials(scopes))


class _Scopes(apitools_cli.NewCmd):

    """Get the list of scopes for a token."""

    usage = 'scopes <access_token>'

    def RunWithArgs(self, access_token):
        """Print the list of scopes for a valid token."""
        scopes = _GetTokenScopes(access_token)
        if not scopes:
            return 1
        for scope in sorted(scopes):
            print scope


class _Userinfo(apitools_cli.NewCmd):

    """Get userinfo."""

    usage = 'userinfo <access_token>'

    def __init__(self, name, flag_values):
        super(_Userinfo, self).__init__(name, flag_values)
        flags.DEFINE_enum(
            'format', 'json', sorted(('json', 'json_compact')),
            'Output format for userinfo.',
            short_name='f', flag_values=flag_values)

    def RunWithArgs(self, access_token):
        """Print the userinfo for this token (if we have the right scopes)."""
        userinfo = apitools_base.GetUserinfo(
            oauth2client.client.AccessTokenCredentials(access_token,
                                                       'oauth2l/1.0'))
        if FLAGS.format == 'json':
            print _PrettyJson(userinfo)
        else:
            print _CompactJson(userinfo)


class _Validate(apitools_cli.NewCmd):

    """Validate a token."""

    usage = 'validate <access_token>'

    def RunWithArgs(self, access_token):
        """Validate an access token. Exits with 0 if valid, 1 otherwise."""
        return 1 - (_ValidateToken(access_token))


def run_main():  # pylint:disable=invalid-name
    """Function to be used as setuptools script entry point."""
    # Put the flags for this module somewhere the flags module will look
    # for them.

    # pylint:disable=protected-access
    new_name = flags._GetMainModule()
    sys.modules[new_name] = sys.modules['__main__']
    for flag in FLAGS.FlagsByModuleDict().get(__name__, []):
        FLAGS._RegisterFlagByModule(new_name, flag)
        for key_flag in FLAGS.KeyFlagsByModuleDict().get(__name__, []):
            FLAGS._RegisterKeyFlagForModule(new_name, key_flag)
    # pylint:enable=protected-access

    # Now set __main__ appropriately so that appcommands will be
    # happy.
    sys.modules['__main__'] = sys.modules[__name__]
    appcommands.Run()
    sys.modules['__main__'] = sys.modules.pop(new_name)


def main(unused_argv):
    appcommands.AddCmd('email', _Email)
    appcommands.AddCmd('fetch', _Fetch)
    appcommands.AddCmd('header', _Header)
    appcommands.AddCmd('scopes', _Scopes)
    appcommands.AddCmd('userinfo', _Userinfo)
    appcommands.AddCmd('validate', _Validate)


if __name__ == '__main__':
    appcommands.Run()
