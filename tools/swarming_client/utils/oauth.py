# Copyright 2013 The LUCI Authors. All rights reserved.
# Use of this source code is governed under the Apache License, Version 2.0
# that can be found in the LICENSE file.

"""OAuth2 related utilities and implementation of browser based login flow."""

# pylint: disable=W0613

import base64
import BaseHTTPServer
import collections
import datetime
import json
import logging
import optparse
import os
import socket
import sys
import threading
import time
import urllib
import urlparse
import webbrowser

# All libraries here expect to find themselves in sys.path.
ROOT_DIR = os.path.dirname(os.path.dirname(os.path.abspath(
    __file__.decode(sys.getfilesystemencoding()))))
sys.path.insert(0, os.path.join(ROOT_DIR, 'third_party'))
sys.path.insert(0, os.path.join(ROOT_DIR, 'third_party', 'pyasn1'))
sys.path.insert(0, os.path.join(ROOT_DIR, 'third_party', 'rsa'))

import httplib2
import rsa

from pyasn1.codec.der import decoder
from pyasn1.type import univ

from oauth2client import client
from oauth2client import locked_file
from oauth2client import multistore_file

from third_party import requests
from utils import tools

from libs import luci_context


# Path to a file with cached OAuth2 credentials used by default. Can be
# overridden by command line option or env variable.
DEFAULT_OAUTH_TOKENS_CACHE = os.path.join(
    os.path.expanduser('~'), '.isolated_oauth')

# List of space separated OAuth scopes for generated tokens. GAE apps usually
# use userinfo.email scope for authentication.
OAUTH_SCOPES = 'https://www.googleapis.com/auth/userinfo.email'

# Endpoint to generate access tokens.
OAUTH_TOKEN_ENDPOINT = 'https://www.googleapis.com/oauth2/v3/token'


# OAuth authentication method configuration, used by utils/net.py.
# See doc string for 'make_oauth_config' for meaning of fields.
OAuthConfig = collections.namedtuple('OAuthConfig', [
  'disabled',
  'tokens_cache',
  'no_local_webserver',
  'webserver_port',
  'service_account_json',
  'use_luci_context_auth',
])


# Indicates that no OAuth-based authentication performed by this module should
# be automatically used.
DISABLED_OAUTH_CONFIG = OAuthConfig(
    disabled=True,
    tokens_cache=None,
    no_local_webserver=None,
    webserver_port=None,
    service_account_json=None,
    use_luci_context_auth=None)


# Access token with its expiration time (UTC datetime, or None if not known).
AccessToken = collections.namedtuple('AccessToken', [
  'token',
  'expires_at',
])


# Service account credentials as loaded from JSON file.
ServiceAccountCredentials = collections.namedtuple('ServiceAccountCredentials',
[
  'client_email',
  'client_id',
  'private_key', # PEM encoded.
  'private_key_id',
])


# Configuration fetched from a service, returned by _fetch_service_config.
_ServiceConfig = collections.namedtuple('_ServiceConfig', [
  'client_id',
  'client_secret',
  'primary_url',
])

# Process cache of _fetch_service_config results.
_service_config_cache = {}
_service_config_cache_lock = threading.Lock()


# LUCI context parameters as loaded from JSON file.
#
# See https://github.com/luci/luci-py/blob/master/client/LUCI_CONTEXT.md.
LocalAuthParameters = collections.namedtuple('LocalAuthParameters', [
  'rpc_port',
  'secret',
  'accounts',
  'default_account_id',
])

# Values of 'accounts' section of LUCI_CONTEXT["local_auth"].
LocalAuthAccount = collections.namedtuple('LocalAuthAccount', [
  'id',
])


class BadServiceAccountCredentials(Exception):
  """Service account JSON is missing or not valid."""


class BadLuciContextParameters(Exception):
  """LUCI context JSON parameters are missing or not valid."""


