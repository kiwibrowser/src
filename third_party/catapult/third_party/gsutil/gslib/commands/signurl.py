# -*- coding: utf-8 -*-
# Copyright 2014 Google Inc. All Rights Reserved.
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
"""Implementation of Url Signing workflow.

see: https://cloud.google.com/storage/docs/access-control#Signed-URLs)
"""

from __future__ import absolute_import

import base64
import calendar
from datetime import datetime
from datetime import timedelta
import getpass
import hashlib
import json
import re
import urllib

from apitools.base.py.exceptions import HttpError
from apitools.base.py.http_wrapper import MakeRequest
from apitools.base.py.http_wrapper import Request

from boto import config

from gslib.cloud_api import AccessDeniedException
from gslib.command import Command
from gslib.command_argument import CommandArgument
from gslib.cs_api_map import ApiSelector
from gslib.exception import CommandException
from gslib.storage_url import ContainsWildcard
from gslib.storage_url import StorageUrlFromString
from gslib.util import GetNewHttp
from gslib.util import NO_MAX
from gslib.util import UTF8

try:
  # Check for openssl.
  # pylint: disable=C6204
  from OpenSSL.crypto import FILETYPE_PEM
  from OpenSSL.crypto import load_pkcs12
  from OpenSSL.crypto import load_privatekey
  from OpenSSL.crypto import sign
  HAVE_OPENSSL = True
except ImportError:
  load_privatekey = None
  load_pkcs12 = None
  sign = None
  HAVE_OPENSSL = False
  FILETYPE_PEM = None

_AUTO_DETECT_REGION = 'auto'
_SIGNING_ALGO = 'GOOG4-RSA-SHA256'
_UNSIGNED_PAYLOAD = 'UNSIGNED-PAYLOAD'
_CANONICAL_REQUEST_FORMAT = ('{method}\n{resource}\n{query_string}\n{headers}'
                             '\n{signed_headers}\n{hashed_payload}')
_STRING_TO_SIGN_FORMAT = ('{signing_algo}\n{request_time}\n{credential_scope}'
                          '\n{hashed_request}')
_SIGNED_URL_FORMAT = ('https://{host}/{path}?x-goog-signature={sig}&'
                      '{query_string}')

_SYNOPSIS = """
  gsutil signurl [-c <content_type>] [-d <duration>] [-m <http_method>] \\
      [-p <password>] [-r <region>] keystore-file url...
"""