def make_oauth_config(
    disabled=None,
    tokens_cache=None,
    no_local_webserver=None,
    webserver_port=None,
    service_account_json=None):
  """Returns new instance of OAuthConfig.

  If some config option is not provided or None, it will be set to a reasonable
  default value. This function also acts as an authoritative place for default
  values of corresponding command line options.

  Args:
    disabled: True to completely turn off OAuth authentication.
    tokens_cache: path to a file with cached OAuth2 credentials.
    no_local_webserver: if True, do not try to run local web server that
        handles redirects. Use copy-pasted verification code instead.
    webserver_port: port to run local webserver on.
    service_account_json: path to JSON file with service account credentials.
  """
  if tokens_cache is None:
    tokens_cache = os.environ.get(
        'SWARMING_AUTH_TOKENS_CACHE', DEFAULT_OAUTH_TOKENS_CACHE)
  if no_local_webserver is None:
    no_local_webserver = tools.get_bool_env_var(
        'SWARMING_AUTH_NO_LOCAL_WEBSERVER')
  if webserver_port is None:
    webserver_port = 8090

  if service_account_json is None:
    service_account_json = os.environ.get('SWARMING_AUTH_SERVICE_ACCOUNT_JSON')

  use_luci_context_auth = has_local_auth()
  if disabled is None:
    disabled = (tools.is_headless()
                and not service_account_json and not use_luci_context_auth)

  if disabled:
    service_account_json = None
    use_luci_context_auth = False
  elif service_account_json and use_luci_context_auth:
    raise ValueError('Cannot use both service account and LUCI_CONTEXT')

  return OAuthConfig(
      disabled,
      tokens_cache,
      no_local_webserver,
      webserver_port,
      service_account_json,
      use_luci_context_auth)


def add_oauth_options(parser):
  """Appends OAuth related options to OptionParser."""
  default_config = make_oauth_config()
  parser.oauth_group = optparse.OptionGroup(parser, 'OAuth options')
  parser.oauth_group.add_option(
      '--auth-disabled',
      action='store_true',
      default=False,
      help='Use it to disable OAuth and rely only on IP whitelist for '
           'authentication. OAuth gets disabled by default on bots that '
           'do not have credentials.')
  parser.oauth_group.add_option(
      '--auth-tokens-cache',
      default=default_config.tokens_cache,
      metavar='FILE',
      help='Path to a file with oauth2client tokens cache. It should be a '
           'safe location accessible only to a current user: knowing content '
           'of this file is roughly equivalent to knowing account password. '
           'Can also be set with SWARMING_AUTH_TOKENS_CACHE environment '
           'variable. [default: %default]')
  parser.oauth_group.add_option(
      '--auth-no-local-webserver',
      action='store_true',
      default=default_config.no_local_webserver,
      help='Do not run a local web server when performing OAuth2 login flow. '
           'Can also be set with SWARMING_AUTH_NO_LOCAL_WEBSERVER=1 '
           'environment variable. [default: %default]')
  parser.oauth_group.add_option(
      '--auth-host-port',
      type=int,
      default=default_config.webserver_port,
      metavar='PORT',
      help='Port a local web server should listen on. Used only if '
           '--auth-no-local-webserver is not set. [default: %default]')
  parser.oauth_group.add_option(
      '--auth-service-account-json',
      default=default_config.service_account_json,
      metavar='FILE',
      help='Path to a JSON file with service account credentials to use. '
           'Can be generated by "Generate new JSON key" button in '
           '"Credentials" section of any Cloud Console project. The value can '
           'also be set with SWARMING_AUTH_SERVICE_ACCOUNT_JSON environment '
           'variable. [default: %default]')
  parser.add_option_group(parser.oauth_group)
  # Use this occasion to monkey patch oauth2client's LockedFile to do retries by
  # default.
  _monkey_patch_oauth2client_locked_file()


def extract_oauth_config_from_options(options):
  """Given OptionParser with oauth options, extracts OAuthConfig from it.

  OptionParser should be populated with oauth options by 'add_oauth_options'.
  """
  if options.auth_service_account_json and options.auth_disabled:
    raise ValueError('Can\'t use service account together with --auth-disabled')

  # If '--auth-disabled' wasn't provided, let 'make_oauth_config' decide whether
  # to disable auth or not (by passing in None).
  cfg = make_oauth_config(
      disabled=True if options.auth_disabled else None,
      tokens_cache=options.auth_tokens_cache,
      no_local_webserver=options.auth_no_local_webserver,
      webserver_port=options.auth_host_port,
      service_account_json=options.auth_service_account_json)

  # Validate service account JSON is correct by trying to load it.
  if cfg.service_account_json:
    try:
      acc = _load_service_account_json(cfg.service_account_json)
      _parse_private_key(acc.private_key)
    except BadServiceAccountCredentials as exc:
      raise ValueError('Bad service account credentials: %s' % exc)

  # Validate LUCI context JSON is correct by trying to load it.
  if cfg.use_luci_context_auth:
    try:
      _load_local_auth()
    except BadLuciContextParameters as exc:
      raise ValueError('Bad LUCI_CONTEXT local_auth parameters: %s' % exc)

  return cfg


def load_access_token(urlhost, config):
  """Returns cached AccessToken if it is not expired yet."""
  assert isinstance(config, OAuthConfig)
  if config.disabled:
    return None
  auth_service_url = _fetch_auth_service_url(urlhost)
  if not auth_service_url:
    return None
  storage = _get_storage(auth_service_url, config)
  credentials = storage.get()
  # Missing?
  if not credentials or credentials.invalid:
    return None
  # Expired?
  if not credentials.access_token or credentials.access_token_expired:
    return None
  return AccessToken(credentials.access_token, credentials.token_expiry)


def create_access_token(urlhost, config, allow_user_interaction):
  """Mints and caches new access_token, launching OAuth2 dance if necessary.

  Args:
    urlhost: base URL of a host to make OAuth2 token for.
    config: OAuthConfig instance.
    allow_user_interaction: if False, do not use interactive browser based
        flow (return None instead if it is required).

  Returns:
    AccessToken on success.
    None on error or if OAuth2 flow was interrupted.
  """
  assert isinstance(config, OAuthConfig)
  if config.disabled:
    return None
  auth_service_url = _fetch_auth_service_url(urlhost)
  if not auth_service_url:
    return None

  storage = _get_storage(auth_service_url, config)
  credentials = None

  if config.service_account_json:
    # 2-legged flow that uses service account credentials.
    try:
      service_account = _load_service_account_json(config.service_account_json)
    except BadServiceAccountCredentials as e:
      logging.error('Bad service account credentials: %s', e)
      return None

    # Body of token refresh request (with JWT assertion signed with secret key).
    body = urllib.urlencode({
      'assertion': _make_assertion_jwt(service_account),
      'grant_type': 'urn:ietf:params:oauth:grant-type:jwt-bearer',
    })

    # Exchange it for access_token.
    http = httplib2.Http(ca_certs=tools.get_cacerts_bundle())
    resp, content = http.request(
        uri=OAUTH_TOKEN_ENDPOINT,
        method='POST',
        body=body,
        headers={'Content-Type': 'application/x-www-form-urlencoded'})
    if resp.status != 200:
      logging.error(
          'Failed to grab access token for service account: %r', content)
      return None

    try:
      token = json.loads(content)
      access_token = token['access_token']
      expires_at = None
      if 'expires_in' in token:
        expires_at = datetime.datetime.utcnow()
        expires_at += datetime.timedelta(seconds=int(token['expires_in']))
    except (KeyError, ValueError) as e:
      logging.error('Unexpected access token response format: %s', e)
      return None

    credentials = client.OAuth2Credentials(
        access_token=access_token,
        client_id=service_account.client_id,
        client_secret=None,
        refresh_token=None,
        token_expiry=expires_at,
        token_uri=None,
        user_agent=None)
  else:
    # 3-legged flow with (perhaps cached) refresh token.
    credentials = storage.get()
    refreshed = False
    if credentials and not credentials.invalid:
      try:
        credentials.refresh(httplib2.Http(ca_certs=tools.get_cacerts_bundle()))
        refreshed = True
      except client.Error as err:
        logging.error('OAuth error: %s', err)

    # Refresh token is missing or invalid, go through full flow.
    if not refreshed:
      if not allow_user_interaction:
        return None
      credentials = _run_oauth_dance(auth_service_url, config)
      if not credentials:
        return None

  # Success.
  logging.info('OAuth access_token refreshed. Expires in %s.',
      credentials.token_expiry - datetime.datetime.utcnow())
  credentials.set_store(storage)
  storage.put(credentials)
  return AccessToken(credentials.access_token, credentials.token_expiry)