_DETAILED_HELP_TEXT = ("""
<B>SYNOPSIS</B>
""" + _SYNOPSIS + """


<B>DESCRIPTION</B>
  The signurl command will generate a signed URL that embeds authentication data
  so the URL can be used by someone who does not have a Google account. Please
  see the `Signed URLs documentation
  <https://cloud.google.com/storage/docs/access-control/signed-urls>`_ for
  background about signed URLs.

  Multiple gs:// urls may be provided and may contain wildcards. A signed url
  will be produced for each provided url, authorized
  for the specified HTTP method and valid for the given duration.

  Note: Unlike the gsutil ls command, the signurl command does not support
  operations on sub-directories. For example, if you run the command:

    gsutil signurl <private-key-file> gs://some-bucket/some-object/

  The signurl command uses the private key for a service account (the
  '<private-key-file>' argument) to generate the cryptographic
  signature for the generated URL. The private key file must be in PKCS12
  or JSON format. If the private key is encrypted the signed url command will
  prompt for the passphrase used to protect the private key file
  (default 'notasecret'). For more information regarding generating a private
  key for use with the signurl command please see the `Authentication
  documentation.
  <https://cloud.google.com/storage/docs/authentication#generating-a-private-key>`_

  gsutil will look up information about the object "some-object/" (with a
  trailing slash) inside bucket "some-bucket", as opposed to operating on
  objects nested under gs://some-bucket/some-object. Unless you actually
  have an object with that name, the operation will fail.

<B>OPTIONS</B>
  -m           Specifies the HTTP method to be authorized for use
               with the signed url, default is GET. You may also specify
               RESUMABLE to create a signed resumable upload start URL. When
               using a signed URL to start a resumable upload session, you will
               need to specify the 'x-goog-resumable:start' header in the
               request or else signature validation will fail.

  -d           Specifies the duration that the signed url should be valid
               for, default duration is 1 hour.

               Times may be specified with no suffix (default hours), or
               with s = seconds, m = minutes, h = hours, d = days.

               This option may be specified multiple times, in which case
               the duration the link remains valid is the sum of all the
               duration options.

  -c           Specifies the content type for which the signed url is
               valid for.

  -p           Specify the keystore password instead of prompting.

  -r <region>  Specifies the `region
               <https://cloud.google.com/storage/docs/bucket-locations>`_ in
               which the resources for which you are creating signed URLs are
               stored.

               Default value is 'auto' which will cause gsutil to fetch the
               region for the resource. When auto-detecting the region, the
               current gsutil user's credentials, not the credentials from the
               private-key-file, are used to fetch the bucket's metadata.

               This option must be specified and not 'auto' when generating a
               signed URL to create a bucket.

<B>USAGE</B>
  Create a signed url for downloading an object valid for 10 minutes:

    gsutil signurl -d 10m <private-key-file> gs://<bucket>/<object>

  Create a signed url, valid for one hour, for uploading a plain text
  file via HTTP PUT:

    gsutil signurl -m PUT -d 1h -c text/plain <private-key-file> \\
        gs://<bucket>/<obj>

  To construct a signed URL that allows anyone in possession of
  the URL to PUT to the specified bucket for one day, creating
  an object of Content-Type image/jpg, run:

    gsutil signurl -m PUT -d 1d -c image/jpg <private-key-file> \\
        gs://<bucket>/<obj>

  To construct a signed URL that allows anyone in possession of
  the URL to POST a resumable upload to the specified bucket for one day,
  creating an object of Content-Type image/jpg, run:

    gsutil signurl -m RESUMABLE -d 1d -c image/jpg <private-key-file> \\
        gs://bucket/<obj>
""")


def _NowUTC():
  """Returns the current utc time as a datetime object."""
  return datetime.utcnow()


def _DurationToTimeDelta(duration):
  r"""Parses the given duration and returns an equivalent timedelta."""

  match = re.match(r'^(\d+)([dDhHmMsS])?$', duration)
  if not match:
    raise CommandException('Unable to parse duration string')

  duration, modifier = match.groups('h')
  duration = int(duration)
  modifier = modifier.lower()

  if modifier == 'd':
    ret = timedelta(days=duration)
  elif modifier == 'h':
    ret = timedelta(hours=duration)
  elif modifier == 'm':
    ret = timedelta(minutes=duration)
  elif modifier == 's':
    ret = timedelta(seconds=duration)

  return ret