def purge_access_token(urlhost, config):
  """Deletes OAuth tokens that can be used to access |urlhost|."""
  assert isinstance(config, OAuthConfig)
  auth_service_url = _fetch_auth_service_url(urlhost)
  if auth_service_url:
    _get_storage(auth_service_url, config).delete()


def _get_storage(urlhost, config):
  """Returns oauth2client.Storage with tokens to access |urlhost|."""
  # Do not mix access_token caches for different service accounts.
  if config.service_account_json:
    creds = _load_service_account_json(config.service_account_json)
    key = 'sa:%s:%s' % (creds.client_id, urlhost.rstrip('/'))
  else:
    key = urlhost.rstrip('/')
  return multistore_file.get_credential_storage_custom_string_key(
      config.tokens_cache, key)


def _fetch_auth_service_url(urlhost):
  """Fetches URL of a main authentication service used by |urlhost|.

  Returns:
    * If |urlhost| is using a authentication service, returns its URL.
    * If |urlhost| is not using authentication servier, returns |urlhost|.
    * If there was a error communicating with |urlhost|, returns None.
  """
  service_config = _fetch_service_config(urlhost)
  if not service_config:
    return None
  url = (service_config.primary_url or urlhost).rstrip('/')
  assert url.startswith(
      ('https://', 'http://127.0.0.1:', 'http://localhost:')), url
  return url


def _fetch_service_config(urlhost):
  """Fetches OAuth related configuration from a service.

  The configuration includes OAuth client_id and client_secret, as well as
  URL of a primary authentication service (or None if not used).

  Returns:
    Instance of _ServiceConfig on success, None on failure.
  """
  def do_fetch():
    # client_secret is not really a secret in that case. So an attacker can
    # impersonate service's identity in OAuth2 flow. But that's generally
    # fine as long as a list of allowed redirect_uri's associated with client_id
    # is limited to 'localhost' or 'urn:ietf:wg:oauth:2.0:oob'. In that case
    # attacker needs some process running on user's machine to successfully
    # complete the flow and grab access_token. When you have malicious code
    # running on your machine you're screwed anyway.
    response = requests.get(
        '%s/auth/api/v1/server/oauth_config' % urlhost.rstrip('/'),
        verify=tools.get_cacerts_bundle())
    if response.status_code == 200:
      try:
        config = response.json()
        if not isinstance(config, dict):
          raise ValueError()
        return _ServiceConfig(
            config['client_id'],
            config['client_not_so_secret'],
            config.get('primary_url'))
      except (KeyError, ValueError) as err:
        logging.error('Invalid response from the service: %s', err)
    else:
      logging.warning(
          'Error when fetching oauth_config, HTTP status code %d',
          response.status_code)
    return None

  # Use local cache to avoid unnecessary network calls.
  with _service_config_cache_lock:
    if urlhost not in _service_config_cache:
      config = do_fetch()
      if config:
        _service_config_cache[urlhost] = config
    return _service_config_cache.get(urlhost)


def _monkey_patch_oauth2client_locked_file():
  """Enables the use of locked file by multiple processes by retrying for a few
  seconds.

  When running multiple processes that reads the same file, even if it is open
  for a few ms, there's a risk of one getting the lock of another instance, and
  this raises, particularly on Windows. Workaround by enforcing retry.
  """
  locked_file.LockedFile.open_and_lock.im_func.func_defaults = (60, 0.05)


# Service account related code.


def _load_service_account_json(path):
  """Loads ServiceAccountCredentials from a JSON file.

  Raises BadServiceAccountCredentials if file is missing or not valid.
  """
  try:
    with open(path, 'r') as f:
      data = json.load(f)
    return ServiceAccountCredentials(
        client_email=str(data['client_email']),
        client_id=str(data['client_id']),
        private_key=str(data['private_key']),
        private_key_id=str(data['private_key_id']))
  except IOError as e:
    raise BadServiceAccountCredentials('Can\'t open %s: %s' % (path, e))
  except ValueError as e:
    raise BadServiceAccountCredentials('Not a JSON file %s: %s' % (path, e))
  except KeyError as e:
    raise BadServiceAccountCredentials('Missing key in %s: %s' % (path, e))


def _parse_private_key(pem):
  """PEM encoded OpenSSL private RSA key -> rsa.PrivateKey."""
  # Cloud console generates OpenSSL compatible private RSA keys. 'rsa' library
  # doesn't support them natively. Do some ASN unwrapping to extract naked
  # RSA key (in der-encoded form). See https://www.ietf.org/rfc/rfc2313.txt.
  try:
    der = rsa.pem.load_pem(pem, 'PRIVATE KEY')
    keyinfo, _ = decoder.decode(der)
    if keyinfo[1][0] != univ.ObjectIdentifier('1.2.840.113549.1.1.1'):
        raise BadServiceAccountCredentials(
            'Not a DER-encoded OpenSSL private RSA key')
    private_key_der = keyinfo[2].asOctets()
  except IndexError:
    raise BadServiceAccountCredentials(
        'Not a DER-encoded OpenSSL private RSA key')
  return rsa.PrivateKey.load_pkcs1(private_key_der, format='DER')


def _make_assertion_jwt(service_account):
  """Generates signed assertion JWT for 2-legged OAuth flow."""
  # For more info see:
  # https://developers.google.com/accounts/docs/OAuth2ServiceAccount.
  now = long(time.time())
  payload = {
    'aud': OAUTH_TOKEN_ENDPOINT,
    'scope': OAUTH_SCOPES,
    'iat': now,
    'exp': now + 3600,
    'iss': service_account.client_email,
  }
  # oauth2client knows how to use PyCrypo or PyOpenSSL for signing. Both are
  # heavy libraries, that require compiled extensions. Use pure python 'rsa' lib
  # instead. It is slower, but we do not care (since this code path is exercised
  # only when access token expires (once an hour).
  pkey = _parse_private_key(service_account.private_key)
  return _make_signed_jwt(payload, pkey)


def _make_signed_jwt(payload, pkey):
  """Wraps |payload| dict into signed JSON Web Token."""
  # See http://self-issued.info/docs/draft-jones-json-web-token.html.
  as_json = lambda d: json.dumps(d, sort_keys=True, separators=(',', ':'))
  b64encode = lambda d: base64.urlsafe_b64encode(d).rstrip('=')
  to_sign = '%s.%s' % (
      b64encode(as_json({'typ': 'JWT', 'alg': 'RS256'})),
      b64encode(as_json(payload)))
  signature = rsa.sign(to_sign, pkey, 'SHA-256')
  return '%s.%s' % (to_sign, b64encode(signature))


# The chunk of code below is based on oauth2client.tools module, but adapted for
# usage of _fetch_service_config, our command line arguments, and so on.