def _GenSignedUrl(key, client_id, method, duration,
                  gcs_path, logger, region,
                  content_type=None, string_to_sign_debug=False):
  """Construct a string to sign with the provided key.

  Args:
    key: The private key to use for signing the URL.
    client_id: Client ID signing this URL.
    method: The HTTP method to be used with the signed URL.
    duration: timedelta for which the constructed signed URL should be valid.
    gcs_path: String path to the bucket of object for signing, in the form
        'bucket' or 'bucket/object'.
    logger: logging.Logger for warning and debug output.
    region: Geographic region in which the requested resource resides.
    content_type: Optional Content-Type for the signed URL. HTTP requests using
        the URL must match this Content-Type.
    string_to_sign_debug: If true AND logger is enabled for debug level,
        print string to sign to debug. Used to differentiate user's
        signed URL from the probing permissions-check signed URL.

  Returns:
    The complete url (string).
  """
  signing_time = _NowUTC()

  gs_host = config.get('Credentials', 'gs_host', 'storage.googleapis.com')
  signed_headers = {'host': gs_host}

  if method == 'RESUMABLE':
    method = 'POST'
    signed_headers['x-goog-resumable'] = 'start'
    if not content_type:
      logger.warn('Warning: no Content-Type header was specified with the -c '
                  'flag, so uploads to the resulting Signed URL must not '
                  'specify a Content-Type.')

  if content_type:
    signed_headers['content-type'] = content_type

  canonical_day = signing_time.strftime('%Y%m%d')
  canonical_time = signing_time.strftime('%Y%m%dT%H%M%SZ')
  canonical_scope = '{date}/{region}/storage/goog4_request'.format(
      date=canonical_day, region=region)

  signed_query_params = {}
  signed_query_params['x-goog-algorithm'] = _SIGNING_ALGO
  signed_query_params['x-goog-credential'] = client_id + '/' + canonical_scope
  signed_query_params['x-goog-date'] = canonical_time
  signed_query_params['x-goog-signedheaders'] = ';'.join(
      sorted(signed_headers.keys()))
  signed_query_params['x-goog-expires'] = '%d' % duration.total_seconds()

  canonical_resource = '/{}'.format(gcs_path)
  canonical_query_string = '&'.join(
      ['{}={}'.format(param, urllib.quote_plus(signed_query_params[param]))
       for param in sorted(signed_query_params.keys())])
  canonical_headers = '\n'.join(
      ['{}:{}'.format(header.lower(), signed_headers[header])
       for header in sorted(signed_headers.keys())]) + '\n'
  canonical_signed_headers = ';'.join(sorted(signed_headers.keys()))

  canonical_request = _CANONICAL_REQUEST_FORMAT.format(
      method=method, resource=canonical_resource,
      query_string=canonical_query_string, headers=canonical_headers,
      signed_headers=canonical_signed_headers, hashed_payload=_UNSIGNED_PAYLOAD)

  canonical_request_hasher = hashlib.sha256()
  canonical_request_hasher.update(canonical_request)
  hashed_canonical_request = base64.b16encode(
      canonical_request_hasher.digest()).lower()

  string_to_sign = _STRING_TO_SIGN_FORMAT.format(
      signing_algo=_SIGNING_ALGO, request_time=canonical_time,
      credential_scope=canonical_scope, hashed_request=hashed_canonical_request)

  if string_to_sign_debug and logger:
    logger.debug('Canonical request (ignore opening/closing brackets): [[[%s]]]'
                 % canonical_request)
    logger.debug('String to sign (ignore opening/closing brackets): [[[%s]]]'
                 % string_to_sign)

  signature = base64.b16encode(sign(key, string_to_sign, 'RSA-SHA256')).lower()

  final_url = _SIGNED_URL_FORMAT.format(
      host=gs_host, path=gcs_path, sig=signature,
      query_string=canonical_query_string)

  return final_url


def _ReadKeystore(ks_contents, passwd):
  ks = load_pkcs12(ks_contents, passwd)
  client_email = (ks.get_certificate()
                  .get_subject()
                  .CN.replace('.apps.googleusercontent.com',
                              '@developer.gserviceaccount.com'))

  return ks.get_privatekey(), client_email


def _ReadJSONKeystore(ks_contents, passwd=None):
  """Read the client email and private key from a JSON keystore.

  Assumes this keystore was downloaded from the Cloud Platform Console.
  By default, JSON keystore private keys from the Cloud Platform Console
  aren't encrypted so the passwd is optional as load_privatekey will
  prompt for the PEM passphrase if the key is encrypted.

  Arguments:
    ks_contents: JSON formatted string representing the keystore contents. Must
                 be a valid JSON string and contain the fields 'private_key'
                 and 'client_email'.
    passwd: Passphrase for encrypted private keys.

  Returns:
    key: Parsed private key from the keystore.
    client_email: The email address for the service account.

  Raises:
    ValueError: If unable to parse ks_contents or keystore is missing
                required fields.
  """
  ks = json.loads(ks_contents)

  if 'client_email' not in ks or 'private_key' not in ks:
    raise ValueError('JSON keystore doesn\'t contain required fields')

  client_email = ks['client_email']
  if passwd:
    key = load_privatekey(FILETYPE_PEM, ks['private_key'], passwd)
  else:
    key = load_privatekey(FILETYPE_PEM, ks['private_key'])

  return key, client_email