def _run_oauth_dance(urlhost, config):
  """Perform full OAuth2 dance with the browser."""
  def out(s):
    print s
  def err(s):
    print >> sys.stderr, s

  # Fetch client_id and client_secret from the service itself.
  service_config = _fetch_service_config(urlhost)
  if not service_config:
    err('Couldn\'t fetch OAuth configuration')
    return None
  if not service_config.client_id or not service_config.client_secret:
    err('OAuth is not configured on the service')
    return None

  flow = client.OAuth2WebServerFlow(
      service_config.client_id,
      service_config.client_secret,
      OAUTH_SCOPES,
      approval_prompt='force')

  use_local_webserver = not config.no_local_webserver
  port = config.webserver_port
  if use_local_webserver:
    success = False
    try:
      httpd = ClientRedirectServer(('localhost', port), ClientRedirectHandler)
    except socket.error:
      pass
    else:
      success = True
    use_local_webserver = success
    if not success:
      out(
        'Failed to start a local webserver listening on port %d.\n'
        'Please check your firewall settings and locally running programs that '
        'may be blocking or using those ports.\n\n'
        'Falling back to --auth-no-local-webserver and continuing with '
        'authentication.\n' % port)

  if use_local_webserver:
    oauth_callback = 'http://localhost:%s/' % port
  else:
    oauth_callback = client.OOB_CALLBACK_URN
  flow.redirect_uri = oauth_callback
  authorize_url = flow.step1_get_authorize_url()

  if use_local_webserver:
    webbrowser.open(authorize_url, new=1, autoraise=True)
    out(
      'Your browser has been opened to visit:\n\n'
      '    %s\n\n'
      'If your browser is on a different machine then exit and re-run this '
      'application with the command-line parameter\n\n'
      '  --auth-no-local-webserver\n' % authorize_url)
  else:
    out(
      'Go to the following link in your browser:\n\n'
      '    %s\n' % authorize_url)

  try:
    code = None
    if use_local_webserver:
      httpd.handle_request()
      if 'error' in httpd.query_params:
        err('Authentication request was rejected.')
        return None
      if 'code' not in httpd.query_params:
        err(
          'Failed to find "code" in the query parameters of the redirect.\n'
          'Try running with --auth-no-local-webserver.')
        return None
      code = httpd.query_params['code']
    else:
      code = raw_input('Enter verification code: ').strip()
  except KeyboardInterrupt:
    err('Canceled.')
    return None

  try:
    return flow.step2_exchange(code)
  except client.FlowExchangeError as e:
    err('Authentication has failed: %s' % e)
    return None


# LUCI context auth related code.


def has_local_auth():
  """Checks LUCI_CONTEXT to see if we should enable ambient authentication."""
  if not luci_context.read('local_auth'):
    return False
  try:
    params = _load_local_auth()
  except BadLuciContextParameters as exc:
    logging.error('LUCI_CONTEXT["local_auth"] is broken, ignoring it: %s', exc)
    return False

  # Old protocol doesn't specify 'accounts' at all. It has only one account that
  # is always enabled.
  #
  # TODO(vadimsh): Get rid of support of old protocol when it isn't deployed
  # anywhere anymore.
  if not params.accounts:
    return True

  # In the new protocol (when 'accounts' are always specified), use ambient
  # authentication only if it is explicitly enabled in LUCI_CONTEXT by non-None
  # 'default_account_id'.
  return bool(params.default_account_id)


def _load_local_auth():
  """Returns a LocalAuthParameters tuple from LUCI_CONTEXT.

  Returns:
    LocalAuthParameters for connecting to a local auth server.

  Raises:
    BadLuciContextParameters if file is missing or not valid.
  """
  data = luci_context.read('local_auth')
  if data is None:
    raise BadLuciContextParameters('Missing "local_auth" in LUCI_CONTEXT')

  try:
    return LocalAuthParameters(
      rpc_port=int(data['rpc_port']),
      secret=str(data['secret']),
      accounts=[
        LocalAuthAccount(id=acc['id']) for acc in data.get('accounts', [])
      ],
      default_account_id=data.get('default_account_id'))
  except (ValueError, KeyError):
    data['secret'] = '...'  # note: 'data' is a copy, it's fine to mutate it
    raise BadLuciContextParameters(
        'Invalid "local_auth" section in LUCI_CONTEXT: %r' % (data,))