class UrlSignCommand(Command):
  """Implementation of gsutil url_sign command."""

  # Command specification. See base class for documentation.
  command_spec = Command.CreateCommandSpec(
      'signurl',
      command_name_aliases=['signedurl', 'queryauth'],
      usage_synopsis=_SYNOPSIS,
      min_args=2,
      max_args=NO_MAX,
      supported_sub_args='m:d:c:p:r:',
      file_url_ok=False,
      provider_url_ok=False,
      urls_start_arg=1,
      gs_api_support=[ApiSelector.XML, ApiSelector.JSON],
      gs_default_api=ApiSelector.JSON,
      argparse_arguments=[
          CommandArgument.MakeNFileURLsArgument(1),
          CommandArgument.MakeZeroOrMoreCloudURLsArgument()
      ]
  )
  # Help specification. See help_provider.py for documentation.
  help_spec = Command.HelpSpec(
      help_name='signurl',
      help_name_aliases=['signedurl', 'queryauth'],
      help_type='command_help',
      help_one_line_summary='Create a signed url',
      help_text=_DETAILED_HELP_TEXT,
      subcommand_help_text={},
  )

  def _ParseAndCheckSubOpts(self):
    # Default argument values
    delta = None
    method = 'GET'
    content_type = ''
    passwd = None
    region = _AUTO_DETECT_REGION

    for o, v in self.sub_opts:
      if o == '-d':
        if delta is not None:
          delta += _DurationToTimeDelta(v)
        else:
          delta = _DurationToTimeDelta(v)
      elif o == '-m':
        method = v
      elif o == '-c':
        content_type = v
      elif o == '-p':
        passwd = v
      elif o == '-r':
        region = v
      else:
        self.RaiseInvalidArgumentException()

    if delta is None:
      delta = timedelta(hours=1)

    if method not in ['GET', 'PUT', 'DELETE', 'HEAD', 'RESUMABLE']:
      raise CommandException('HTTP method must be one of'
                             '[GET|HEAD|PUT|DELETE|RESUMABLE]')

    return method, delta, content_type, passwd, region

  def _ProbeObjectAccessWithClient(self, key, client_email, gcs_path, logger,
                                   region):
    """Performs a head request against a signed url to check for read access."""

    # Choose a reasonable time in the future; if the user's system clock is
    # 60 or more seconds behind the server's this will generate an error.
    signed_url = _GenSignedUrl(key, client_email, 'HEAD', timedelta(seconds=60),
                               gcs_path, logger, region)

    try:
      h = GetNewHttp()
      req = Request(signed_url, 'HEAD')
      response = MakeRequest(h, req)

      if response.status_code not in [200, 403, 404]:
        raise HttpError.FromResponse(response)

      return response.status_code
    except HttpError as http_error:
      if http_error.has_attr('response'):
        error_response = http_error.response
        error_string = ('Unexpected HTTP response code %s while querying '
                        'object readability. Is your system clock accurate?'
                        % error_response.status_code)
        if error_response.content:
          error_string += ' Content: %s' % error_response.content
      else:
        error_string = ('Expected an HTTP response code of '
                        '200 while querying object readability, but received '
                        'an error: %s' % http_error)
      raise CommandException(error_string)

  def _EnumerateStorageUrls(self, in_urls):
    ret = []

    for url_str in in_urls:
      if ContainsWildcard(url_str):
        ret.extend([blr.storage_url for blr in self.WildcardIterator(url_str)])
      else:
        ret.append(StorageUrlFromString(url_str))

    return ret

  def RunCommand(self):
    """Command entry point for signurl command."""
    if not HAVE_OPENSSL:
      raise CommandException(
          'The signurl command requires the pyopenssl library (try pip '
          'install pyopenssl or easy_install pyopenssl)')

    method, delta, content_type, passwd, region = self._ParseAndCheckSubOpts()
    storage_urls = self._EnumerateStorageUrls(self.args[1:])
    region_cache = {}

    key = None
    client_email = None
    try:
      key, client_email = _ReadJSONKeystore(open(self.args[0], 'rb').read(),
                                            passwd)
    except ValueError:
      # Ignore and try parsing as a pkcs12.
      if not passwd:
        passwd = getpass.getpass('Keystore password:')
      try:
        key, client_email = _ReadKeystore(
            open(self.args[0], 'rb').read(), passwd)
      except ValueError:
        raise CommandException('Unable to parse private key from {0}'.format(
            self.args[0]))

    print 'URL\tHTTP Method\tExpiration\tSigned URL'
    for url in storage_urls:
      if url.scheme != 'gs':
        raise CommandException('Can only create signed urls from gs:// urls')
      if url.IsBucket():
        if region == _AUTO_DETECT_REGION:
          raise CommandException('Generating signed URLs for creating buckets'
                                 ' requires a region be specified via the -r '
                                 'option. Run `gsutil help signurl` for more '
                                 'information about the \'-r\' option.')
        gcs_path = url.bucket_name
        if method == 'RESUMABLE':
          raise CommandException('Resumable signed URLs require an object '
                                 'name.')
      else:
        # Need to url encode the object name as Google Cloud Storage does when
        # computing the string to sign when checking the signature.
        gcs_path = '{0}/{1}'.format(url.bucket_name,
                                    urllib.quote(url.object_name.encode(UTF8)))

      if region == _AUTO_DETECT_REGION:
        if url.bucket_name in region_cache:
          bucket_region = region_cache[url.bucket_name]
        else:
          try:
            _, bucket = self.GetSingleBucketUrlFromArg(
                'gs://{}'.format(url.bucket_name), bucket_fields=['location'])
          except Exception, e:
            raise CommandException(
                '{}: Failed to auto-detect location for bucket \'{}\'. Please '
                'ensure you have storage.buckets.get permission on the bucket '
                'or specify the bucket\'s location using the \'-r\' option.'
                .format(e.__class__.__name__, url.bucket_name))
          bucket_region = bucket.location.lower()
          region_cache[url.bucket_name] = bucket_region
      else:
        bucket_region = region
      final_url = _GenSignedUrl(key, client_email,
                                method, delta, gcs_path, self.logger,
                                bucket_region, content_type,
                                string_to_sign_debug=True)

      expiration = calendar.timegm((datetime.utcnow() + delta).utctimetuple())
      expiration_dt = datetime.fromtimestamp(expiration)

      print '{0}\t{1}\t{2}\t{3}'.format(url.url_string.encode(UTF8), method,
                                        (expiration_dt
                                         .strftime('%Y-%m-%d %H:%M:%S')),
                                        final_url.encode(UTF8))

      response_code = self._ProbeObjectAccessWithClient(
          key, client_email, gcs_path, self.logger, bucket_region)

      if response_code == 404:
        if url.IsBucket() and method != 'PUT':
          raise CommandException(
              'Bucket {0} does not exist. Please create a bucket with '
              'that name before a creating signed URL to access it.'
              .format(url))
        else:
          if method != 'PUT' and method != 'RESUMABLE':
            raise CommandException(
                'Object {0} does not exist. Please create/upload an object '
                'with that name before a creating signed URL to access it.'
                .format(url))
      elif response_code == 403:
        self.logger.warn(
            '%s does not have permissions on %s, using this link will likely '
            'result in a 403 error until at least READ permissions are granted',
            client_email, url)

    return 0