def _get_luci_context_access_token(local_auth):
  """Returns a valid AccessToken from the local LUCI context auth server.

  Returns:
    AccessToken on success.
    None on failure.
  """
  logging.debug(
      'local_auth: requesting an access token for account "%s"',
      local_auth.default_account_id)
  body = json.dumps({
    'account_id': local_auth.default_account_id,  # may be None for old protocol
    'scopes': [OAUTH_SCOPES],
    'secret': local_auth.secret,
  })
  http = httplib2.Http()
  host = 'http://127.0.0.1:%d' % local_auth.rpc_port
  resp, content = http.request(
      uri='%s/rpc/LuciLocalAuthService.GetOAuthToken' % host,
      method='POST',
      body=body,
      headers={'Content-Type': 'application/json'})
  if resp.status != 200:
    logging.error(
        'local_auth: Failed to grab access token from LUCI context server: %r',
        content)
    return None

  try:
    token = json.loads(content)
    error_code = token.get('error_code')
    error_message = token.get('error_message')
    access_token = token.get('access_token')
    expiry = token.get('expiry')
  except (KeyError, ValueError) as e:
    logging.error('local_auth: Unexpected access token response format: %s', e)
    return None

  if error_code or not access_token:
    logging.error(
        'local_auth: Error %d in retrieving access token: %s',
        error_code, error_message)
    return None

  try:
    expiry_dt = datetime.datetime.utcfromtimestamp(expiry)
  except (TypeError, ValueError) as e:
    logging.error('Invalid expiry in returned token: %s', e)
    return None

  logging.debug(
      'local_auth: got an access token for account "%s" that expires in %d sec',
       local_auth.default_account_id, expiry - time.time())

  access_token = AccessToken(access_token, expiry_dt)
  if not _validate_luci_context_access_token(access_token):
    logging.error('local_auth: the returned access token is invalid')
    return None
  return access_token


def _validate_luci_context_access_token(access_token):
  """Validates access_token to be a valid AccessToken.

  Args:
    access_token: an AccessToken instance to validate.

  Returns:
    True if a valid AccessToken that is not expired.
    False otherwise.
  """
  if isinstance(access_token, AccessToken) and access_token.token:
    # Valid if expires_at is None or it is not expired soon
    if access_token.expires_at:
      if (not isinstance(access_token.expires_at, datetime.datetime) or
          datetime.datetime.utcfromtimestamp(time.time()) >
          access_token.expires_at - datetime.timedelta(seconds=60)):
        return False
    return True
  return False


class ClientRedirectServer(BaseHTTPServer.HTTPServer):
  """A server to handle OAuth 2.0 redirects back to localhost.

  Waits for a single request and parses the query parameters
  into query_params and then stops serving.
  """
  query_params = {}


class ClientRedirectHandler(BaseHTTPServer.BaseHTTPRequestHandler):
  """A handler for OAuth 2.0 redirects back to localhost.

  Waits for a single request and parses the query parameters
  into the servers query_params and then stops serving.
  """

  def do_GET(self):
    """Handle a GET request.

    Parses the query parameters and prints a message
    if the flow has completed. Note that we can't detect
    if an error occurred.
    """
    self.send_response(200)
    self.send_header('Content-type', 'text/html')
    self.end_headers()
    query = self.path.split('?', 1)[-1]
    query = dict(urlparse.parse_qsl(query))
    self.server.query_params = query
    self.wfile.write('<html><head><title>Authentication Status</title></head>')
    self.wfile.write('<body><p>The authentication flow has completed.</p>')
    self.wfile.write('</body></html>')

  def log_message(self, _format, *args):
    """Do not log messages to stdout while running as command line program."""
